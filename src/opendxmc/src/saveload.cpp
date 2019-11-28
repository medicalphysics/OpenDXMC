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

#include <QByteArray>
#include <QFileDialog>
#include <QSettings>
#include <QApplication>

#include "imagecontainer.h"
#include "source.h"
#include <hdf5_hl.h>
#include <vtk_hdf5.h>

#include <memory>
#include <string>
#include <type_traits>

SaveLoad::SaveLoad(QObject* parent)
	:QObject(parent)
{
	
}

template<typename T, ImageContainer::ImageType type>
std::shared_ptr<ImageContainer> readArrayBuffer(hid_t group_id, const std::array<std::size_t, 3>& dimensions, const std::array<double, 3>& dataSpacing, const std::array<double, 3>& origin, const std::string& units)
{
	auto array_name = ImageContainer::getImageName(type);
	hid_t data_type;
	if (std::is_same<T, double>::value)
		data_type = H5T_IEEE_F64LE;
	else if (std::is_same<T, float>::value)
		data_type = H5T_IEEE_F32LE;
	else if (std::is_same<T, unsigned char>::value)
		data_type = H5T_STD_U8LE;
	else
		return nullptr;

	auto data_size = dimensions[0] * dimensions[1] * dimensions[2];
	auto data = std::make_shared<std::vector<T>>(data_size);
	T* data_buffer = data->data();
	auto read_status = H5LTread_dataset(group_id, array_name.c_str(), data_type, data_buffer);

	if (read_status < 0)
		return nullptr;

	auto image = std::make_shared<ImageContainer>(type, data, dimensions, dataSpacing, origin, units);
	return image;
}

std::shared_ptr<ImageContainer> readArray(hid_t file_id, ImageContainer::ImageType type)
{
	auto array_name = ImageContainer::getImageName(type);

	//opening array group
	hid_t group_id;
	H5G_info_t group_info;
	herr_t group_status = H5Gget_info_by_name(file_id, "arrays", &group_info, H5P_DEFAULT);
	if (group_status < 0)
	{
		// error no group named array
		return nullptr;
	}
	group_id = H5Gopen2(file_id, "arrays", H5P_DEFAULT);

	//finding dataset
	herr_t dataset_exists = H5LTfind_dataset(group_id, array_name.c_str());
	if (dataset_exists == 0)
	{
		//dataset do not exists
		H5Gclose(group_id);
		return nullptr;
	}

	std::array<hsize_t, 3> h5_dims;
	H5T_class_t class_id;
	size_t data_size;
	herr_t dataset_status = H5LTget_dataset_info(group_id, array_name.c_str(), h5_dims.data(), &class_id, &data_size);
	if (dataset_status < 0)
	{
		H5Gclose(group_id);
		return nullptr; //some error, this should never happend
	}

	//creating attributes 
	std::array<std::size_t, 3> dims{ h5_dims[0], h5_dims[1] , h5_dims[2] };
	char units_buffer[64];
	herr_t units_status = H5LTget_attribute_string(group_id, array_name.c_str(), "units", units_buffer);
	std::string units(units_buffer);
	std::array<double, 3> spacing;
	herr_t spacing_status = H5LTget_attribute_double(group_id, array_name.c_str(), "spacing", spacing.data());
	std::array<double, 6> cosines;
	herr_t cosines_status = H5LTget_attribute_double(group_id, array_name.c_str(), "direction_cosines", cosines.data());
	std::array<double, 3> origin{ 0,0,0 };
	herr_t origin_status = H5LTget_attribute_double(group_id, array_name.c_str(), "origin", origin.data());
	if (units_status < 0 || spacing_status < 0 || cosines_status < 0)
	{
		H5Gclose(group_id);
		return nullptr; // error
	}

	std::shared_ptr<ImageContainer> image = nullptr;

	if (type == ImageContainer::CTImage)
	{
		image = readArrayBuffer<float, ImageContainer::CTImage>(group_id, dims, spacing, origin, units);
	}
	else if (type == ImageContainer::DensityImage)
	{
		image = readArrayBuffer<double, ImageContainer::DensityImage>(group_id, dims, spacing, origin, units);
	}
	else if (type == ImageContainer::DoseImage)
	{
		image = readArrayBuffer<double, ImageContainer::DoseImage>(group_id, dims, spacing, origin, units);
	}
	else if (type == ImageContainer::MaterialImage)
	{
		image = readArrayBuffer<unsigned char, ImageContainer::MaterialImage>(group_id, dims, spacing, origin, units);
	}
	else if (type == ImageContainer::OrganImage)
	{
		image = readArrayBuffer<unsigned char, ImageContainer::OrganImage>(group_id, dims, spacing, origin, units);
	}
	if (image)
	{
		image->directionCosines = cosines;
	}
	H5Gclose(group_id);
	return image;
}

