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

PhantomImportPipeline::PhantomImportPipeline(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
    qRegisterMetaType<std::vector<Material>>();
    qRegisterMetaType<std::vector<std::string>>();
}

struct organElement {
    unsigned char ID = 0;
    unsigned char tissue = 0;
    double density = 0;
    std::string name;
};

std::vector<organElement> readICRPOrgans(const std::string& path)
{
    std::ifstream input(path);
    if (!input.is_open())
        return std::vector<organElement>();
    std::size_t teller = 0;

    //reading organs
    std::vector<organElement> organs;

    // adding 0 air organ
    struct organElement airElement;
    airElement.ID = 0;
    airElement.tissue = 0;
    Material airMaterial("Air, Dry (near sea level)");
    airElement.density = airMaterial.standardDensity();
    airElement.name = airMaterial.name();
    organs.push_back(airElement);

    for (std::string line; getline(input, line);) {
        //skipping first 4 lines
        if (teller > 3) {
            if (line.size() > 65) {
                std::string id = line.substr(0, 6);
                std::string name = line.substr(6, 49);
                std::string tissue = line.substr(54, 3);
                std::string density = line.substr(61, 5);
                struct organElement o;
                o.ID = std::stoi(id);
                o.tissue = std::stoi(tissue);
                name.erase(name.find_last_not_of(" \n\r\t") + 1);
                o.name = name;
                o.density = std::stod(density);
                organs.push_back(o);
            }
        }
        ++teller;
    }
    //sorting
    airElement.ID = static_cast<int>(organs.size());
    organs.push_back(airElement);

    std::sort(organs.begin(), organs.end(), [](auto& left, auto& right) {
        return left.ID < right.ID;
    });

    return organs;
}

std::vector<std::pair<unsigned char, Material>> readICRPMedia(const std::string& path)
{
    std::ifstream input(path);
    if (!input.is_open())
        return std::vector<std::pair<unsigned char, Material>>();
    std::size_t teller = 0;

    //reading media
    std::vector<std::pair<unsigned char, Material>> media;
    media.push_back(std::make_pair(0, Material("Air, Dry (near sea level)")));

    std::vector<int> atomicNumbers;
    std::vector<double> massFractions;

    std::size_t lineOffset = 78;
    std::size_t numberSkip = 6;
    for (std::string line; getline(input, line);) {
        //First line contains atomic numbers
        if (teller == 0) {
            std::size_t offset = lineOffset;
            while ((offset + numberSkip) < line.size()) {
                auto subString = line.substr(offset, numberSkip);
                atomicNumbers.push_back(std::stoi(subString));
                offset += numberSkip;
            }
        }
        //skipping first 4 lines
        if (teller > 2) {
            //read media here make materials remember convert to number of atoms
            std::size_t offset = lineOffset;
            massFractions.clear();
            while ((offset + numberSkip) < line.size()) {
                auto subString = line.substr(offset, numberSkip);
                massFractions.push_back(std::stod(subString));
                offset += numberSkip;
            }

            //constructing material string
            std::string compound;
            for (std::size_t i = 0; i < atomicNumbers.size(); ++i) {
                if (massFractions[i] > 0.0) {
                    double numberFraction = massFractions[i] / Material::getAtomicWeight(atomicNumbers[i]);
                    auto element = Material::getAtomicNumberToSymbol(atomicNumbers[i]);
                    compound += element;
                    compound += std::to_string(numberFraction);
                }
            }

            // finding material number
            auto materialNumber = static_cast<unsigned char>(std::stoi(line.substr(0, 3)));
            //material name
            std::string materialName = line.substr(6, 69);
            materialName.erase(materialName.find_last_not_of(" \n\r\t") + 1);
            // adding material
            media.push_back(std::make_pair(materialNumber, Material(compound, materialName)));
        }
        ++teller;
    }
    // sorting
    std::sort(media.begin(), media.end(), [](auto& left, auto& right) {
        return left.first < right.first;
    });
    return media;
}

std::shared_ptr<std::vector<unsigned char>> readICRPData(const std::string& path, std::size_t size)
{
    auto organs = std::make_shared<std::vector<unsigned char>>();
    std::ifstream input(path);
    if (!input.is_open())
        return organs;
    organs->reserve(size);
    int c;
    while (input >> c) {
        organs->push_back(static_cast<unsigned char>(c));
    }
    return organs;
}

