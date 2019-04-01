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

#include "simulationpipeline.h"
#include "transport.h"

#include "vtkType.h"


constexpr double KEV_TO_MJ = 1.6021773e-13;


SimulationPipeline::SimulationPipeline(QObject *parent)
	:QObject(parent)
{
	qRegisterMetaType<DoseReportContainer>();
	m_world = World();	
}

void SimulationPipeline::setImageData(std::shared_ptr<ImageContainer> image)
{
	if (image->imageType == ImageContainer::DensityImage)
		m_densityImage = std::static_pointer_cast<DensityImageContainer>(image);
		//m_densityImage = static_cast<DensityImageContainer>(image);
	else if (image->imageType == ImageContainer::MaterialImage)
		m_materialImage = std::static_pointer_cast<MaterialImageContainer>(image);
	else if (image->imageType == ImageContainer::OrganImage)
		m_organImage = std::static_pointer_cast<OrganImageContainer>(image);
}


void SimulationPipeline::setMaterials(const std::vector<Material>& materials)
{
	m_world.clearMaterialMap();
	for (auto const & material : materials)
		m_world.addMaterialToMap(material);
}

void SimulationPipeline::runSimulation(const std::vector<std::shared_ptr<Source>> sources)
{
	emit processingDataStarted();

	auto dummyImage = std::make_shared<DoseImageContainer>();
	emit imageDataChanged(dummyImage);
	DoseReportContainer dummyContainer;
	emit doseDataChanged(dummyContainer);

	if (!m_densityImage || !m_materialImage)
	{
		emit processingDataEnded();
		return;
	}
	if (m_densityImage->ID != m_materialImage->ID)
	{
		emit processingDataEnded();
		return;
	}

	std::array<double, 3> spacing;
	std::array<std::size_t, 3> dimensions;
	for (std::size_t i = 0; i < 3; ++i)
	{
		spacing[i] = (m_densityImage->image->GetSpacing())[i];
		dimensions[i]= static_cast<std::size_t>((m_densityImage->image->GetDimensions())[i]);
	}
	m_world.setSpacing(spacing);
	m_world.setDimensions(dimensions);
	m_world.setDirectionCosines(m_densityImage->directionCosines);
	m_world.setMaterialIndexArray(m_materialImage->imageData());
	m_world.setDensityArray(m_densityImage->imageData());
	
	auto totalDose = std::make_shared<std::vector<double>>(m_world.size(), 0.0);

	for (std::shared_ptr<Source> s : sources)
	{
		
		m_world.setAttenuationLutMaxEnergy(s->maxPhotonEnergyProduced());
		m_world.validate();
		auto dose = transport::run(m_world, s.get());
		std::transform(std::execution::par_unseq, totalDose->begin(), totalDose->end(), dose.begin(), totalDose->begin(), std::plus<double>());
	}

	
	std::array<double, 3> origin;
	for (int i = 0; i < 3; ++i)
		origin[i] = -(dimensions[i] * spacing[i] * 0.5);
	
	auto doseContainer = std::make_shared<DoseImageContainer>(totalDose, dimensions, spacing, origin);
	doseContainer->directionCosines = m_densityImage->directionCosines;
	doseContainer->ID = m_densityImage->ID;

	if (m_organImage && (m_organList.size() > 0)) {
		if (m_organImage->ID == m_materialImage->ID) {
			DoseReportContainer cont(m_world.materialMap(), m_organList, m_materialImage, m_organImage, m_densityImage, doseContainer);
			emit doseDataChanged(cont);
		}
		else {
			DoseReportContainer cont(m_world.materialMap(), m_materialImage, m_densityImage, doseContainer);
			emit doseDataChanged(cont);
		}
	}
	else {
		DoseReportContainer cont(m_world.materialMap(), m_materialImage, m_densityImage, doseContainer);
		emit doseDataChanged(cont);
	}

	if (m_ignoreAirDose)
	{
		auto mat = m_world.materialIndexArray();
		std::transform(std::execution::par_unseq, totalDose->begin(), totalDose->end(), mat->begin(), totalDose->begin(),
			[](double e, unsigned char i)->double {return i == 0 ? 0.0 : e; });
	}	
	emit imageDataChanged(doseContainer);

	emit processingDataEnded();
}
