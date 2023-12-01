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

#include <dosetablepipeline.hpp>

#include <ranges>

DoseTablePipeline::DoseTablePipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void DoseTablePipeline::updateImageData(std::shared_ptr<DataContainer> data)
{
    emit clearTable();
    if (!data->hasImage(DataContainer::ImageType::Organ) || !data->hasImage(DataContainer::ImageType::Dose)) {
        emit clearTable();
        return;
    }

    const auto& organArray = data->getOrganArray();
    const auto organNames = data->getOrganNames();
    const auto doseArray = data->getDoseArray();
    const auto densityArray = data->getDensityArray();

    const auto voxelVolume = std::reduce(data->dimensions().cbegin(), data->dimensions().cend(), 1.0, std::multiplies {});
    std::vector<double> energy_imparted(doseArray.size());
    std::transform(std::execution::par_unseq, doseArray.cbegin(), doseArray.cend(), densityArray.cbegin(), energy_imparted.begin(),
        [=](const auto& dose, const auto& dens) {
            const auto mass = voxelVolume * dens;
            return dose * mass;
        });

    for (std::uint8_t i = 0; i < organNames.size(); ++i) {
        const auto Nvoxels = std::count(std::execution::par_unseq, organArray.cbegin(), organArray.cend(), i);
        if (Nvoxels > 0) {
            const double energy = std::transform_reduce(std::execution::par_unseq, energy_imparted.cbegin(), energy_imparted.cend(), organArray.cbegin(), 0.0, std::plus {},
                [=](const auto& d, const auto& o) {
                    return o == i ? d : 0.0;
                });
            const double mass = voxelVolume * std::transform_reduce(std::execution::par_unseq, densityArray.cbegin(), densityArray.cend(), organArray.cbegin(), 0.0, std::plus {}, [=](const auto& d, const auto& o) {
                return o == i ? d : 0.0;
            });

            emit doseData(0, i, QVariant { QString::fromStdString(organNames[i]) });
            emit doseData(1, i, QVariant { static_cast<int>(Nvoxels) });
            emit doseData(2, i, QVariant { Nvoxels * voxelVolume });
            emit doseData(3, i, QVariant { mass });
            emit doseData(4, i, QVariant { energy / mass });
        }
    }
}