std::pair<std::shared_ptr<std::vector<unsigned char>>, std::shared_ptr<std::vector<double>>> generateICRUPhantomArrays(
    std::vector<unsigned char>& organArray,
    std::vector<organElement>& organs,
    const std::vector<std::pair<unsigned char, Material>>& media)
{
    std::map<unsigned char, double> densityLut;
    std::map<unsigned char, unsigned char> materialLut;
    std::map<unsigned char, unsigned char> organLut;

    for (std::size_t i = 0; i < organs.size(); ++i) {
        auto key = organs[i].ID;
        materialLut[key] = static_cast<unsigned char>(organs[i].tissue);
        densityLut[key] = organs[i].density;
        organLut[key] = static_cast<unsigned char>(i);
        organs[i].ID = static_cast<unsigned char>(i);
    }

    auto materialArray = std::make_shared<std::vector<unsigned char>>(organArray.size());
    auto densityArray = std::make_shared<std::vector<double>>(organArray.size());

    auto materialBuffer = materialArray->data();
    auto densityBuffer = densityArray->data();
    for (std::size_t i = 0; i < organArray.size(); ++i) {
        materialBuffer[i] = materialLut[organArray[i]];
        densityBuffer[i] = densityLut[organArray[i]];
        organArray[i] = organLut[organArray[i]];
    }
    return std::make_pair(materialArray, densityArray);
}

