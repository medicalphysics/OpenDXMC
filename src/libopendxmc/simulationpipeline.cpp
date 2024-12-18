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
SimulationPipeline::~SimulationPipeline()
{
    m_progress.setStopSimulation();
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
            const auto& max_iter = std::max_element(std::execution::par_unseq, marr.cbegin(), marr.cend());
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

void SimulationPipeline::finishingSimulation()
{
    emit imageDataChanged(m_data);
    killTimer(m_timerID);
    emit simulationRunning(false);
    emit dataProcessingFinished(ProgressWorkType::Simulating);
}

void SimulationPipeline::timerEvent(QTimerEvent* event)
{
    const auto [n, total] = m_progress.progress();
    const int percent = static_cast<int>((n * 100) / total);
    auto message = QString::fromStdString(m_progress.message());
    emit this->simulationProgress(message, percent);

    if (!m_progress.continueSimulation()) {
        finishingSimulation();
    }
}

template <int CORRECTION = 1>
void worker(bool deleteAirDose, int nthreads, std::shared_ptr<DataContainer> data, std::vector<std::shared_ptr<Beam>> beams, dxmc::TransportProgress* progress)
{
    using VoxelGrid = dxmc::AAVoxelGrid<5, CORRECTION, 255>;
    using World = dxmc::World<VoxelGrid>;

    World world;
    auto& vgrid = world.template addItem<VoxelGrid>();

    {
        std::vector<Material> materials;
        for (const auto& materialTemplate : data->getMaterials()) {
            auto material = Material::byWeight(materialTemplate.Z);
            if (!material) {
                // we failed to create material
                progress->setStopSimulation();
                return;
            } else {
                materials.push_back(material.value());
            }
        }
        const auto dims = data->dimensions();
        const auto spacing = data->spacing();
        const auto& densityArray = data->getDensityArray();
        const auto& materialArray = data->getMaterialArray();
        vgrid.setData(dims, densityArray, materialArray, materials);
        vgrid.setSpacing(spacing);
    }

    world.build();

    dxmc::Transport transport;
    if (nthreads > 0)
        transport.setNumberOfThreads(nthreads);

    const int Njobs = beams.size();

    for (int jobIdx = 0; jobIdx < Njobs; jobIdx++) {
        const auto& currentbeam = *(beams[jobIdx]);
        std::visit(
            [&](auto&& beam) {
                transport(world, beam, progress, true);
            },
            currentbeam);

        if (!progress->continueSimulation())
            return;
    }

    // collect dose
    const auto N = vgrid.size();
    {
        std::vector<double> dose(N);
        for (std::size_t i = 0; i < N; ++i)
            dose[i] = vgrid.doseScored(i).dose();

        if (deleteAirDose) {
            const auto matarr = data->getMaterialArray();
            std::transform(std::execution::par_unseq, dose.cbegin(), dose.cend(), matarr.cbegin(), dose.begin(), [](const auto d, const auto m) {
                return m > 0 ? d : 0.0;
            });
        }

        const auto& max_idx = std::max_element(std::execution::par_unseq, dose.cbegin(), dose.cend());
        if (*max_idx < 1) {
            std::transform(std::execution::par_unseq, dose.cbegin(), dose.cend(), dose.begin(), [](const auto d) {
                return d * 1e3;
            });
            data->setDoseUnits("uGy");
        } else {
            data->setDoseUnits("mGy");
        }

        data->setImageArray(DataContainer::ImageType::Dose, dose);
    }

    // collect number of events
    {
        std::vector<double> dose_count_array(N, 0);
        for (std::size_t i = 0; i < N; ++i)
            dose_count_array[i] = static_cast<double>(vgrid.doseScored(i).numberOfEvents());

        if (deleteAirDose) {
            const auto matarr = data->getMaterialArray();
            std::transform(std::execution::par_unseq, dose_count_array.cbegin(), dose_count_array.cend(), matarr.cbegin(), dose_count_array.begin(), [](const auto d, const auto m) {
                return m > 0 ? d : 0;
            });
        }
        data->setImageArray(DataContainer::ImageType::DoseCount, dose_count_array);
    }

    // collect stddev
    {
        std::vector<double> dose_var(N, 0.0);
        for (std::size_t i = 0; i < N; ++i)
            dose_var[i] = vgrid.doseScored(i).variance();

        if (deleteAirDose) {
            const auto matarr = data->getMaterialArray();
            std::transform(std::execution::par_unseq, dose_var.cbegin(), dose_var.cend(), matarr.cbegin(), dose_var.begin(), [](const auto d, const auto m) {
                return m > 0 ? d : 0.0;
            });
        }
        if (data->units(DataContainer::ImageType::Dose)[0] == 'u') {
            std::for_each(std::execution::par_unseq, dose_var.begin(), dose_var.end(), [](auto& v) { v *= 1e6; });
        }

        data->setImageArray(DataContainer::ImageType::DoseVariance, dose_var);
    }

    progress->setStopSimulation();
}

void SimulationPipeline::startSimulation()
{
    emit dataProcessingStarted(ProgressWorkType::Simulating);
    emit simulationRunning(true);
    m_timerID = startTimer(3000, Qt::VeryCoarseTimer);

    if (!testIfReadyForSimulation(true)) {
        emit simulationRunning(false);
        return;
    }
    if (m_lowenergyCorrection == 0) {
        std::jthread t(worker<0>, m_deleteAirDose, m_threads, m_data, m_beams, &m_progress);
        t.detach();
    } else if (m_lowenergyCorrection == 1) {
        std::jthread t(worker<1>, m_deleteAirDose, m_threads, m_data, m_beams, &m_progress);
        t.detach();
    } else {
        std::jthread t(worker<2>, m_deleteAirDose, m_threads, m_data, m_beams, &m_progress);
        t.detach();
    }
}

void SimulationPipeline::stopSimulation()
{
    m_progress.setStopSimulation();
}
