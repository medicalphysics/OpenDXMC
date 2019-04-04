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

#pragma once

#include "material.h"
#include "imagecontainer.h"
#include <memory>
#include <vector>
#include <string>


struct DoseReportElement
{
	std::size_t voxels = 0;
	double mass = 0.0;
	double volume = 0.0;
	double dose = 0.0;
	double doseStd = 0.0;
	std::size_t ID = 0;
	std::string name;
};

class DoseReportContainer
{
public:
	DoseReportContainer() { m_materialValues = std::make_shared<std::vector<DoseReportElement>>(); m_organValues = std::make_shared<std::vector<DoseReportElement>>(); }
	DoseReportContainer(
		const std::vector<Material>& materialMap, 
		std::shared_ptr<MaterialImageContainer> materialImage, 
		std::shared_ptr<DensityImageContainer> densityImage, 
		std::shared_ptr<DoseImageContainer> doseImage);
	DoseReportContainer(
		const std::vector<Material>& materialMap,
		const std::vector<std::string>& organMap,
		std::shared_ptr<MaterialImageContainer> materialImage,
		std::shared_ptr<OrganImageContainer> organImage,
		std::shared_ptr<DensityImageContainer> densityImage,
		std::shared_ptr<DoseImageContainer> doseImage);
	std::shared_ptr<std::vector<DoseReportElement>> organData() const { return m_organValues; }
	std::shared_ptr<std::vector<DoseReportElement>> materialData() const { return m_materialValues; }
protected:
	void createMaterialData(const std::vector<Material>& materialMap,
		std::shared_ptr<MaterialImageContainer> materialImage,
		std::shared_ptr<DensityImageContainer> densityImage,
		std::shared_ptr<DoseImageContainer> doseImage);
	void createOrganData(
		const std::vector<std::string>& organMap, 
		std::shared_ptr<OrganImageContainer> organImage, 
		std::shared_ptr<DensityImageContainer> densityImage, 
		std::shared_ptr<DoseImageContainer> doseImage);
private:
	std::shared_ptr<std::vector<DoseReportElement>> m_materialValues;
	std::shared_ptr<std::vector<DoseReportElement>> m_organValues;
};
