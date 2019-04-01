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
