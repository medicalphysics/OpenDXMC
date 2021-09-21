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

Copyright 2020 Erlend Andersen
*/

#include "opendxmc/phantomimportpipeline.h"
#include "opendxmc/stringmanipulation.h"

#include <algorithm>
#include <numeric>
#include <regex>
#include <stdexcept>

PhantomImportPipeline::PhantomImportPipeline(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
    qRegisterMetaType<std::vector<Material>>();
    qRegisterMetaType<std::vector<std::string>>();
    qRegisterMetaType<std::vector<PhantomImportPipeline::Phantom>>();
}

std::array<double, 3> PhantomImportPipeline::icrpSpacing(Phantom phantom)
{
    std::array<double, 3> arr = { 0, 0, 0 };
    switch (phantom) {
    case Phantom::IcrpAdultMale:
        arr = { 2.137, 2.137, 8 };
        break;
    case Phantom::IcrpAdultFemale:
        arr = { 1.775, 1.775, 4.84 };
        break;
    case Phantom::Icrp15YrMale:
        arr = { 1.25, 1.25, 2.832 };
        break;
    case Phantom::Icrp15YrFemale:
        arr = { 1.2, 1.2, 2.828 };
        break;
    case Phantom::Icrp10YrMale:
        arr = { .99, .99, 2.425 };
        break;
    case Phantom::Icrp10YrFemale:
        arr = { .99, .99, 2.425 };
        break;
    case Phantom::Icrp5YrMale:
        arr = { .85, .85, 1.928 };
        break;
    case Phantom::Icrp5YrFemale:
        arr = { .85, .85, 1.928 };
        break;
    case Phantom::Icrp1YrMale:
        arr = { .663, .633, 1.4 };
        break;
    case Phantom::Icrp1YrFemale:
        arr = { .663, .633, 1.4 };
        break;
    case Phantom::Icrp0YrMale:
        arr = { .663, .663, .663 };
        break;
    case Phantom::Icrp0YrFemale:
        arr = { .663, .663, .663 };
        break;
    }
    return arr;
}
std::array<std::size_t, 3> PhantomImportPipeline::icrpDimensions(Phantom phantom)
{
    std::array<std::size_t, 3> arr = { 0, 0, 0 };
    switch (phantom) {
    case Phantom::IcrpAdultMale:
        arr = { 254, 127, 222 };
        break;
    case Phantom::IcrpAdultFemale:
        arr = { 299, 137, 348 };
        break;
    case Phantom::Icrp15YrMale:
        arr = { 407, 225, 586 };
        break;
    case Phantom::Icrp15YrFemale:
        arr = { 401, 236, 571 };
        break;
    case Phantom::Icrp10YrMale:
        arr = { 419, 226, 576 };
        break;
    case Phantom::Icrp10YrFemale:
        arr = { 419, 226, 576 };
        break;
    case Phantom::Icrp5YrMale:
        arr = { 419, 230, 572 };
        break;
    case Phantom::Icrp5YrFemale:
        arr = { 419, 230, 572 };
        break;
    case Phantom::Icrp1YrMale:
        arr = { 393, 248, 546 };
        break;
    case Phantom::Icrp1YrFemale:
        arr = { 393, 248, 546 };
        break;
    case Phantom::Icrp0YrMale:
        arr = { 345, 211, 716 };
        break;
    case Phantom::Icrp0YrFemale:
        arr = { 345, 211, 716 };
        break;
    }
    return arr;
}

std::string PhantomImportPipeline::icrpFolderPath(Phantom phantom)
{
    std::string path;
    switch (phantom) {
    case Phantom::IcrpAdultMale:
        path = "resources/phantoms/icrp/AM/AM_";
        break;
    case Phantom::IcrpAdultFemale:
        path = "resources/phantoms/icrp/AF/AF_";
        break;
    case Phantom::Icrp15YrMale:
        path = "resources/phantoms/icrp/15M/15M_";
        break;
    case Phantom::Icrp15YrFemale:
        path = "resources/phantoms/icrp/15F/15F_";
        break;
    case Phantom::Icrp10YrMale:
        path = "resources/phantoms/icrp/10M/10M_";
        break;
    case Phantom::Icrp10YrFemale:
        path = "resources/phantoms/icrp/10F/10F_";
        break;
    case Phantom::Icrp5YrMale:
        path = "resources/phantoms/icrp/05M/05M_";
        break;
    case Phantom::Icrp5YrFemale:
        path = "resources/phantoms/icrp/05F/05F_";
        break;
    case Phantom::Icrp1YrMale:
        path = "resources/phantoms/icrp/01M/01M_";
        break;
    case Phantom::Icrp1YrFemale:
        path = "resources/phantoms/icrp/01F/01F_";
        break;
    case Phantom::Icrp0YrMale:
        path = "resources/phantoms/icrp/00M/00M_";
        break;
    case Phantom::Icrp0YrFemale:
        path = "resources/phantoms/icrp/00F/00F_";
        break;
    }
    return path;
}

