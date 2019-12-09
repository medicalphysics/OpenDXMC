
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


#include "h5wrapper.h"
#include "stringmanipulation.h"

H5Wrapper::H5Wrapper(const std::string& filePath)
{
	try {
		m_file = std::make_unique<H5::H5File>(filePath.c_str(), H5F_ACC_TRUNC);
	}
	catch (...)
	{
		m_file = nullptr;
	}
}


H5Wrapper::~H5Wrapper()
{
}

bool H5Wrapper::saveImage(std::shared_ptr<ImageContainer> image)
{
	std::string arrayPath = "/arrays";
	auto dataset = createDataSet(image, arrayPath);
	if (!dataset)
		return false;

	//writing spacing
	const hsize_t dim3 = 3;
	H5::DataSpace doubleSpace3(1, &dim3);
	auto spacing = dataset->createAttribute("spacing", H5::PredType::NATIVE_DOUBLE, doubleSpace3);
	spacing.write(H5::PredType::NATIVE_DOUBLE, image->image->GetSpacing());
	//writing origin
	auto origin = dataset->createAttribute("origin", H5::PredType::NATIVE_DOUBLE, doubleSpace3);
	origin.write(H5::PredType::NATIVE_DOUBLE, image->image->GetOrigin());
	//writing direction cosines
	const hsize_t dim6 = 6;
	H5::DataSpace doubleSpace6(1, &dim6);
	auto cosines = dataset->createAttribute("direction_cosines", H5::PredType::NATIVE_DOUBLE, doubleSpace6);
	double* cosines_data = image->directionCosines.data();
	cosines.write(H5::PredType::NATIVE_DOUBLE, static_cast<void*>(image->directionCosines.data()));
	//writing data units
	if (image->dataUnits.length() > 0) {
		hsize_t strLen = static_cast<hsize_t>(image->dataUnits.length());
		H5::StrType stringType(0, strLen);
		H5::DataSpace stringSpace(H5S_SCALAR);
		auto dataUnits = dataset->createAttribute("dataUnits", stringType, stringSpace);
		dataUnits.write(stringType, image->dataUnits.c_str());
	}
	return true;
}

std::shared_ptr<ImageContainer> H5Wrapper::loadImage(ImageContainer::ImageType type)
{

	auto image = loadDataSet(type, "/arrays");

	return image;
}

bool H5Wrapper::saveOrganList(const std::vector<std::string>& organList)
{
	return false;
}

std::vector<std::string> H5Wrapper::loadOrganList(void)
{
	return std::vector<std::string>();
}

bool H5Wrapper::saveMaterials(const std::vector<Material>& materials)
{
	return false;
}

std::vector<Material> H5Wrapper::loadMaterials(void)
{
	return std::vector<Material>();
}

bool H5Wrapper::saveSources(const std::vector<std::shared_ptr<Source>>& sources)
{
	return false;
}

std::vector<std::shared_ptr<Source>> H5Wrapper::loadSources(void)
{
	return std::vector<std::shared_ptr<Source>>();
}


std::unique_ptr<H5::Group> H5Wrapper::getGroup(const std::string& groupPath, bool create)
{
	if (!m_file)
		return nullptr;

	auto names = string_split(groupPath, '/');
	std::string fullname;
	std::unique_ptr<H5::Group> group = nullptr;
	for (const auto& name : names) {
		fullname += "/" + name;
		if(!m_file->nameExists(fullname.c_str())) {
			if (create) {
				group = std::make_unique<H5::Group>(m_file->createGroup(fullname.c_str()));
			}
			else {
				return nullptr;
			}
		}
		else {
			group = std::make_unique<H5::Group>(m_file->openGroup(fullname.c_str()));
		}
	}
	return group;
}

