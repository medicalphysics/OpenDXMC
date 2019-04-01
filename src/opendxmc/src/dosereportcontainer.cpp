#include "dosereportcontainer.h"

DoseReportContainer::DoseReportContainer(const std::vector<Material>& materialMap, std::shared_ptr<MaterialImageContainer> materialImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage)
{
	createMaterialData(materialMap, materialImage, densityImage, doseImage);
	m_organValues = std::make_shared<std::vector<DoseReportElement>>();
}

DoseReportContainer::DoseReportContainer(const std::vector<Material>& materialMap, const std::vector<std::string>& organMap, std::shared_ptr<MaterialImageContainer> materialImage, std::shared_ptr<OrganImageContainer> organImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage)
{
	createMaterialData(materialMap, materialImage, densityImage, doseImage);
	createOrganData(organMap, organImage, densityImage, doseImage);
}

void DoseReportContainer::createMaterialData(const std::vector<Material>& materialMap, std::shared_ptr<MaterialImageContainer> materialImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage)
{
	m_materialValues = std::make_shared<std::vector<DoseReportElement>>(materialMap.size());
	for (std::size_t i = 0; i < materialMap.size(); ++i)
	{
		(*m_materialValues)[i].name = materialMap[i].prettyName();
		(*m_materialValues)[i].ID = i;
	}

	auto spacing = materialImage->image->GetSpacing();
	double voxelVolume = spacing[0] * spacing[1] * spacing[2] / 1000.0;
	std::size_t size = materialImage->imageData()->size();

	auto mBuffer = materialImage->imageData()->data();
	auto dBuffer = densityImage->imageData()->data();
	auto doseBuffer = doseImage->imageData()->data();

	for (std::size_t i = 0; i < size; ++i)
	{
		auto idx = static_cast<std::size_t>(mBuffer[i]);
		(*m_materialValues)[idx].voxels += 1;
		(*m_materialValues)[idx].dose += doseBuffer[i];
		(*m_materialValues)[idx].mass += dBuffer[i];
	}
	for (auto& el : *m_materialValues)
	{
		el.volume = el.voxels * voxelVolume;
		el.dose /= el.voxels;
		el.mass *= voxelVolume / 1000.0; // kg
	}
}

void DoseReportContainer::createOrganData(const std::vector<std::string>& organMap, std::shared_ptr<OrganImageContainer> organImage, std::shared_ptr<DensityImageContainer> densityImage, std::shared_ptr<DoseImageContainer> doseImage)
{
	m_organValues = std::make_shared<std::vector<DoseReportElement>>(organMap.size());
	for (std::size_t i = 0; i < organMap.size(); ++i)
	{
		(*m_organValues)[i].name = organMap[i];
		(*m_organValues)[i].ID = i;
	}
	auto spacing = organImage->image->GetSpacing();
	double voxelVolume = spacing[0] * spacing[1] * spacing[2] / 1000.0;
	std::size_t size = organImage->imageData()->size();

	auto mBuffer = organImage->imageData()->data();
	auto dBuffer = densityImage->imageData()->data();
	auto doseBuffer = doseImage->imageData()->data();

	for (std::size_t i = 0; i < size; ++i)
	{
		auto idx = static_cast<std::size_t>(mBuffer[i]);
		const double voxelMass = voxelVolume * dBuffer[i] * 0.001; //kg
		(*m_organValues)[idx].voxels += 1;
		(*m_organValues)[idx].dose += doseBuffer[i] * voxelMass;
		(*m_organValues)[idx].mass += voxelMass;
	}
	for (auto& el : *m_organValues)
	{
		el.volume = el.voxels * voxelVolume;
		if (el.mass > 0.0)
			el.dose /= el.mass;
	}

}