struct OrganElement {
    std::uint8_t ID = 0;
    std::uint8_t medium = 0;
    double density = 0;
    std::string name;
};

struct PhantomData {
    std::shared_ptr<std::vector<floating>> densityArray = nullptr;
    std::shared_ptr<std::vector<std::uint8_t>> materialArray = nullptr;
    std::shared_ptr<std::vector<std::uint8_t>> organArray = nullptr;
    std::vector<std::string> organNames;
    std::vector<dxmc::Material> materials;
};

std::vector<OrganElement> readICRPOrgans(const std::string& path)
{
    std::ifstream input(path);
    if (!input.is_open())
        return std::vector<OrganElement>();

    //reading organs
    std::vector<OrganElement> organs;

    // adding 0 air organ
    struct OrganElement airElement;
    airElement.ID = 0;
    airElement.medium = 0;
    Material airMaterial("Air, Dry (near sea level)");
    airElement.density = airMaterial.standardDensity();
    airElement.name = airMaterial.name();
    organs.push_back(airElement);

    //regex find
    std::string regex_string = "([[:digit:]]+)[[:space:]]+([0-9a-zA-Z, \\(\\)]+)[[:space:]]+([[:digit:]]+)[[:space:]]+([[:digit:]]+\\.[[:digit:]]+)";

    const std::regex regex(regex_string);
    const std::size_t required_matches = 4;

    for (std::string line; getline(input, line);) {
        std::smatch regex_result;
        auto test = std::regex_search(line, regex_result, regex);
        if (std::regex_search(line, regex_result, regex)) {
            if (regex_result.size() >= required_matches + 1) {
                std::string id = regex_result[1];
                std::string name = regex_result[2];
                std::string medium = regex_result[3];
                std::string dens = regex_result[4];

                bool valid = true;
                OrganElement o;
                try {
                    o.ID = static_cast<std::uint8_t>(std::stoi(id));
                    o.medium = static_cast<std::uint8_t>(std::stoi(medium));
                    o.density = std::stod(dens);
                } catch (const std::invalid_argument& e) {
                    valid = false;
                } catch (const std::out_of_range& e) {
                    valid = false;
                }
                if (valid) {
                    o.name = string_trim(name);
                    organs.push_back(o);
                }
            }
        }
    }
    std::sort(organs.begin(), organs.end(), [](auto& left, auto& right) {
        return left.ID < right.ID;
    });
    //adding an air organ at the end because thats just how ICRP rolls
    /* airElement.ID = organs.back().ID + 1;
    organs.push_back(airElement);
    */

    return organs;
}