void SaveLoad::loadFromFile()
{
	//getting file
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");

	QString path = settings.value("saveload/path").value<QString>();
	if (path.isNull())
		path = ".";
	auto topList = QApplication::topLevelWidgets();
	QWidget* parent = nullptr;
	if (topList.size() > 0)
		parent = topList[0];

	path = QFileDialog::getOpenFileName(parent, tr("Open simulation"), path, tr("HDF5 (*.h5)"));
	if (path.isNull())
		return;

	emit processingDataStarted();
	QByteArray bytes = path.toLocal8Bit();
	char* c_path = bytes.data();
	hid_t fid = H5Fopen(c_path, H5F_ACC_RDONLY, H5P_DEFAULT);
	if (fid < 0)
	{
		return; //error opening file
	}
	m_ctImage = readArray(fid, ImageContainer::CTImage);
	m_doseImage = readArray(fid, ImageContainer::DoseImage);
	m_densityImage = readArray(fid, ImageContainer::DensityImage);
	m_organImage = readArray(fid, ImageContainer::OrganImage);
	m_materialImage = readArray(fid, ImageContainer::MaterialImage);
	H5Fclose(fid);
	std::array<std::shared_ptr<ImageContainer>, 6> images{ m_ctImage, m_densityImage, m_organImage, m_materialImage, m_doseImage };
	auto ID = ImageContainer::generateID();
	for (auto im : images)
	{
		if (im)
		{
			im->ID = ID;
			emit imageDataChanged(im);
		}
	}
	emit processingDataEnded();
	settings.setValue("saveload/path", path);
}

herr_t createArray(hid_t file_id, std::shared_ptr<ImageContainer> image)
{
	if (!image)
		return -1;
	if (!(image->image))
		return -1;

	//finding data type
	hid_t type_id;
	if (image->image->GetScalarType() == VTK_FLOAT)
		type_id = H5T_IEEE_F32LE;
	else if (image->image->GetScalarType() == VTK_DOUBLE)
		type_id = H5T_IEEE_F64LE;
	else if (image->image->GetScalarType() == VTK_UNSIGNED_CHAR)
		type_id = H5T_STD_U8LE;
	else
		return -1;

	//creating array group
	hid_t group_id;
	H5G_info_t group_info;
	herr_t group_status = H5Gget_info_by_name(file_id, "arrays", &group_info, H5P_DEFAULT);
	if (group_status < 0) // need to create group
	{
		group_id = H5Gcreate2(file_id, "arrays", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	}
	else {
		group_id = H5Gopen2(file_id, "arrays", H5P_DEFAULT);
	}

	std::array<hsize_t, 3> dims;
	int* vtk_dims = image->image->GetDimensions();
	for (std::size_t i = 0; i < 3; ++i)
		dims[i] = static_cast<hsize_t>(vtk_dims[i]);

	std::string name = image->getImageName();

	herr_t status = H5LTmake_dataset(group_id, name.c_str(), 3, dims.data(), type_id, image->image->GetScalarPointer());

	//setting attributes
	std::string att_name = "spacing";
	auto att_status = H5LTset_attribute_double(group_id, name.c_str(), att_name.c_str(), image->image->GetSpacing(), 3);
	att_name = "direction_cosines";
	att_status = H5LTset_attribute_double(group_id, name.c_str(), att_name.c_str(), image->directionCosines.data(), 6);
	att_name = "units";
	att_status = H5LTset_attribute_string(group_id, name.c_str(), att_name.c_str(), image->dataUnits.c_str());
	att_name = "origin";
	att_status = H5LTset_attribute_double(group_id, name.c_str(), att_name.c_str(), image->image->GetOrigin(), 3);
	auto group_id_status = H5Gclose(group_id); // close group
	return status;
}

void SaveLoad::saveToFile()
{
	//getting file
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");

	QString path = settings.value("saveload/path").value<QString>();
	if (path.isNull())
		path = ".";
	auto topList = QApplication::topLevelWidgets();
	QWidget* parent = nullptr;
	if (topList.size() > 0)
		parent = topList[0];

	path = QFileDialog::getSaveFileName(parent, tr("Save simulation"), path, tr("HDF5 (*.h5)"));
	if (path.isNull())
		return;

	emit processingDataStarted();
	//creating file 
	QByteArray bytes = path.toLocal8Bit();
	char* c_path = bytes.data();
	hid_t fid = H5Fcreate(c_path, H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	if (fid < 0)
	{
		//error creating file 
		return;
	}
	if (m_ctImage)
		auto status = createArray(fid, m_ctImage);
	if (m_densityImage)
		auto status = createArray(fid, m_densityImage);
	if (m_organImage)
		auto status = createArray(fid, m_organImage);
	if (m_materialImage)
		auto status = createArray(fid, m_materialImage);
	if (m_doseImage)
		auto status = createArray(fid, m_doseImage);
	H5Fclose(fid);
	emit processingDataEnded();
	settings.setValue("saveload/path", path);
}

void SaveLoad::setImageData(std::shared_ptr<ImageContainer> image)
{
	if (m_currentImageID != image->ID)
	{
		m_ctImage = nullptr;
		m_densityImage = nullptr;
		m_organImage = nullptr;
		m_materialImage = nullptr;
		m_doseImage = nullptr;
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
	m_materialList.clear();
	m_materialList.reserve(materials.size());
	for (const auto& m : materials)
	{
		m_materialList.push_back(m.name());
	}
}