std::unique_ptr<H5::DataSet> H5Wrapper::createDataSet(std::shared_ptr<ImageContainer> image, const std::string& groupPath)
{
	/*
	Creates a dataset from image data and writes array
	*/
	if (!m_file)
		return nullptr;
	if (!image)
		return nullptr;
	if (!image->image)
		return nullptr;
	auto name = image->getImageName();

	auto group = getGroup(groupPath, true);
	if (!group)
		return nullptr;
	auto namePath = groupPath + "/" + name;

	constexpr hsize_t rank = 3;
	hsize_t dims[3];
	for (std::size_t i = 0; i < 3; ++i)
		dims[i] = static_cast<hsize_t>(image->image->GetDimensions()[i]);

	auto dataspace = std::make_unique<H5::DataSpace>(rank, dims);
	
	H5::DSetCreatPropList ds_createplist;
	hsize_t cdims[3];
	cdims[0] = dims[0];
	cdims[1] = dims[1];
	cdims[2] = 1;
	ds_createplist.setChunk(rank, cdims);
	ds_createplist.setDeflate(6);

	auto type = H5::PredType::NATIVE_UCHAR;
	if (image->image->GetScalarType() == VTK_DOUBLE)
		type = H5::PredType::NATIVE_DOUBLE;
	else if (image->image->GetScalarType() == VTK_FLOAT)
		type = H5::PredType::NATIVE_FLOAT;
	else if (image->image->GetScalarType() == VTK_UNSIGNED_CHAR)
		type = H5::PredType::NATIVE_UCHAR;
	else
		return nullptr;
	auto dataset = std::make_unique<H5::DataSet>(group->createDataSet(namePath.c_str(), type, *dataspace, ds_createplist));
	dataset->write(image->image->GetScalarPointer(), type);
	return dataset;
}

std::shared_ptr<ImageContainer> H5Wrapper::loadDataSet(ImageContainer::ImageType type, const std::string& groupPath)
{
	if (!m_file)
		return nullptr;
	
	auto group = getGroup(groupPath, false);
	if (!group)
		return nullptr;

	auto path = groupPath + "/" + ImageContainer::getImageName(type);

	std::unique_ptr<H5::DataSet> dataset = nullptr;
	try {
		dataset = std::make_unique<H5::DataSet>(m_file->openDataSet(path.c_str()));
	}
	catch (H5::FileIException notFoundError){
		return nullptr;
	}
	
	auto space = dataset->getSpace();
	auto rank = space.getSimpleExtentNdims();
	if (rank != 3)
		return nullptr;
	hsize_t h5dims[3];
	space.getSimpleExtentDims(h5dims);
	
	std::array<std::size_t, 3> dim{ h5dims[0], h5dims[1], h5dims[2] };
	std::array<double, 3> origin{ 0,0,0 };
	std::array<double, 3> spacing{ 1,1,1 };
	std::array<double, 6> direction{ 1,0,0,0,1,0 };
	std::string units("");

	try {
		auto origin_attr = dataset->openAttribute("origin");
		origin_attr.read(H5T_NATIVE_DOUBLE, origin.data());
		auto spacing_attr = dataset->openAttribute("spacing");
		spacing_attr.read(H5T_NATIVE_DOUBLE, spacing.data());
		auto dir_attr = dataset->openAttribute("direction_cosines");
		dir_attr.read(H5T_NATIVE_DOUBLE, direction.data());
		if (dataset->attrExists("dataUnits"))
		{
			auto units_attr = dataset->openAttribute("dataUnits");
			std::size_t units_size = units_attr.getInMemDataSize();
			units.resize(units_size);
			H5::StrType stringType(0, units_size);
			units_attr.read(stringType, units.data());
		}
	}
	catch (...)
	{
		return nullptr;
	}
	
	
	

	std::shared_ptr<ImageContainer> image = nullptr;

	auto type_class = dataset->getTypeClass();
	auto size = dim[0] * dim[1] * dim[2];
	if (type_class == H5T_FLOAT)
	{
		auto double_type_class = dataset->getFloatType();
		auto type_size = double_type_class.getSize();
		if (type_size = 4)
		{
			auto data = std::make_shared<std::vector<float>>(size);
			float* buffer = data->data();
			dataset->read(buffer, H5::PredType::NATIVE_FLOAT);
			image = std::make_shared<ImageContainer>(type, data, dim, spacing, origin);
		}
		else if (type_size = 8)
		{
			auto data = std::make_shared<std::vector<double>>(size);
			double* buffer = data->data();
			dataset->read(buffer, H5::PredType::NATIVE_DOUBLE);
			image = std::make_shared<ImageContainer>(type, data, dim, spacing, origin);
		}
	}
	else if (type_class == H5T_INTEGER)
	{
		auto int_type_class = dataset->getIntType();
		auto type_size = int_type_class.getSize();
		if (type_size = 1)
		{
			auto data = std::make_shared<std::vector<unsigned char>>(size);
			unsigned char* buffer = data->data();
			dataset->read(buffer, H5::PredType::NATIVE_UCHAR);
			image = std::make_shared<ImageContainer>(type, data, dim, spacing, origin);
		}
	}
	if (!image)
		return nullptr;
	image->directionCosines = direction;
	image->dataUnits = units;
	return image;
}

bool H5Wrapper::saveStringList(const std::vector<std::string>& list, const std::string& name, const std::string& groupPath)
{
	return false;

}