std::vector<std::pair<std::uint8_t, Material>> readICRPMedia(const std::string& path)
{
    std::ifstream input(path);
    if (!input.is_open())
        return std::vector<std::pair<std::uint8_t, Material>>();
    std::size_t teller = 0;

    //reading media
    std::vector<std::pair<std::uint8_t, Material>> media;
    media.push_back(std::make_pair(0, Material("Air, Dry (near sea level)")));

    std::vector<int> atomicNumbers;
    std::vector<double> massFractions;

    std::array<std::string, 13> elements = { "H", "C", "N", "O", "Na", "Mg", "P", "S", "Cl", "K", "Ca", "Fe", "I" };
    std::array<int, elements.size()> elementsZ;
    std::transform(elements.cbegin(), elements.cend(), elementsZ.begin(), [](const auto& s) { return Material::getAtomicNumberFromSymbol(s); });

    //regex find
    std::string regex_string = "([[:digit:]]+)[[:space:]]+([a-zA-Z,.& \\(\\)\\-]+)[[:space:]]+";
    for (std::size_t i = 0; i < elements.size(); ++i) {
        regex_string += "([[:digit:]]+\\.[[:digit:]]+)[[:space:]]*";
    }
    const std::regex regex(regex_string);
    const std::size_t required_matches = 2 + elements.size();

    for (std::string line; getline(input, line);) {

        std::smatch regex_result;
        if (std::regex_search(line, regex_result, regex)) {
            if (regex_result.size() >= required_matches + 1) {

                const auto mediaNumber = static_cast<std::uint8_t>(std::stoi(regex_result[1]));
                const std::string pretty_name = string_trim(regex_result[2]);
                std::string compound;
                bool validMaterial = true;
                try {

                    for (std::size_t i = 3; i < regex_result.size(); ++i) {
                        const std::size_t index = i - 3;
                        const double massFraction = std::stod(regex_result[i]);
                        if (massFraction > 0) {
                            const double atomicWeight = Material::getAtomicWeight(elementsZ[index]);
                            const double numberFraction = massFraction / atomicWeight;
                            compound += elements[index] + std::to_string(numberFraction);
                        }
                    }
                } catch (const std::exception& e) {
                    validMaterial = false;
                }
                if (validMaterial) {
                    media.push_back(std::make_pair(mediaNumber, Material(compound, pretty_name)));
                    media.back().second.setStandardDensity(1.0);
                }
            }
        }
    }

    // sorting
    std::sort(media.begin(), media.end(), [](auto& left, auto& right) {
        return left.first < right.first;
    });
    return media;
}

std::shared_ptr<std::vector<std::uint8_t>> readICRPData(const std::string& path, std::size_t size)
{
    auto organs = std::make_shared<std::vector<std::uint8_t>>();
    std::ifstream input(path, std::ios::binary | std::ios::in);

    if (!input.is_open())
        return organs;
    input.unsetf(std::ios::skipws);
    organs->reserve(size);
    organs->insert(organs->begin(),
        std::istream_iterator<std::uint8_t>(input),
        std::istream_iterator<std::uint8_t>());
    return organs;
}

