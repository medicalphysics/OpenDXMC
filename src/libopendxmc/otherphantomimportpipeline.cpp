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

Copyright 2024 Erlend Andersen
*/

#include "dxmc/material/nistmaterials.hpp"
#include <otherphantomimportpipeline.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

OtherPhantomImportPipeline::OtherPhantomImportPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void OtherPhantomImportPipeline::updateImageData(std::shared_ptr<DataContainer>)
{
    // we do nothing
}

std::vector<std::uint8_t> generateCylinder(const std::array<std::size_t, 3>& dim)
{
    std::vector<std::uint8_t> cyl(dim[0] * dim[1] * dim[2]);

    const auto cx = dim[0] / 2.0;
    const auto cy = dim[1] / 2.0;
    const auto r = std::min(cx, cy);
    const auto r2 = r * r;

    for (std::size_t k = 0; k < dim[2]; ++k)
        for (std::size_t j = 0; j < dim[1]; ++j)
            for (std::size_t i = 0; i < dim[0]; ++i) {
                const auto ind = i + j * dim[0] + k * dim[0] * dim[1];
                const auto x = i - cx;
                const auto y = j - cy;
                cyl[ind] = x * x + y * y <= r2 ? 1 : 0;
            }
    return cyl;
}

std::vector<std::uint8_t> generateCube(const std::array<std::size_t, 3>& dim)
{
    std::vector<std::uint8_t> cube(dim[0] * dim[1] * dim[2], std::uint8_t { 1 });
    return cube;
}

void OtherPhantomImportPipeline::importPhantom(int type, double dx, double dy, double dz, int nx, int ny, int nz)
{
    emit dataProcessingStarted(ProgressWorkType::Importing);
    auto vol = std::make_shared<DataContainer>();

    const std::array<std::size_t, 3> dims = {
        static_cast<std::size_t>(nx),
        static_cast<std::size_t>(ny),
        static_cast<std::size_t>(nz)
    };
    const auto N = dims[0] * dims[1] * dims[2];
    vol->setDimensions(dims);
    vol->setSpacing({ dx, dy, dz });

    std::vector<std::uint8_t> mat;
    if (type == 0)
        mat = generateCylinder(dims);
    else
        mat = generateCube(dims);
    vol->setImageArray(DataContainer::ImageType::Material, mat);
    vol->setImageArray(DataContainer::ImageType::Organ, mat);

    std::vector<DataContainer::Material> materials;
    std::vector<std::string> organ_names;
    organ_names.push_back("Air, Dry (near sea level)");
    organ_names.push_back("Polymethyl Methacralate (Lucite, Perspex)");
    vol->setOrganNames(organ_names);

    for (const auto& n : organ_names) {
        auto Z = dxmc::NISTMaterials::Composition(n);
        materials.push_back({ .name = n, .Z = Z });
    }
    vol->setMaterials(materials);

    const double air_dens = dxmc::NISTMaterials::density(organ_names[0]);
    const double pmma_dens = dxmc::NISTMaterials::density(organ_names[1]);
    std::vector<double> dens(N);
    std::transform(std::execution::par_unseq, mat.cbegin(), mat.cend(), dens.begin(), [air_dens, pmma_dens](const auto m) {
        return m == 1 ? pmma_dens : air_dens;
    });
    vol->setImageArray(DataContainer::ImageType::Density, dens);

    emit imageDataChanged(vol);
    emit dataProcessingFinished(ProgressWorkType::Importing);
}

struct HMGUPhantom {
    enum class HMGUType {
        None,
        Katja,
        Golem,
        Helga,
        Irene,
        Frank,
        Child,
        Jo,
        Baby,
        Vishum,
        Donna
    };

    HMGUType type = HMGUType::None;
    std::array<double, 3> spacing = { 1, 1, 1 };
    std::array<std::size_t, 3> dimensions = { 1, 1, 1 };

    std::string organFile()
    {
        switch (type) {
        case HMGUPhantom::HMGUType::Katja:
            return "Katja_organs.dat";
        case HMGUPhantom::HMGUType::Golem:
            return "Golem_organs.dat";
        case HMGUPhantom::HMGUType::Helga:
            return "Helga_organs.dat";
        case HMGUPhantom::HMGUType::Irene:
            return "Irene_organs.dat";
        case HMGUPhantom::HMGUType::Frank:
            return "Frank_organs.dat";
        case HMGUPhantom::HMGUType::Child:
            return "Child_organs.dat";
        case HMGUPhantom::HMGUType::Jo:
            return "Jo_organs.dat";
        case HMGUPhantom::HMGUType::Baby:
            return "Baby_organs.dat";
        case HMGUPhantom::HMGUType::Vishum:
            return "Vishum_organs.dat";
        case HMGUPhantom::HMGUType::Donna:
            return "Donna_organs.dat";
        default:
            return std::string {};
        }
    }
};

