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

#include "imagecontainer.h"
#include "material.h"
#include "source.h"

#include "H5Cpp.h"
#include <vector>
#include <string>
#include <memory>

class H5Wrapper
{
public:
	H5Wrapper(const std::string& filePath);
	~H5Wrapper();
	bool saveImage(std::shared_ptr<ImageContainer> image);
	std::shared_ptr<ImageContainer> loadImage(ImageContainer::ImageType type);
	bool saveOrganList(const std::vector<std::string>& organList);
	std::vector<std::string> loadOrganList(void);
	bool saveMaterials(const std::vector<Material>& materials);
	std::vector<Material> loadMaterials(void);
	bool saveSources(const std::vector<std::shared_ptr<Source>>& sources);
	std::vector<std::shared_ptr<Source>> loadSources(void);
protected:
	std::unique_ptr<H5::Group> getGroup(const std::string& path, bool create = true);
	std::unique_ptr<H5::DataSet> createDataSet(std::shared_ptr<ImageContainer> image, const std::string& groupPath);
	std::shared_ptr<ImageContainer> loadDataSet(ImageContainer::ImageType type, const std::string& groupPath);
	bool saveStringList(const std::vector<std::string>& list, const std::string& name, const std::string& groupPath = "");
	std::vector<std::string> loadStringList(const std::string& name, const std::string& groupPath = "");
private:
	std::unique_ptr<H5::H5File> m_file = nullptr;

};