PhantomData generateICRUPhantomArrays(
    std::shared_ptr<std::vector<std::uint8_t>> organArray,
    std::vector<OrganElement>& organs,
    const std::vector<std::pair<std::uint8_t, Material>>& materials)
{
    PhantomData data;
    data.organArray = std::make_shared<std::vector<std::uint8_t>>(organArray->size(), 0);
    data.densityArray = std::make_shared<std::vector<floating>>(organArray->size(), 0);
    data.materialArray = std::make_shared<std::vector<std::uint8_t>>(organArray->size(), 0);

    //unique organs
    std::vector<std::uint8_t> uniqueOrgans(organArray->cbegin(), organArray->cend());
    std::sort(std::execution::par_unseq, uniqueOrgans.begin(), uniqueOrgans.end());
    auto last = std::unique(std::execution::par_unseq, uniqueOrgans.begin(), uniqueOrgans.end());
    uniqueOrgans.erase(last, uniqueOrgans.end());

    //do we have all organs?
    //if not set organ as air
    for (std::size_t i = 0; i < uniqueOrgans.size(); ++i) {
        const auto oIdx = uniqueOrgans[i];
        bool exists = false;
        for (const auto& o : organs) {
            exists = exists || o.ID == oIdx;
        }
        if (!exists) {
            uniqueOrgans[i] = 0;
        }
    }
    std::sort(std::execution::par_unseq, uniqueOrgans.begin(), uniqueOrgans.end());
    last = std::unique(std::execution::par_unseq, uniqueOrgans.begin(), uniqueOrgans.end());
    uniqueOrgans.erase(last, uniqueOrgans.end());

    //do we have all materials, if not set material to air
    for (auto& o : organs) {
        bool exists = false;
        for (const auto& [mId, m] : materials) {
            exists = exists || mId == o.medium;
        }
        if (!exists) {
            o.medium = 0;
        }
    }

    std::map<std::uint8_t, std::uint8_t> organLut;
    std::map<std::uint8_t, std::uint8_t> organLutInv;
    for (std::uint8_t i = 0; i < uniqueOrgans.size(); ++i) {
        organLut[i] = uniqueOrgans[i];
        organLutInv[uniqueOrgans[i]] = i;
    }
    std::transform(std::execution::par_unseq, organArray->cbegin(), organArray->cend(), data.organArray->begin(), [&](const auto o) { return organLutInv[o]; });
    data.organNames.resize(uniqueOrgans.size());
    for (const auto& [key, value] : organLutInv) {
        for (const auto& o : organs) {
            if (o.ID == key) {
                data.organNames[value] = o.name;
            }
        }
    }

    std::map<std::uint8_t, std::uint8_t> mediumMap;
    for (const auto& [key, value] : organLutInv) {
        for (const auto& o : organs) {
            if (o.ID == key) {
                mediumMap[key] = o.medium;
            }
        }
    }
    std::vector<std::uint8_t> uniqueMediums;
    for (const auto [key, medium] : mediumMap) {
        uniqueMediums.push_back(medium);
    }
    std::sort(std::execution::par_unseq, uniqueMediums.begin(), uniqueMediums.end());
    last = std::unique(std::execution::par_unseq, uniqueMediums.begin(), uniqueMediums.end());
    uniqueMediums.erase(last, uniqueMediums.end());
    data.materials.resize(uniqueMediums.size());
    std::map<std::uint8_t, std::uint8_t> materialLutInv;
    for (std::uint8_t i = 0; i < uniqueMediums.size(); ++i) {
        for (const auto& [medium, mat] : materials) {
            if (medium == uniqueMediums[i])
                data.materials[i] = mat;
        }
        for (const auto [key, value] : mediumMap) {
            if (value == uniqueMediums[i]) {
                materialLutInv[key] = i;
            }
        }
    }
    std::transform(std::execution::par_unseq, organArray->cbegin(), organArray->cend(), data.materialArray->begin(), [&](const auto o) { return materialLutInv[o]; });

    std::map<std::uint8_t, floating> densLutInv;
    for (const auto& [key, value] : organLutInv) {
        for (const auto& o : organs) {
            if (o.ID == key) {
                densLutInv[key] = o.density;
            }
        }
    }
    std::transform(std::execution::par_unseq, organArray->cbegin(), organArray->cend(), data.densityArray->begin(), [&](const auto o) { return densLutInv[o]; });

    return data;
}
std::pair<std::shared_ptr<std::vector<std::uint8_t>>, std::shared_ptr<std::vector<floating>>> generateICRUPhantomArrays_old(
    std::shared_ptr<std::vector<std::uint8_t>> organArray,
    const std::vector<OrganElement>& organs,
    const std::vector<Material>& materials)
{

    //max organ ID
    std::uint8_t maxID = 0;
    for (const auto& organ : organs) {
        maxID = std::max(organ.ID, maxID);
    }

    std::vector<floating> densLut(maxID + 1, 1.0);
    std::vector<std::uint8_t> matLut(maxID + 1, 0);
    for (const auto& organ : organs) {
        matLut[organ.ID] = static_cast<std::uint8_t>(organ.medium);
        densLut[organ.ID] = static_cast<floating>(organ.density);
    }

    auto materialArray = std::make_shared<std::vector<std::uint8_t>>(organArray->size());
    auto densityArray = std::make_shared<std::vector<floating>>(organArray->size());

    std::transform(std::execution::par_unseq, organArray->cbegin(), organArray->cend(), materialArray->begin(),
        [&](const auto organ) { return organ > maxID ? matLut[0] : matLut[organ]; });
    std::transform(std::execution::par_unseq, organArray->cbegin(), organArray->cend(), densityArray->begin(),
        [&](const auto organ) { return organ > maxID ? densLut[0] : densLut[organ]; });

    return std::make_pair(materialArray, densityArray);
}

