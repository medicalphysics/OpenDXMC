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

#include <QCoreApplication>
#include <QDir>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
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
    std::vector<std::uint8_t> data;
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

    if (phantom.type != HMGUPhantom::HMGUType::None) {
        const auto size = phantom.dimensions[0] * phantom.dimensions[1] * phantom.dimensions[2];
        if (data.size() == size + header_size) {
            phantom.data.insert(phantom.data.begin(), data.begin() + header_size, data.end());
        }
    }

    return phantom;
}

struct Media {
    std::uint8_t ID = 0;
    std::map<std::size_t, double> composition;
    std::string name;
};

static std::optional<Media> parseMediaLine(const std::string& line)
{

    Media media;
    auto start = line.data();
    auto end = start + line.size();

    // read ID
    if (auto res = std::from_chars(start, end, media.ID); res.ec != std::errc {}) {
        return std::nullopt;
    } else {
        start = res.ptr;
    }

    auto namestart = start;
    // skipping name
    while (!std::isdigit(*start) && start != end) {
        start++;
    }
    auto nameend = start;
    if (start == end)
        return std::nullopt;

    // atomic weights
    constexpr std::array<std::size_t, 13> Zs = { 1, 6, 7, 8, 11, 12, 15, 16, 17, 19, 20, 26, 53 };
    std::array<double, 13> Ws;
    for (std::size_t i = 0; i < Ws.size(); ++i) {
        while (std::isspace(*start) && start != end) {
            start++;
        }
        if (start == end)
            return std::nullopt;

        auto res = std::from_chars(start, end, Ws[i]);
        start = res.ptr;
        if (res.ec != std::errc {})
            return std::nullopt;
    }

    for (std::size_t i = 0; i < Ws.size(); ++i) {
        if (Ws[i] > 0.0)
            media.composition[Zs[i]] = Ws[i];
    }

    // create name string
    // trimming name
    while (std::isspace(*namestart) && namestart != nameend) {
        namestart++;
    }
    while (std::isspace(*(nameend - 1)) && namestart != nameend) {
        nameend--;
    }
    media.name = std::string(namestart, nameend);

    return std::make_optional(media);
}

static std::vector<Media> readMedia(const std::string& path)
{
    std::ifstream input(path, std::ios::in);
    if (!input.is_open())
        return std::vector<Media> {};

    std::string line;
    std::vector<Media> media;

    // Get the input from the input file until EOF
    while (std::getline(input, line)) {
        auto m_cand = parseMediaLine(line);
        if (m_cand)
            media.push_back(m_cand.value());
    }

    return media;
}

struct Organ {
    double density = 0;
    std::uint8_t ID = 0;
    std::uint8_t materialID = 0;
    std::string name;
};

static std::optional<Organ> parseOrganLine(const std::string& line)
{
    Organ organ;
    // valid line starts with uint8 number
    auto start = line.data();
    auto end = start + line.size();

    // read ID
    if (auto res = std::from_chars(start, end, organ.ID); res.ec != std::errc {}) {
        // first characters was not a number
        return std::nullopt;
    } else {
        start = res.ptr;
    }
    auto namestart = start;
    start += 50; // skipping som numbers in names
    auto nameend = start;
    bool still_name = true;
    while (still_name && start < end) {
        if (auto res = std::from_chars(start, end, organ.materialID); res.ec != std::errc {}) {
            start++;
        } else {
            still_name = false;
            nameend = start;
            start = res.ptr;
        }
    }

    // last we read density
    while (start < end) {
        auto res = std::from_chars(start++, end, organ.density);
        if (res.ec == std::errc {}) // we read a double successfully
            break;
    }
    if (organ.density == 0.0)
        return std::nullopt;
    // trimming name
    while (std::isspace(*namestart) && namestart != nameend) {
        namestart++;
    }
    while (std::isspace(*(nameend - 1)) && namestart != nameend) {
        nameend--;
    }
    if (namestart == nameend)
        return std::nullopt;
    organ.name = std::string(namestart, nameend);
    return std::make_optional(organ);
}

static std::vector<Organ> readOrgans(const std::string& path)
{
    std::ifstream input(path, std::ios::in);
    if (!input.is_open())
        return std::vector<Organ> {};

    std::string line;
    std::vector<Organ> organs;

    // Get the input from the input file until EOF
    while (std::getline(input, line)) {
        auto o_cand = parseOrganLine(line);
        if (o_cand)
            organs.push_back(o_cand.value());
    }
    return organs;
}