std::vector<char> readHMGUFile(const QString& path)
{
    std::filesystem::path filePath = path.toStdString();
    if (std::filesystem::exists(filePath)) {
        std::ifstream file(filePath, std::ios::binary);
        if (file.is_open()) {
            std::vector<char> data(std::istreambuf_iterator<char>(file), {});
            file.close();
            return data;
        }
    }
    return std::vector<char> {};
}

HMGUPhantom readHMGUheader(const std::vector<char>& data)
{
    constexpr int header_size = 4096;
    HMGUPhantom phantom;
    if (data.size() < header_size)
        return phantom;

    std::string header(data.begin(), data.begin() + header_size);

    std::array<std::string, 3> dim_tokens = { " Width=", " Height=", " Depth=" };
    std::array<std::string, 3> space_tokens = { "VoxelWidth=", "VoxelHeight=", "VoxelDepth=" };

    for (std::size_t i = 0; i < 3; ++i) {
        // dimensions
        {
            auto& token = dim_tokens[i];
            auto pos = header.find(token);
            if (pos == std::string::npos)
                return phantom;
            if (std::from_chars(header.data() + pos + token.size(), header.data() + header.size(), phantom.dimensions[i]).ec != std::errc {})
                return phantom;
        }
        // spacing
        {
            auto& token = space_tokens[i];
            auto pos = header.find(token);
            if (pos == std::string::npos)
                return phantom;
            if (std::from_chars(header.data() + pos + token.size(), header.data() + header.size(), phantom.spacing[i]).ec != std::errc {})
                return phantom;
        }
    }
    // assign phantom type
    if (phantom.dimensions[0] == 299 && phantom.dimensions[1] == 150 && phantom.dimensions[2] == 348) {
        phantom.type = HMGUPhantom::HMGUType::Katja;
    } else if (phantom.dimensions[0] == 226 && phantom.dimensions[1] == 118 && phantom.dimensions[2] == 136) {
        phantom.type = HMGUPhantom::HMGUType::Jo;
    } else if (phantom.dimensions[0] == 267 && phantom.dimensions[1] == 138 && phantom.dimensions[2] == 142) {
        phantom.type = HMGUPhantom::HMGUType::Baby;
    } else if (phantom.dimensions[0] == 256 && phantom.dimensions[1] == 256 && phantom.dimensions[2] == 144) {
        phantom.type = HMGUPhantom::HMGUType::Child;
    } else if (phantom.dimensions[0] == 256 && phantom.dimensions[1] == 256 && phantom.dimensions[2] == 179) {
        phantom.type = HMGUPhantom::HMGUType::Donna;
    } else if (phantom.dimensions[0] == 512 && phantom.dimensions[1] == 512 && phantom.dimensions[2] == 193) {
        phantom.type = HMGUPhantom::HMGUType::Frank;
    } else if (phantom.dimensions[0] == 256 && phantom.dimensions[1] == 256 && phantom.dimensions[2] == 220) {
        phantom.type = HMGUPhantom::HMGUType::Golem;
    } else if (phantom.dimensions[0] == 512 && phantom.dimensions[1] == 512 && phantom.dimensions[2] == 114) {
        phantom.type = HMGUPhantom::HMGUType::Helga;
    } else if (phantom.dimensions[0] == 262 && phantom.dimensions[1] == 132 && phantom.dimensions[2] == 348) {
        phantom.type = HMGUPhantom::HMGUType::Irene;
    } else if (phantom.dimensions[0] == 512 && phantom.dimensions[1] == 512 && phantom.dimensions[2] == 250) {
        phantom.type = HMGUPhantom::HMGUType::Vishum;
    }
    return phantom;
}

void OtherPhantomImportPipeline::importHMGUPhantom(QString path)
{
    emit dataProcessingStarted(ProgressWorkType::Importing);
    auto data = readHMGUFile(path);
    auto phantom = readHMGUheader(data);
    if (phantom.type != HMGUPhantom::HMGUType::None) {
        // read media
        // read phantom organs
        // generate arrays
        // generate materials
        // generate organs
        // generate ct image
    }

    // emit imageDataChanged(vol);

    emit dataProcessingFinished(ProgressWorkType::Importing);
}