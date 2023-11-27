/*This file is part of OpenDXMC.

OpenDXMC is free software : you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenDXMC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenDXMC. If not, see < https://www.gnu.org/licenses/>.

Copyright 2023 Erlend Andersen
*/

#include <beamactorcontainer.hpp>
#include <dxmc_specialization.hpp>
#include <simulationpipeline.hpp>

#include <dxmc/transport.hpp>
#include <dxmc/world/world.hpp>
#include <dxmc/world/worlditems/aavoxelgrid.hpp>

#include <algorithm>
#include <execution>
#include <thread>

SimulationPipeline::SimulationPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void SimulationPipeline::updateImageData(std::shared_ptr<DataContainer> data)
{
    m_data = data;
    emit simulationReady(testIfReadyForSimulation());
}

bool SimulationPipeline::testIfReadyForSimulation(bool test_image) const
{
    bool ready = false;
    if (m_data) {
        const auto n_materials = m_data->getMaterials().size();
        ready = m_data->hasImage(DataContainer::ImageType::Density);
        ready = ready && m_data->hasImage(DataContainer::ImageType::Material);
        ready = ready && n_materials > 0;
        if (ready && test_image) {
            // check if materials is satisfied
            const auto& marr = m_data->getMaterialArray();
            const auto& [min_iter, max_iter] = std::minmax_element(std::execution::par_unseq, marr.cbegin(), marr.cend());
            ready = ready && *min_iter == 0;
            ready = ready && *max_iter < n_materials;
        }
    }

    ready = ready && m_beams.size() > 0;
    return ready;
}

void SimulationPipeline::addBeamActor(std::shared_ptr<BeamActorContainer> actor)
{
    if (actor) {
        auto beam = actor->getBeam();
        if (beam) {
            // we do not add existing beam
            for (const auto& b : m_beams)
                if (beam == b)
                    return;
            m_beams.push_back(beam);
            emit simulationReady(testIfReadyForSimulation(false));
        }
    }
}

void SimulationPipeline::removeBeamActor(std::shared_ptr<BeamActorContainer> actor)
{
    if (actor) {
        auto beam = actor->getBeam();
        if (beam) {
            std::size_t Idx = m_beams.size();
            for (std::size_t i = 0; i < m_beams.size(); ++i) {
                if (beam == m_beams[i]) {
                    Idx = i;
                }
            }
            if (Idx < m_beams.size())
                m_beams.erase(m_beams.begin() + Idx);
            emit simulationReady(testIfReadyForSimulation(false));
        }
    }
}

void SimulationPipeline::setNumberOfThreads(int nthreads)
{
    const auto nthreads_max = 2 * std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    m_threads = std::clamp(nthreads, 0, nthreads_max);
}

void SimulationPipeline::startSimulation()
{
    run();
}

void SimulationPipeline::stopSimulation()
{
}

void SimulationPipeline::run()
{
    emit simulationRunning(true);
    if (!testIfReadyForSimulation(true)) {
        emit simulationRunning(false);
        return;
    }

    using VoxelGrid = dxmc::AAVoxelGrid<double, 5, 1, 255>;
    using World = dxmc::World<double, VoxelGrid>;

    World world;
    auto& vgrid = world.addItem<VoxelGrid>();

    {
        std::vector<Material> materials;
        for (const auto& materialTemplate : m_data->getMaterials()) {
            auto material = Material::byWeight(materialTemplate.Z);
            if (!material) {
                // we failed to create material
                emit simulationRunning(false);
                return;
            } else {
                materials.push_back(material.value());
            }
        }
        const auto dims = m_data->dimensions();
        const auto spacing = m_data->spacing();
        const auto& densityArray = m_data->getDensityArray();
        const auto& materialArray = m_data->getMaterialArray();
        vgrid.setData(dims, densityArray, materialArray, materials);
        vgrid.setSpacing(spacing);
    }

    world.build();

    dxmc::Transport transport;
    if (m_threads > 0)
        transport.setNumberOfThreads(m_threads);

    dxmc::TransportProgress progress;

    const auto Njobs = m_beams.size();

    for (std::size_t jobIdx = 0; jobIdx < Njobs; jobIdx++) {
        const auto& currentbeam = *(m_beams[jobIdx]);
        std::visit([&](auto&& beam) {
            bool running = true;
            std::thread job([&, this]() {
                transport(world, beam, &progress, true);
                running = false;
            });
            while (running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                auto elapsed = QString::fromStdString(progress.message());
                emit this->simulationProgress(elapsed, jobIdx, Njobs);
            }
            job.join();
        },
            currentbeam);
    }

    // collect dose
    const auto N = vgrid.size();
    {
        std::vector<double> dose(N);
        for (std::size_t i = 0; i < N; ++i)
            dose[i] = vgrid.doseScored(i).dose();

        if (m_deleteAirDose) {
            const auto matarr = m_data->getMaterialArray();
            std::transform(std::execution::par_unseq, dose.cbegin(), dose.cend(), matarr.cbegin(), dose.begin(), [](const auto d, const auto m) {
                return m > 0 ? d : 0.0;
            });
        }

        const auto& [min_idx, max_idx] = std::minmax_element(std::execution::par_unseq, dose.cbegin(), dose.cend());
        if (*min_idx < 1 && *max_idx < 1) {
            std::transform(std::execution::par_unseq, dose.cbegin(), dose.cend(), dose.begin(), [](const auto d) {
                return d * 1e3;
            });
            m_data->setDoseUnits("uGy");
        } else {
            m_data->setDoseUnits("mGy");
        }

        m_data->setImageArray(DataContainer::ImageType::Dose, dose);
    }

    emit imageDataChanged(m_data);
    emit simulationRunning(false);
}