std::vector<OrganElement> sortICRUOrgans(std::shared_ptr<std::vector<std::uint8_t>> organArray, const std::vector<OrganElement>& organs)
{
    std::vector<std::uint8_t> uniqueOrgans(organArray->cbegin(), organArray->cend());
    std::sort(std::execution::par_unseq, uniqueOrgans.begin(), uniqueOrgans.end());
    auto last = std::unique(std::execution::par_unseq, uniqueOrgans.begin(), uniqueOrgans.end());
    uniqueOrgans.erase(last, uniqueOrgans.end());

    std::map<std::uint8_t, std::uint8_t> lutInv;
    for (std::size_t i = 0; i < uniqueOrgans.size(); ++i) {
        lutInv[i] = uniqueOrgans[i];
    }
    std::map<std::uint8_t, std::uint8_t> lut;
    for (auto [key, value] : lutInv) {
        lut[value] = key;
    }

    std::transform(std::execution::par_unseq, organArray->cbegin(), organArray->cend(), organArray->begin(), [&](const auto o) { return lut.at(o); });

    std::vector<OrganElement> newOrgans;
    for (const auto& organ : organs) {
        if (lut.count(organ.ID) > 0) {
            newOrgans.push_back(organ);
            newOrgans.back().ID = lut.at(organ.ID);
            lut.extract(organ.ID);
        }
    }

    return newOrgans;
}

std::vector<Material> sortICRUMaterials(std::vector<OrganElement>& organs, const std::vector<std::pair<std::size_t, Material>>& mediums)
{
    std::vector<std::size_t> mediumIdx;
    for (const auto& organ : organs) {
        mediumIdx.push_back(organ.medium);
    }
    //remove duplicates
    std::sort(mediumIdx.begin(), mediumIdx.end());
    auto last = std::unique(mediumIdx.begin(), mediumIdx.end());
    mediumIdx.erase(last, mediumIdx.end());

    for (auto& organ : organs) {
        for (std::size_t i = 0; i < mediumIdx.size(); ++i) {
            if (mediumIdx[i] == organ.medium) {
                organ.medium = i;
                break;
            }
        }
    }

    std::vector<Material> materials(mediumIdx.size());
    for (const auto& [medium, mat] : mediums) {
        for (std::size_t i = 0; i < mediumIdx.size(); ++i) {
            if (medium == mediumIdx[i]) {
                materials[i] = mat;
                break;
            }
        }
    }

    return materials;
}

void PhantomImportPipeline::importICRUPhantom(PhantomImportPipeline::Phantom phantom, bool ignoreArms)
{
    emit processingDataStarted();
    auto spacing = icrpSpacing(phantom);
    auto dimensions = icrpDimensions(phantom);
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);
    auto size = dimensions[0] * dimensions[1] * dimensions[2];

    auto organPath = icrpFolderPath(phantom) + "organs.dat";
    auto mediaPath = icrpFolderPath(phantom) + "media.dat";
    auto arrayPath = icrpFolderPath(phantom) + "binary.dat";

    auto organs = readICRPOrgans(organPath);
    auto media = readICRPMedia(mediaPath);

    auto organArray = readICRPData(arrayPath, size);

    if (ignoreArms) {
        for (auto& organ : organs) {
            //find arm string
            auto armPos = organ.name.find("arm");
            if (armPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<std::uint8_t>(organ.ID), static_cast<std::uint8_t>(0));
            }
            auto handPos = organ.name.find("hand");
            if (handPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<std::uint8_t>(organ.ID), static_cast<std::uint8_t>(0));
            }
            auto humeriPos = organ.name.find("Humeri");
            if (humeriPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<std::uint8_t>(organ.ID), static_cast<std::uint8_t>(0));
            }
            auto ulnaRadiiPos = organ.name.find("Ulnae");
            if (ulnaRadiiPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<std::uint8_t>(organ.ID), static_cast<std::uint8_t>(0));
            }
        }
    }

    //organs = sortICRUOrgans(organArray, organs);
    //auto materials = sortICRUMaterials(organs, media);
    auto data = generateICRUPhantomArrays(organArray, organs, media);

    auto organImage = std::make_shared<OrganImageContainer>(data.organArray, dimensions, spacing, origin);
    auto materialImage = std::make_shared<MaterialImageContainer>(data.materialArray, dimensions, spacing, origin);
    auto densityImage = std::make_shared<DensityImageContainer>(data.densityArray, dimensions, spacing, origin);
    organImage->ID = ImageContainer::generateID();
    materialImage->ID = organImage->ID;
    densityImage->ID = organImage->ID;

    emit processingDataEnded();
    emit materialDataChanged(data.materials);
    emit organDataChanged(data.organNames);
    emit imageDataChanged(organImage);
    emit imageDataChanged(densityImage);
    emit imageDataChanged(materialImage);
}

