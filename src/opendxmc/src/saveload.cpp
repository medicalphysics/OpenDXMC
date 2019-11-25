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

#include "imagecontainer.h"
#include "source.h"
#include <hdf5_hl.h>
#include <vtk_hdf5.h>

#include <memory>
#include <string>

SaveLoad::SaveLoad(QObject* parent)
	:QObject(parent)
{
	/*std::vector<int> testArr(512 * 512 * 5, 0);
	std::array<hsize_t, 3> dims={ 512, 512, 5 };
	int* buffer = testArr.data();

	auto file_id = H5Fcreate("ex_lite1.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	// create and write an integer type dataset named "dset" 
	H5LTmake_dataset(file_id, "/dset", 3, dims.data(), H5T_NATIVE_INT, buffer);

	// close file 
	H5Fclose(file_id);



	auto file = vtkhdf5_H5Fcreate("test.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

	auto dataspace = vtkhdf5_H5Screate_simple(3, dims.data(), NULL);
	//auto datatype = vtkhdf5_H5Tcopy(H5T_NATIVE_INT64);
	auto datatype = vtkhdf5_H5Tarray_create2(H5T_NATIVE_INT64, 3, dims.data());
	auto status = H5Tset_order(datatype, H5T_ORDER_LE);

	auto dset = H5Dcreate(file, "testArr", datatype, dataspace, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
	status = vtkhdf5_H5Dwrite(dset, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
		buffer);

	status = vtkhdf5_H5Tclose(datatype);
	status = vtkhdf5_H5Dclose(dset);
	status = vtkhdf5_H5Sclose(dataspace);
	status = vtkhdf5_H5Fclose(file);
	*/
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

	auto group_id_status = H5Gclose(group_id); // close group
	return status;
}


void SaveLoad::saveToFile(const QString& path)
{
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

	if (image->imageType == ImageContainer::DensityImage)
		saveToFile("atest.h5");
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
