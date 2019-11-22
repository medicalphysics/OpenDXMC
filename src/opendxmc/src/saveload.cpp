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

#include "imagecontainer.h"
#include "source.h"
#include <hdf5.h>
#include <vtk_hdf5.h>


SaveLoad::SaveLoad(QObject* parent)
	:QObject(parent)
{
	auto file_id = vtkhdf5_H5Fcreate("test.h5", H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
	std::vector<double> testArr(512 * 512 * 5, 0);
	std::array<hsize_t, 3> dims={ 512, 512, 5 };
	auto dataspace_id = vtkhdf5_H5Screate_simple(2, dims.data(), NULL);

	auto dataset_id = vtkhdf5_H5Dcreate2(file_id, "/arrays/test", H5T_STD_U64LE, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);

	/* Write the first dataset. */
	auto status = vtkhdf5_H5Dwrite(dataset_id, H5T_NATIVE_INT, H5S_ALL, H5S_ALL, H5P_DEFAULT,
		testArr.data());

	/* Close the data space for the first dataset. */
	status = H5Sclose(dataspace_id);

	/* Close the first dataset. */
	status = H5Dclose(dataset_id);
	auto status = vtkhdf5_H5Fclose(file_id);
}

void SaveLoad::saveToFile(const QString& path)
{
	
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