struct AWSImageData {
    std::array<std::size_t, 3> dimensions = { 0, 0, 0 };
    std::array<double, 3> spacing = { 0, 0, 0 };
    std::array<double, 6> cosines = { 1, 0, 0, 0, 1, 0 };
    std::shared_ptr<std::vector<std::uint8_t>> image = nullptr;
};

AWSImageData readAWSData(const std::string& path)
{
    std::array<std::size_t, 3> dimensions = { 0, 0, 0 };
    std::array<double, 3> spacing = { 0, 0, 0 };
    std::array<double, 6> cosines = { 1, 0, 0, 0, 1, 0 };
    std::ifstream input(path, std::ios::in | std::ios::binary);
    if (!input.is_open())
        return AWSImageData();

    bool valid = false;
    std::size_t headerSize = 0;

    // first line, this shoul be "# HEADER_DATA_BEGIN: 4096" (type version header lenght) if valid file
    std::string firstline;
    std::getline(input, firstline);
    auto strings = string_split(firstline, ':');
    if (strings.size() > 1) {
        if (strings[0].compare("# HEADER_DATA_BEGIN") == 0) {
            try {
                headerSize = std::stoi(strings[1]);
            } catch (const std::invalid_argument&) {
                return AWSImageData();
            }
            valid = true;
        }
    }

    if (!valid) {
        return AWSImageData();
    }
    //reading dimension data
    std::string header(headerSize, ' ');
    input.read(&header[0], headerSize);
    auto lines = string_split(header, '\n');
    for (const auto& line : lines) {
        auto lv = string_split(line, ':');
        if (lv.size() == 2) {
            if (lv[0].compare("# WIDTH") == 0) {
                dimensions[0] = std::stoi(lv[1]);
            } else if (lv[0].compare("# HEIGHT") == 0) {
                dimensions[1] = std::stoi(lv[1]);
            } else if (lv[0].compare("# DEPTH") == 0) {
                dimensions[2] = std::stoi(lv[1]);
            } else if (lv[0].compare("# HEIGHT_SPACING") == 0) {
                spacing[0] = std::stod(lv[1]);
            } else if (lv[0].compare("# WIDTH_SPACING") == 0) {
                spacing[1] = std::stod(lv[1]);
            } else if (lv[0].compare("# DEPTH_SPACING") == 0) {
                spacing[2] = std::stod(lv[1]);
            } else if (lv[0].compare("# COSINES_X1") == 0) {
                cosines[0] = std::stod(lv[1]);
            } else if (lv[0].compare("# COSINES_X2") == 0) {
                cosines[1] = std::stod(lv[1]);
            } else if (lv[0].compare("# COSINES_X3") == 0) {
                cosines[2] = std::stod(lv[1]);
            } else if (lv[0].compare("# COSINES_Y1") == 0) {
                cosines[3] = std::stod(lv[1]);
            } else if (lv[0].compare("# COSINES_Y2") == 0) {
                cosines[4] = std::stod(lv[1]);
            } else if (lv[0].compare("# COSINES_Y3") == 0) {
                cosines[5] = std::stod(lv[1]);
            }
        }
    }

    const std::size_t imageSize = dimensions[0] * dimensions[1] * dimensions[2];

    valid = (spacing[0] * spacing[1] * spacing[2] != 0) && (imageSize != 0);
    if (!valid)
        return AWSImageData();

    //reading image data
    auto organArray = std::make_shared<std::vector<std::uint8_t>>(imageSize + headerSize, 0);

    input.seekg(0, std::ios::beg);
    input.read(reinterpret_cast<char*>(organArray->data()), imageSize + headerSize);
    organArray->erase(organArray->begin(), organArray->begin() + headerSize);

    AWSImageData data;
    data.dimensions = dimensions;
    data.spacing = spacing;
    data.image = organArray;
    data.cosines = cosines;
    return data;
}

