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

Copyright 2019 Erlend Andersen
*/

#include "opendxmc/dosereportcontainer.h"

DoseReportContainer::DoseReportContainer(const std::vector<Material>& materialMap, std::shared_ptr<MaterialImageContainer> materialImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage, std::shared_ptr<TallyImageContainer> tallyImage)
{
	//createMaterialData(materialMap, materialImage, densityImage, doseImage);
	m_materialValues = std::make_shared<std::vector<DoseReportElement>>(createData(materialMap, materialImage, densityImage, doseImage, tallyImage));
	m_organValues = std::make_shared<std::vector<DoseReportElement>>();
	setDoseUnits(doseImage->dataUnits);
}

DoseReportContainer::DoseReportContainer(const std::vector<Material>& materialMap, const std::vector<std::string>& organMap, std::shared_ptr<MaterialImageContainer> materialImage, std::shared_ptr<OrganImageContainer> organImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage, std::shared_ptr<TallyImageContainer> tallyImage)
{
	//createMaterialData(materialMap, materialImage, densityImage, doseImage);
	m_materialValues = std::make_shared<std::vector<DoseReportElement>>(createData(materialMap, materialImage, densityImage, doseImage, tallyImage));
	//createOrganData(organMap, organImage, densityImage, doseImage);
	m_organValues = std::make_shared<std::vector<DoseReportElement>>(createData(organMap, organImage, densityImage, doseImage, tallyImage));
	setDoseUnits(doseImage->dataUnits);
}

template<typename RegionImage>
std::vector<DoseReportElement> DoseReportContainer::createData(const std::vector<Material>& organMap, RegionImage organImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage, std::shared_ptr<TallyImageContainer> tallyImage) const
{
	std::vector<std::string> names;
	names.reserve(organMap.size());
	for (const auto& m : organMap)
		names.push_back(m.prettyName());
	return createData(names, organImage, densityImage, doseImage, tallyImage);
}
template<typename RegionImage>
std::vector<DoseReportElement> DoseReportContainer::createData(const std::vector<std::string>& organMap, RegionImage organImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage, std::shared_ptr<TallyImageContainer> tallyImage) const
{
	std::vector<DoseReportElement> organValues(organMap.size());
	for (std::size_t i = 0; i < organMap.size(); ++i)
	{
		organValues[i].name = organMap[i];
		organValues[i].ID = i;
	}
	auto spacing = organImage->image->GetSpacing();
	const double voxelVolume = spacing[0] * spacing[1] * spacing[2] / 1000.0; //cm
	std::size_t size = organImage->imageData()->size();

	const auto mBuffer = organImage->imageData()->data();
	const auto densBuffer = densityImage->imageData()->data();
	const auto doseBuffer = doseImage->imageData()->data();
	const auto tallyBuffer = tallyImage->imageData()->data();

	for (std::size_t i = 0; i < size; ++i)
	{
		auto idx = static_cast<std::size_t>(mBuffer[i]);
		const double voxelMass = voxelVolume * densBuffer[i] * 0.001; //g->kg
		organValues[idx].voxels += 1;
		const double dose = doseBuffer[i] * voxelMass;
		organValues[idx].dose += dose;
		organValues[idx].mass += voxelMass;
		organValues[idx].doseMax = std::max(organValues[idx].doseMax, doseBuffer[i]);
		organValues[idx].nEvents += tallyBuffer[i];
	}
	for (std::size_t i = 0; i < size; ++i)
	{
		auto idx = static_cast<std::size_t>(mBuffer[i]);
		const double voxelMass = voxelVolume * densBuffer[i] * 0.001; //kg
		const double energy = doseBuffer[i] * voxelMass;
		organValues[idx].doseStd += (energy - organValues[idx].dose) * (energy - organValues[idx].dose);
	}
	for (auto& el : organValues)
	{
		el.volume = el.voxels * voxelVolume;
		if (el.mass > 0.0)
		{
			el.dose /= el.mass;
			el.doseStd = std::sqrt(el.doseStd / static_cast<double>(size)) / el.mass;
		}

	}
	return organValues;
}