void PhantomImportPipeline::importICRUMalePhantom(bool ignoreArms)
{
    emit processingDataStarted();
    std::array<double, 3> spacing = { 2.137, 2.137, 8.8 };
    std::array<std::size_t, 3> dimensions = { 254, 127, 222 };
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);
    auto size = dimensions[0] * dimensions[1] * dimensions[2];

    auto organs = readICRPOrgans("resources/phantoms/icrp/AM/AM_organs.dat");
    auto media = readICRPMedia("resources/phantoms/icrp/AM/AM_media.dat");
    auto organArray = readICRPData("resources/phantoms/icrp/AM/AM.dat", size);

    if (ignoreArms) {
        for (auto& organ : organs) {
            //find arm string
            auto armPos = organ.name.find("arm");
            if (armPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
            auto handPos = organ.name.find("hand");
            if (handPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
            auto humeriPos = organ.name.find("Humeri");
            if (humeriPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
            auto ulnaRadiiPos = organ.name.find("Ulnae");
            if (ulnaRadiiPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
        }
    }

    auto [materialArray, densityArray] = generateICRUPhantomArrays(*organArray, organs, media);

    bool valid = true;
    std::vector<std::string> organMap(organs.size());
    std::vector<Material> materialMap(organs.size());
    for (std::size_t i = 0; i < organs.size(); ++i) {
        if (organs[i].ID == i)
            organMap[i] = organs[i].name;
        else
            valid = false;
    }
    for (std::size_t i = 0; i < media.size(); ++i) {
        auto& [id, material] = media[i];
        material.setStandardDensity(1.0);
        if (id == i)
            materialMap[i] = material;
        else
            valid = false;
    }
    if (!valid) {
        emit processingDataEnded();
        return;
    }

    auto organImage = std::make_shared<OrganImageContainer>(organArray, dimensions, spacing, origin);
    auto materialImage = std::make_shared<MaterialImageContainer>(materialArray, dimensions, spacing, origin);
    auto densityImage = std::make_shared<DensityImageContainer>(densityArray, dimensions, spacing, origin);
    organImage->ID = ImageContainer::generateID();
    materialImage->ID = organImage->ID;
    densityImage->ID = organImage->ID;

    emit processingDataEnded();
    emit materialDataChanged(materialMap);
    emit organDataChanged(organMap);
    emit imageDataChanged(organImage);
    emit imageDataChanged(densityImage);
    emit imageDataChanged(materialImage);
}

void PhantomImportPipeline::importICRUFemalePhantom(bool ignoreArms)
{
    emit processingDataStarted();
    std::array<double, 3> spacing = { 1.775, 1.775, 4.84 };
    std::array<std::size_t, 3> dimensions = { 299, 137, 348 };
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);
    auto size = dimensions[0] * dimensions[1] * dimensions[2];

    auto organs = readICRPOrgans("resources/phantoms/icrp/AF/AF_organs.dat");
    auto media = readICRPMedia("resources/phantoms/icrp/AF/AF_media.dat");
    auto organArray = readICRPData("resources/phantoms/icrp/AF/AF.dat", size);

    if (ignoreArms) {
        for (auto& organ : organs) {
            //find arm string
            auto armPos = organ.name.find("arm");
            if (armPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
            auto handPos = organ.name.find("hand");
            if (handPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
            auto humeriPos = organ.name.find("Humeri");
            if (humeriPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
            auto ulnaRadiiPos = organ.name.find("Ulnae");
            if (ulnaRadiiPos != std::string::npos) {
                std::replace(std::execution::par_unseq, organArray->begin(), organArray->end(), static_cast<unsigned char>(organ.ID), static_cast<unsigned char>(0));
            }
        }
    }

    auto [materialArray, densityArray] = generateICRUPhantomArrays(*organArray, organs, media);

    bool valid = true;
    std::vector<std::string> organMap(organs.size());
    for (std::size_t i = 0; i < organs.size(); ++i) {
        if (organs[i].ID == i)
            organMap[i] = organs[i].name;
        else
            valid = false;
    }
    std::vector<Material> materialMap(media.size());
    for (std::size_t i = 0; i < media.size(); ++i) {
        auto& [id, material] = media[i];
        material.setStandardDensity(1.0);
        if (id == i)
            materialMap[i] = material;
        else
            valid = false;
    }
    if (!valid) {
        emit processingDataEnded();
        return;
    }

    auto organImage = std::make_shared<OrganImageContainer>(organArray, dimensions, spacing, origin);
    auto materialImage = std::make_shared<MaterialImageContainer>(materialArray, dimensions, spacing, origin);
    auto densityImage = std::make_shared<DensityImageContainer>(densityArray, dimensions, spacing, origin);
    organImage->ID = ImageContainer::generateID();
    materialImage->ID = organImage->ID;
    densityImage->ID = organImage->ID;

    emit processingDataEnded();
    emit materialDataChanged(materialMap);
    emit organDataChanged(organMap);
    emit imageDataChanged(organImage);
    emit imageDataChanged(densityImage);
    emit imageDataChanged(materialImage);
}

struct AWSImageData {
    std::array<std::size_t, 3> dimensions = { 0, 0, 0 };
    std::array<double, 3> spacing = { 0, 0, 0 };
    std::array<double, 6> cosines = { 1, 0, 0, 0, 1, 0 };
    std::shared_ptr<std::vector<unsigned char>> image = nullptr;
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

    auto organArray = organData.image;
    auto dimensions = organData.dimensions;
    auto spacing = organData.spacing;
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);

    auto [materialArray, densityArray] = generateICRUPhantomArrays(*organArray, organs, media);

    bool valid = true;
    std::vector<std::string> organMap(organs.size());
    std::vector<Material> materialMap(media.size());
    for (std::size_t i = 0; i < organs.size(); ++i) {
        if (organs[i].ID == i)
            organMap[i] = organs[i].name;
        else
            valid = false;
    }
    for (std::size_t i = 0; i < media.size(); ++i) {
        auto& [id, material] = media[i];
        material.setStandardDensity(1.0);
        if (id == i)
            materialMap[i] = material;
        else
            valid = false;
    }
    if (!valid) {
        emit processingDataEnded();
        return;
    }

    auto organImage = std::make_shared<OrganImageContainer>(organArray, dimensions, spacing, origin);
    auto materialImage = std::make_shared<MaterialImageContainer>(materialArray, dimensions, spacing, origin);
    auto densityImage = std::make_shared<DensityImageContainer>(densityArray, dimensions, spacing, origin);
    organImage->ID = ImageContainer::generateID();
    materialImage->ID = organImage->ID;
    densityImage->ID = organImage->ID;
    organImage->directionCosines = organData.cosines;
    materialImage->directionCosines = organData.cosines;
    densityImage->directionCosines = organData.cosines;

    emit imageDataChanged(densityImage);
    emit imageDataChanged(organImage);
    emit imageDataChanged(materialImage);
    emit materialDataChanged(materialMap);
    emit organDataChanged(organMap);
    emit processingDataEnded();
}

void PhantomImportPipeline::importCTDIPhantom(int mm, bool force_interaction_measurements)
{
    emit processingDataStarted();
    auto w = CTDIPhantom(static_cast<std::size_t>(mm));

    auto materialMap = w.materialMap();

    auto densityArray = w.densityArray();
    auto materialArray = w.materialIndexArray();
    auto forceInteractionArray = w.measurementMapArray();

    auto dimensions = w.dimensions();
    auto spacing = w.spacing();
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);

    //copying organMap and organArray from materialMap and materialArray
    std::vector<std::string> organMap;
    auto organArray = std::make_shared<std::vector<unsigned char>>(materialArray->size());
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
    const auto nMaterials = static_cast<unsigned char>(materialMap.size());
    for (std::size_t i = 0; i < 5; ++i) {
        const auto idx = w.holeIndices(CTDIpositions[i]);
        for (auto ind : idx)
            organBuffer[ind] = static_cast<unsigned char>(i + nMaterials);
    }

    auto materialImage = std::make_shared<MaterialImageContainer>(materialArray, dimensions, spacing, origin);
    auto densityImage = std::make_shared<DensityImageContainer>(densityArray, dimensions, spacing, origin);
    auto organImage = std::make_shared<OrganImageContainer>(organArray, dimensions, spacing, origin);
    auto measureImage = std::make_shared<MeasurementImageContainer>(forceInteractionArray, dimensions, spacing, origin);
    materialImage->ID = ImageContainer::generateID();
    densityImage->ID = materialImage->ID;
    organImage->ID = materialImage->ID;
    measureImage->ID = materialImage->ID;
    materialImage->directionCosines = w.directionCosines();
    densityImage->directionCosines = w.directionCosines();
    organImage->directionCosines = w.directionCosines();
    measureImage->directionCosines = w.directionCosines();
    emit processingDataEnded();
    emit materialDataChanged(materialMap);
    emit organDataChanged(organMap);
    emit imageDataChanged(densityImage);
    emit imageDataChanged(materialImage);
    emit imageDataChanged(organImage);

    if (force_interaction_measurements)
        emit imageDataChanged(measureImage);
}