void OtherPhantomImportPipeline::importHMGUPhantom(QString path)
{
    emit dataProcessingStarted(ProgressWorkType::Importing);
    auto data = readHMGUFile(path);
    auto phantom = readHMGUheader(data);
    if (phantom.type != HMGUPhantom::HMGUType::None) {
        const auto exeDirPath = QCoreApplication::applicationDirPath();
        QDir phantoms_path(exeDirPath);
        phantoms_path.cd("data");
        phantoms_path.cd("phantoms");
        phantoms_path.cd("other");
        const auto media_path = phantoms_path.absoluteFilePath(QStringLiteral("media.dat")).toStdString();
        auto media = readMedia(media_path);
        const auto organ_path = phantoms_path.absoluteFilePath(QString::fromStdString(phantom.organFile())).toStdString();
        auto organs = readOrgans(organ_path);

        auto container = std::make_shared<DataContainer>();
        container->setDimensions(phantom.dimensions);
        container->setSpacingInmm(phantom.spacing);

        auto& organ_array = phantom.data;
        { // organ pruning
            auto unique_organs = phantom.data;
            std::ranges::sort(unique_organs);
            auto ret = std::ranges::unique(unique_organs);
            unique_organs.erase(ret.begin(), ret.end());
            std::map<std::uint8_t, std::uint8_t> organ_map;
            for (std::uint8_t o = 0; o < unique_organs.size(); ++o)
                organ_map[unique_organs[o]] = o;

            std::for_each(std::execution::par_unseq, organ_array.begin(), organ_array.end(),
                [&organ_map](auto& o) {
                    o = organ_map[o];
                });
            for (auto& organ : organs)
                if (organ_map.contains(organ.ID))
                    organ.ID = organ_map[organ.ID];
                else
                    organ.ID = 255;

            if (organs[0].ID != 0) { // add air
                organs.push_back({ .density = 0.001, .ID = 0, .materialID = 0, .name = "Air" });
            }
            std::sort(organs.begin(), organs.end(), [](auto& lh, auto& rh) { return lh.ID < rh.ID; });
            organs.erase(std::find_if(organs.begin(), organs.end(), [](const auto& o) { return o.ID == 255; }), organs.end());
        }
        { // media pruning
            // add air to media
            media.push_back({ .ID = 0, .composition = NISTMaterials::Composition("Air, Dry (near sea level)"), .name = "Air" });
            std::vector<std::uint8_t> media_idx;
            for (auto& organ : organs) {
                media_idx.push_back(organ.materialID);
            }
            std::ranges::sort(media_idx);
            auto ret = std::ranges::unique(media_idx);
            media_idx.erase(ret.begin(), ret.end());
            std::map<std::uint8_t, std::uint8_t> media_map;
            for (std::uint8_t i = 0; i < media_idx.size(); ++i) {
                media_map[media_idx[i]] = i;
            }
            for (auto& organ : organs) {
                organ.materialID = media_map[organ.materialID];
            }
            for (auto& m : media) {
                if (media_map.contains(m.ID)) {
                    m.ID = media_map[m.ID];
                } else {
                    m.ID = 255;
                }
            }
            std::sort(media.begin(), media.end(), [](auto& lh, auto& rh) { return lh.ID < rh.ID; });
            media.erase(std::find_if(media.begin(), media.end(), [](const auto& o) { return o.ID == 255; }), media.end());
        }

        { // create organ data
            container->setImageArray(DataContainer::ImageType::Organ, organ_array);
            std::vector<std::string> organ_names;
            for (const auto& o : organs)
                organ_names.push_back(o.name);
            container->setOrganNames(organ_names);
        }
        { // create media data
            std::vector<std::uint8_t> media_array(organ_array.size());
            std::map<std::uint8_t, std::uint8_t> map;
            for (const auto& o : organs)
                map[o.ID] = o.materialID;
            std::transform(std::execution::par_unseq, organ_array.cbegin(), organ_array.cend(), media_array.begin(), [&map](const auto o) { return map[o]; });
            container->setImageArray(DataContainer::ImageType::Material, media_array);
            std::vector<DataContainer::Material> materials;
            for (const auto& m : media) {
                materials.push_back({ .name = m.name, .Z = m.composition });
            }
            container->setMaterials(materials);
        }
        { // create density array
            std::vector<double> dens_array(organ_array.size());
            std::map<std::uint8_t, double> map;
            for (const auto& o : organs)
                map[o.ID] = o.density;
            std::transform(std::execution::par_unseq, organ_array.cbegin(), organ_array.cend(), dens_array.begin(), [&map](const auto o) { return map[o]; });
            container->setImageArray(DataContainer::ImageType::Density, dens_array);
        }
        // synthezice ct
        auto ct = container->generateSyntheticCT();
        if (ct)
            container->setImageArray(DataContainer::ImageType::CT, ct.value());

        emit imageDataChanged(container);
    }

    emit dataProcessingFinished(ProgressWorkType::Importing);
}