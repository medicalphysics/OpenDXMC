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

#include "saveload.h"
#include "h5wrapper.h"

SaveLoad::SaveLoad(QObject* parent)
	:QObject(parent)
{
	qRegisterMetaType<std::shared_ptr<ImageContainer>>();
	qRegisterMetaType<std::vector<Material>>();
	qRegisterMetaType<std::vector<std::string >>();
	qRegisterMetaType<std::shared_ptr<ImageContainer>>();
	qRegisterMetaType<DoseReportContainer>();
}


void SaveLoad::loadFromFile(const QString& path)
{
	emit processingDataStarted();

	clear();
	m_sources.clear();

	H5Wrapper wrapper(path.toStdString(), H5Wrapper::FileOpenType::ReadOnly);
	m_ctImage = wrapper.loadImage(ImageContainer::CTImage);
	m_materialImage= wrapper.loadImage(ImageContainer::MaterialImage);
	m_densityImage = wrapper.loadImage(ImageContainer::DensityImage);
	m_organImage = wrapper.loadImage(ImageContainer::OrganImage);
	m_doseImage = wrapper.loadImage(ImageContainer::DoseImage);

	m_materialList = wrapper.loadMaterials();
	m_organList = wrapper.loadOrganList();

	//creating dose data
	if (m_materialImage && m_densityImage && m_doseImage) {
		if (m_organImage) {
			DoseReportContainer cont(
				m_materialList, 
				m_organList, 
				std::static_pointer_cast<MaterialImageContainer>(m_materialImage),
				std::static_pointer_cast<OrganImageContainer>(m_organImage),
				std::static_pointer_cast<DensityImageContainer>(m_densityImage),
				std::static_pointer_cast<DoseImageContainer>(m_doseImage));
			emit doseDataChanged(cont);
		}
		else {
			DoseReportContainer cont(
				m_materialList, 
				std::static_pointer_cast<MaterialImageContainer>(m_materialImage),
				std::static_pointer_cast<DensityImageContainer>(m_densityImage),
				std::static_pointer_cast<DoseImageContainer>(m_doseImage));
			emit doseDataChanged(cont);
		}
	}
	m_sources = wrapper.loadSources();

	if (m_ctImage)
		emit imageDataChanged(m_ctImage);
	if (m_materialImage)
		emit imageDataChanged(m_materialImage);
	if (m_densityImage)
		emit imageDataChanged(m_densityImage);
	if (m_organImage)
		emit imageDataChanged(m_organImage);
	if (m_doseImage)
		emit imageDataChanged(m_doseImage);

	emit materialDataChanged(m_materialList);
	emit organDataChanged(m_organList);
	emit sourcesChanged(m_sources);

	emit processingDataEnded();
}

void SaveLoad::saveToFile(const QString& path)
{
	emit processingDataStarted();


	H5Wrapper wrapper(path.toStdString());
	if (m_ctImage)
		wrapper.saveImage(m_ctImage);
	if (m_densityImage)
		wrapper.saveImage(m_densityImage);
	if (m_organImage)
		wrapper.saveImage(m_organImage);
	if (m_materialImage)
		wrapper.saveImage(m_materialImage);
	if (m_doseImage)
		wrapper.saveImage(m_doseImage);

	wrapper.saveMaterials(m_materialList);
	wrapper.saveOrganList(m_organList);
	wrapper.saveSources(m_sources);
	emit processingDataEnded();
}

void SaveLoad::setImageData(std::shared_ptr<ImageContainer> image)
{
	if (!image)
		return;
	if (m_currentImageID != image->ID)
	{
		if (image->image)
		{
			m_ctImage = nullptr;
			m_densityImage = nullptr;
			m_organImage = nullptr;
			m_materialImage = nullptr;
			m_doseImage = nullptr;
		}
	}
	m_currentImageID = image->ID;
	if (image->imageType == ImageContainer::CTImage)
		m_ctImage = image;
	else if (image->imageType == ImageContainer::DensityImage)
		m_densityImage = image;
	else if (image->imageType == ImageContainer::DoseImage)
		m_doseImage = image;
	else if (image->imageType == ImageContainer::MaterialImage)
		m_materialImage = image;
	else if (image->imageType == ImageContainer::OrganImage)
		m_organImage = image;
}

void SaveLoad::setMaterials(const std::vector<Material>& materials)
{
	m_materialList = materials;
}

void SaveLoad::clear(void)
{
	m_currentImageID = 0;
	m_densityImage = nullptr;
	m_materialImage = nullptr;
	m_organImage = nullptr;
	m_ctImage = nullptr;
	m_organList.clear();
	m_materialList.clear();
	// we do not clear m_sources here
}

void SaveLoad::addSource(std::shared_ptr<Source> source)
{
	auto pos = std::find(m_sources.begin(), m_sources.end(), source);
	if (pos == m_sources.end())
		m_sources.push_back(source);
}

void SaveLoad::removeSource(std::shared_ptr<Source> source)
{
	auto pos = std::find(m_sources.begin(), m_sources.end(), source);
	if (pos != m_sources.end()) // we found it
		m_sources.erase(pos);
}
