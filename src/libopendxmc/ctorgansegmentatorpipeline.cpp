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

#include <vector>

CTOrganSegmentatorPipeline::CTOrganSegmentatorPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void CTOrganSegmentatorPipeline::updateImageData(std::shared_ptr<DataContainer> data)
{
    if (!data)
        return;
    if (!data->hasImage(DataContainer::ImageType::CT))
        return;
    emit dataProcessingStarted();

    std::vector<std::uint8_t> org_array(data->size(), 0);

    ctsegmentator::Segmentator S;
    bool success = S.segment(data->getCTArray(), org_array, data->dimensions());
    if (success) {
        data->setImageArray(DataContainer::ImageType::Organ, std::move(org_array));
        std::vector<std::string> names;
        for (int i = 0; i < 60; ++i)
            names.push_back("test");
        data->setOrganNames(names);
    }

    emit imageDataChanged(data);
    emit dataProcessingFinished();
}
