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

Copyright 2023 Erlend Andersen
*/

#include <icrpphantomimportpipeline.hpp>

#include <charconv>
#include <fstream>
#include <iterator>
#include <string>

ICRPPhantomImportPipeline::ICRPPhantomImportPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void ICRPPhantomImportPipeline::updateImageData(std::shared_ptr<DataContainer>)
{
    // we do nothing
}

void ICRPPhantomImportPipeline::setRemoveArms(bool on)
{
    m_remove_arms = on;
}

std::vector<std::uint8_t> readOrganArray(const std::string& path, const std::array<std::size_t, 3>& dim)
{
    std::ifstream input(path, std::ios::binary | std::ios::in);
    if (!input.is_open())
        return std::vector<std::uint8_t> {};

    // copies all data into buffer
    std::vector<std::uint8_t> organs(std::istreambuf_iterator<char>(input), {});

    // flipping y axis
    std::vector<std::uint8_t> organs_flip(organs.size());
    for (std::size_t z = 0; z < dim[2]; z++)
        for (std::size_t y = 0; y < dim[1]; y++)
            for (std::size_t x = 0; x < dim[0]; x++) {
                const auto fid = z * dim[0] * dim[1] + y * dim[0] + x;
                const auto sid = z * dim[0] * dim[1] + (dim[1] - y - 1) * dim[0] + x;
                organs_flip[sid] = organs[fid];
            }

    return organs_flip;
}

struct Organ {
    double density = 0;
    std::uint8_t ID = 0;
    std::uint8_t materialID = 0;
    std::string name;
};

std::optional<Organ> parseOrganLine(const std::string& line)
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

std::vector<Organ> readOrgans(const std::string& path)
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

struct Media {
    std::uint8_t ID = 0;
    std::map<std::size_t, double> composition;
    std::string name;
};

std::optional<Media> parseMediaLine(const std::string& line)
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

std::vector<Media> readMedia(const std::string& path)
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

void pruneOrganArray(std::vector<std::uint8_t>& organArray, std::vector<Organ>& organs)
{
    std::sort(organs.begin(), organs.end(), [](const auto& lh, const auto& rh) { return lh.ID < rh.ID; });
    std::uint8_t index = 0;
    while (index < organs.size()) {
        // do we have index ID in array?
        auto id = organs[index].ID;
        bool id_exists = organArray.cend() != std::find(std::execution::par_unseq, organArray.cbegin(), organArray.cend(), id);
        if (!id_exists) {
            // organ id do not exists in organ array we delete it
            organs.erase(organs.begin() + index);
        } else {
            index++;
        }
    }
    // now we only have valid organs, lets make organ indexes concecutive
    for (std::uint8_t i = 0; i < organs.size(); ++i) {
        if (i != organs[i].ID) {
            std::replace(std::execution::par_unseq, organArray.begin(), organArray.end(), organs[i].ID, i);
            organs[i].ID = i;
        }
    }
}

void pruneMedia(std::vector<Organ>& organs, std::vector<Media>& media)
{
    std::sort(media.begin(), media.end(), [](const auto& lh, const auto& rh) { return lh.ID < rh.ID; });
    std::uint8_t index = 0;
    while (index < media.size()) {
        bool media_in_organ_list = false;
        for (const auto& o : organs)
            media_in_organ_list = media_in_organ_list || o.materialID == media[index].ID;
        if (media_in_organ_list)
            index++;
        else
            media.erase(media.begin() + index);
    }
    // now we only have media that is in an organ
    // lets make media IDs consecutive
    for (std::uint8_t i = 0; i < media.size(); ++i) {
        if (media[i].ID != i) {
            for (auto& o : organs)
                if (o.materialID == media[i].ID)
                    o.materialID = i;
            media[i].ID = i;
        }
    }
}

void ICRPPhantomImportPipeline::importPhantom(QString organArrayPath, QString organMediaPath, QString mediaPath, double sx, double sy, double sz, int x, int y, int z)
{

    auto container = std::make_shared<DataContainer>();
    const std::array spacing_mm = { sx, sy, sz };
    const std::array dimensions = { static_cast<std::size_t>(x), static_cast<std::size_t>(y), static_cast<std::size_t>(z) };
    container->setDimensions(dimensions);
    container->setSpacingInmm(spacing_mm);

    auto organArray = readOrganArray(organArrayPath.toStdString(), container->dimensions());

    auto organs = readOrgans(organMediaPath.toStdString());
    if (organs.size() == 0)
        return;
    organs.push_back({ .density = 0.001, .ID = 0, .materialID = 0, .name = "Air" });
    if (m_remove_arms) {
        for (auto& organ : organs) {
            // find arm string
            auto armPos = organ.name.find("arm");
            if (armPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray.begin(), organArray.end(), organ.ID, std::uint8_t { 0 });
            }
            auto handPos = organ.name.find("hand");
            if (handPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray.begin(), organArray.end(), organ.ID, std::uint8_t { 0 });
            }
            auto humeriPos = organ.name.find("Humeri");
            if (humeriPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray.begin(), organArray.end(), organ.ID, std::uint8_t { 0 });
            }
            auto ulnaRadiiPos = organ.name.find("Ulnae");
            if (ulnaRadiiPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray.begin(), organArray.end(), organ.ID, std::uint8_t { 0 });
            }
        }
    }
    pruneOrganArray(organArray, organs);

    auto media = readMedia(mediaPath.toStdString());
    if (media.size() == 0)
        return;
    media.push_back({ .ID = 0, .composition = { { 7, 0.8 }, { 8, 0.20 } }, .name = "Air" });
    pruneMedia(organs, media);

    auto success = container->setImageArray(DataContainer::ImageType::Organ, organArray);
    {
        std::vector<std::string> organNames;
        organNames.reserve(organs.size());
        for (const auto& o : organs)
            organNames.push_back(o.name);
        container->setOrganNames(organNames);
    }

    if (!success) {
        emit errorMessage(tr("Could not read organ array"));
    }

    std::vector<std::uint8_t> mediaArray(organArray.size());

    {
        std::unordered_map<std::uint8_t, std::uint8_t> organTomedia;
        for (const auto& o : organs) {
            organTomedia[o.ID] = o.materialID;
        }
        std::transform(std::execution::par_unseq, organArray.cbegin(), organArray.cend(), mediaArray.begin(), [&](const auto oId) { 
            if(!organTomedia.contains(oId))
                return std::uint8_t{0};            
            return organTomedia.at(oId); });
    }
    container->setImageArray(DataContainer::ImageType::Material, mediaArray);
    std::vector<double> densityArray(organArray.size());
    {
        std::unordered_map<std::uint8_t, double> organTodens;
        for (const auto& o : organs) {
            organTodens[o.ID] = o.density;
        }
        std::transform(std::execution::par_unseq, organArray.cbegin(), organArray.cend(), densityArray.begin(), [&](const auto oId) { 
            if(!organTodens.contains(oId))
                return 0.0;
            return organTodens.at(oId); });
    }
    container->setImageArray(DataContainer::ImageType::Density, densityArray);

    {
        std::vector<DataContainer::Material> mats;
        for (const auto& m : media) {
            mats.push_back({ .name = m.name, .Z = m.composition });
        }
        container->setMaterials(mats);
    }
    emit imageDataChanged(container);
}
