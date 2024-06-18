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

Copyright 2024 Erlend Andersen
*/

#include "ctsegmentator/ctsegmentator.hpp"
#include <ctorgansegmentatorpipeline.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <execution>
#include <vector>

CTOrganSegmentatorPipeline::CTOrganSegmentatorPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void CTOrganSegmentatorPipeline::updateImageData(std::shared_ptr<DataContainer> data)
{
    m_requestCancel = false;
    if (!m_useOrganSegmentator)
        return;
    if (!data)
        return;
    if (!data->hasImage(DataContainer::ImageType::CT))
        return;
    emit dataProcessingStarted(ProgressWorkType::Segmentating);

    std::vector<std::uint8_t> org_array(data->size(), 0);
    const auto& ct_array = data->getCTArray();
    const auto& shape = data->dimensions();

    ctsegmentator::Segmentator s;

    const auto jobs = s.segmentJobs(ct_array, org_array, shape);
    const int nJobs = static_cast<int>(jobs.size());
    int currentJob = 1;
    emit importProgressChanged(0, 0, "Segmentating");

    bool success = true;

    const auto start_time = std::chrono::high_resolution_clock::now();

    for (const auto& job : jobs) {
        if (m_requestCancel) {
            emit importProgressChanged(-1, -1);
            emit dataProcessingFinished(ProgressWorkType::Segmentating);
            return;
        }

        success = success && s.segment(job, ct_array, org_array, shape);
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time);
        const auto rest = (duration * (nJobs - currentJob)) / currentJob;

        QString fmt("Segmentating %p% ");
        if (rest > std::chrono::hours(3))
            fmt += QString::fromStdString(std::to_string(std::chrono::duration_cast<std::chrono::hours>(rest).count()) + " hrs");
        else if (rest > std::chrono::minutes(3))
            fmt += QString::fromStdString(std::to_string(std::chrono::duration_cast<std::chrono::minutes>(rest).count()) + " min");
        else
            fmt += QString::fromStdString(std::to_string(std::chrono::duration_cast<std::chrono::seconds>(rest).count()) + " sec");
        fmt += " remaining";
        emit importProgressChanged(currentJob++, nJobs, fmt);
    }

    if (success) {

        // only use found organs
        std::vector<std::uint8_t> unique_organs(org_array.cbegin(), org_array.cend());
        std::sort(unique_organs.begin(), unique_organs.end());
        auto last = std::unique(unique_organs.begin(), unique_organs.end());
        unique_organs.erase(last, unique_organs.end());

        std::vector<std::uint8_t> reverse_map(unique_organs.back() + 1, 0);
        for (std::uint8_t i = 0; i < unique_organs.size(); ++i) {
            reverse_map[unique_organs[i]] = i;
        }
        std::transform(std::execution::par_unseq, org_array.cbegin(), org_array.cend(), org_array.begin(), [&](std::uint8_t i) {
            return reverse_map[i];
        });

        std::vector<std::string> names;
        for (const auto i : unique_organs) {
            if (i == 0)
                names.push_back("air");
            else
                names.push_back(s.organNames().at(i));
        }
        names.push_back("remainder");
        const auto remainderIdx = static_cast<std::uint8_t>(unique_organs.size());
        std::transform(org_array.cbegin(), org_array.cend(), ct_array.cbegin(), org_array.begin(), [remainderIdx](const auto o, const auto hu) {
            return o == 0 && hu > -500 ? remainderIdx : o;
        });

        data->setImageArray(DataContainer::ImageType::Organ, std::move(org_array));
        data->setOrganNames(names);
        emit imageDataChanged(data);
    }

    emit importProgressChanged(-1, -1);
    emit dataProcessingFinished(ProgressWorkType::Segmentating);
}

void CTOrganSegmentatorPipeline::setUseOrganSegmentator(bool trigger)
{
    m_useOrganSegmentator = trigger;
}

void CTOrganSegmentatorPipeline::cancelSegmentation()
{
    // making threadsafe
    auto ref = std::atomic_ref(m_requestCancel);
    ref.store(true);
}
