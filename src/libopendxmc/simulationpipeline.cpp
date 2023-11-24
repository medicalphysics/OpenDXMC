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

#include <simulationpipeline.hpp>

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
    // we do not add existing beam
    for (const auto& b : m_beams)
        if (actor == b)
            return;
    m_beams.push_back(actor);
    emit simulationReady(testIfReadyForSimulation(false));
}

void SimulationPipeline::removeBeamActor(std::shared_ptr<BeamActorContainer> actor)
{
    std::size_t Idx = m_beams.size();
    for (std::size_t i = 0; i < m_beams.size(); ++i) {
        if (actor == m_beams[i]) {
            Idx = i;
        }
    }
    if (Idx < m_beams.size())
        m_beams.erase(m_beams.begin() + Idx);

    emit simulationReady(testIfReadyForSimulation(false));
}

void SimulationPipeline::setNumberOfThreads(int nthreads)
{
    const auto nthreads_max = 2 * std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    m_threads = std::clamp(nthreads, 0, nthreads_max);
}

void SimulationPipeline::startSimulation()
{
}

void SimulationPipeline::stopSimulation()
{
}
