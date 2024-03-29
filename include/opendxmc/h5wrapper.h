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

#include "H5Cpp.h"

#include <memory>
#include <string>
#include <vector>

#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/imagecontainer.h"

class H5Wrapper {
public:
    enum class FileOpenType { WriteOver,
        ReadOnly };
    H5Wrapper(const std::string& filePath, FileOpenType type = FileOpenType::WriteOver);
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
    bool saveDoubleList(const std::vector<double>& values, const std::string& name, const std::string& groupPath = "");
    std::vector<double> loadDoubleList(const std::string& name, const std::string& groupPath = "");

    std::unique_ptr<H5::Group> saveTube(const Tube& tube, const std::string& name, const std::string& groupPath);
    void loadTube(Tube& tube, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<Source> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<Source> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<CTBaseSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<CTBaseSource> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<CTSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<CTSource> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<DAPSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<DAPSource> src, const std::string& name, const std::string& groupPath);

    bool saveSource(std::shared_ptr<DXSource> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<CBCTSource> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<CTAxialSource> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<CTSpiralSource> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<CTSpiralDualSource> src, const std::string& name, const std::string& groupPath);
    bool saveSource(std::shared_ptr<CTTopogramSource> src, const std::string& name, const std::string& groupPath);

    bool loadSource(std::shared_ptr<DXSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<CBCTSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<CTAxialSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<CTSpiralSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<CTSpiralDualSource> src, const std::string& name, const std::string& groupPath);
    bool loadSource(std::shared_ptr<CTTopogramSource> src, const std::string& name, const std::string& groupPath);

private:
    std::unique_ptr<H5::H5File> m_file = nullptr;
};