void PhantomImportPipeline::importAWSPhantom(const QString& name)
{
    emit processingDataStarted();

    auto name_std = name.toStdString();

    auto organs = readICRPOrgans("resources/phantoms/other/" + name_std + "_organs.dat");
    auto media = readICRPMedia("resources/phantoms/other/media.dat");
    auto organData = readAWSData("resources/phantoms/other/" + name_std);

    if (!organData.image) {
        emit processingDataEnded();
        return;
    }

    auto& organArray = organData.image;
    auto dimensions = organData.dimensions;
    auto spacing = organData.spacing;
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);
    //organs = sortICRUOrgans(organArray, organs);
    //auto materials = sortICRUMaterials(organs, media);

    auto data = generateICRUPhantomArrays(organArray, organs, media);

    auto organImage = std::make_shared<OrganImageContainer>(data.organArray, dimensions, spacing, origin);
    auto materialImage = std::make_shared<MaterialImageContainer>(data.materialArray, dimensions, spacing, origin);
    auto densityImage = std::make_shared<DensityImageContainer>(data.densityArray, dimensions, spacing, origin);
    organImage->ID = ImageContainer::generateID();
    materialImage->ID = organImage->ID;
    densityImage->ID = organImage->ID;
    organImage->directionCosines = organData.cosines;
    materialImage->directionCosines = organData.cosines;
    densityImage->directionCosines = organData.cosines;

    emit imageDataChanged(densityImage);
    emit imageDataChanged(organImage);
    emit imageDataChanged(materialImage);
    emit materialDataChanged(data.materials);
    emit organDataChanged(data.organNames);
    emit processingDataEnded();
}

void PhantomImportPipeline::importCTDIPhantom(int mm, bool force_interaction_measurements)
{
    emit processingDataStarted();
    auto w = CTDIPhantom(static_cast<std::size_t>(mm));

    const auto& materialMap = w.materialMap();

    auto densityArray = w.densityArray();
    auto materialArray = w.materialIndexArray();
    auto forceInteractionArray = w.measurementMapArray();

    const auto& dimensions = w.dimensions();
    const auto& spacing_f = w.spacing();
    auto spacing = convert_array_to<double>(spacing_f);
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);

    //copying organMap and organArray from materialMap and materialArray
    std::vector<std::string> organMap;
    auto organArray = std::make_shared<std::vector<std::uint8_t>>(materialArray->size());
    std::copy(materialArray->begin(), materialArray->end(), organArray->begin());
    for (const auto& m : materialMap) {
        organMap.push_back(m.name());
    }
    //adding measurement holes
    typedef CTDIPhantom::HolePosition holePosition;
    std::array<CTDIPhantom::HolePosition, 5> CTDIpositions = { holePosition::West, holePosition::East, holePosition::North, holePosition::South, holePosition::Center };
    organMap.push_back("CTDI measurement west");
    organMap.push_back("CTDI measurement east");
    organMap.push_back("CTDI measurement north");
    organMap.push_back("CTDI measurement south");
    organMap.push_back("CTDI measurement center");
    auto organBuffer = organArray->data();
    const auto nMaterials = static_cast<std::uint8_t>(materialMap.size());
    for (std::size_t i = 0; i < 5; ++i) {
        const auto& idx = w.holeIndices(CTDIpositions[i]);
        for (auto ind : idx)
            organBuffer[ind] = static_cast<std::uint8_t>(i + nMaterials);
    }

    auto materialImage = std::make_shared<MaterialImageContainer>(materialArray, dimensions, spacing, origin);
    auto densityImage = std::make_shared<DensityImageContainer>(densityArray, dimensions, spacing, origin);
    auto organImage = std::make_shared<OrganImageContainer>(organArray, dimensions, spacing, origin);
    auto measureImage = std::make_shared<MeasurementImageContainer>(forceInteractionArray, dimensions, spacing, origin);
    materialImage->ID = ImageContainer::generateID();
    densityImage->ID = materialImage->ID;
    organImage->ID = materialImage->ID;
    measureImage->ID = materialImage->ID;
    materialImage->directionCosines = convert_array_to<double>(w.directionCosines());
    densityImage->directionCosines = materialImage->directionCosines;
    organImage->directionCosines = materialImage->directionCosines;
    measureImage->directionCosines = materialImage->directionCosines;
    emit processingDataEnded();
    emit materialDataChanged(materialMap);
    emit organDataChanged(organMap);
    emit imageDataChanged(densityImage);
    emit imageDataChanged(materialImage);
    emit imageDataChanged(organImage);

    if (force_interaction_measurements)
        emit imageDataChanged(measureImage);
}
