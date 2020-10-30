
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

#include "opendxmc/h5wrapper.h"
#include "opendxmc/stringmanipulation.h"

H5Wrapper::H5Wrapper(const std::string& filePath, FileOpenType type)
{
    m_file = nullptr;
    try {
        if (type == FileOpenType::WriteOver)
            m_file = std::make_unique<H5::H5File>(filePath.c_str(), H5F_ACC_TRUNC);
        else if (type == FileOpenType::ReadOnly)
            m_file = std::make_unique<H5::H5File>(filePath.c_str(), H5F_ACC_RDONLY);
    } catch (...) {
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
    return true;
}

std::shared_ptr<ImageContainer> H5Wrapper::loadImage(ImageContainer::ImageType type)
{
    auto image = loadDataSet(type, "/arrays");
    return image;
}

bool H5Wrapper::saveOrganList(const std::vector<std::string>& organList)
{
    saveStringList(organList, "organList", "/arrays");
    return false;
}

std::vector<std::string> H5Wrapper::loadOrganList(void)
{
    std::vector<std::string> list = loadStringList("organList", "/arrays");
    return list;
}

bool H5Wrapper::saveMaterials(const std::vector<Material>& materials)
{
    std::vector<std::string> names;
    std::vector<std::string> prettyNames;
    std::vector<double> densities;
    names.reserve(materials.size());
    prettyNames.reserve(materials.size());
    densities.reserve(materials.size());
    for (const Material& m : materials) {
        names.push_back(m.name());
        prettyNames.push_back(m.prettyName());
        densities.push_back(m.standardDensity());
    }
    saveStringList(names, "materialList", "/arrays");
    saveStringList(prettyNames, "materialPrettyList", "/arrays");
    saveDoubleList(densities, "materialDensityList", "/arrays");
    return false;
}

std::vector<Material> H5Wrapper::loadMaterials(void)
{
    auto materialNames = loadStringList("materialList", "/arrays");
    auto materialPrettyNames = loadStringList("materialPrettyList", "/arrays");
    auto densities = loadDoubleList("materialDensityList", "/arrays");
    if (materialNames.size() != materialPrettyNames.size() || materialNames.size() != densities.size())
        return std::vector<Material>();
    std::vector<Material> materials;
    materials.reserve(materialNames.size());

    for (std::size_t i = 0; i < materialNames.size(); ++i) {
        Material m(materialNames[i], materialPrettyNames[i]);
        m.setStandardDensity(densities[i]);
        if (!m.isValid())
            return std::vector<Material>();
        materials.push_back(m);
    }

    //this is better but we must loop anyway to prevent invalid materials
    /*std::transform(materialNames.begin(), materialNames.end(), materialPrettyNames.begin(), std::back_inserter(materials), [](const auto& name, const auto& pname) {
		Material m(name, pname);
		m.setStandardDensity(1.0);
		return m;
		});
	*/

    return materials;
}

bool H5Wrapper::saveSources(const std::vector<std::shared_ptr<Source>>& sources)
{
    std::size_t tellerDX = 1;
    std::size_t tellerCTAxial = 1;
    std::size_t tellerCTSpiral = 1;
    std::size_t tellerCTDual = 1;

    std::string groupPath = "/sources";
    for (auto s : sources) {
        bool valid = false;
        if (s->type() == Source::Type::DX) {
            auto s_downcast = std::static_pointer_cast<DXSource>(s);
            auto path = groupPath + "/" + "DX";
            auto name = std::to_string(tellerDX++);
            valid = saveSource(s_downcast, name, path);
        } else if (s->type() == Source::Type::CTAxial) {
            auto s_downcast = std::static_pointer_cast<CTAxialSource>(s);
            auto path = groupPath + "/" + "CTAxial";
            auto name = std::to_string(tellerCTAxial++);
            valid = saveSource(s_downcast, name, path);
        } else if (s->type() == Source::Type::CTSpiral) {
            auto s_downcast = std::static_pointer_cast<CTSpiralSource>(s);
            auto path = groupPath + "/" + "CTSpiral";
            auto name = std::to_string(tellerCTSpiral++);
            valid = saveSource(s_downcast, name, path);
        } else if (s->type() == Source::Type::CTDual) {
            auto s_downcast = std::static_pointer_cast<CTSpiralDualSource>(s);
            auto path = groupPath + "/" + "CTDual";
            auto name = std::to_string(tellerCTDual++);
            valid = saveSource(s_downcast, name, path);
        }
        if (!valid)
            return false;
    }
    return true;
}

std::vector<std::shared_ptr<Source>> H5Wrapper::loadSources(void)
{
    std::vector<std::shared_ptr<Source>> sources;
    if (!m_file)
        return sources;
    if (!m_file->nameExists("sources"))
        return sources;

    auto sourceGroup = getGroup("sources/", false);
    if (!sourceGroup)
        return sources;

    std::map<std::string, Source::Type> sourceFolders;
    sourceFolders["CTDual"] = Source::Type::CTDual;
    sourceFolders["CTAxial"] = Source::Type::CTAxial;
    sourceFolders["CTSpiral"] = Source::Type::CTSpiral;
    sourceFolders["DX"] = Source::Type::DX;

    for (const auto [sourceFolder, type] : sourceFolders) {
        auto folderPath = "sources/" + sourceFolder;
        auto folderGroup = getGroup(folderPath, false);
        if (folderGroup) {
            std::size_t teller = 1;
            auto name = std::to_string(teller);
            auto sourcepath = folderPath + "/" + name;
            while (getGroup(sourcepath, false)) {
                if (type == Source::Type::DX) {
                    auto src = std::make_shared<DXSource>();
                    bool valid = loadSource(src, name, folderPath);
                    if (valid)
                        sources.push_back(src);
                } else if (type == Source::Type::CTSpiral) {
                    auto src = std::make_shared<CTSpiralSource>();
                    bool valid = loadSource(src, name, folderPath);
                    if (valid)
                        sources.push_back(src);
                } else if (type == Source::Type::CTAxial) {
                    auto src = std::make_shared<CTAxialSource>();
                    bool valid = loadSource(src, name, folderPath);
                    if (valid)
                        sources.push_back(src);
                } else if (type == Source::Type::CTDual) {
                    auto src = std::make_shared<CTSpiralDualSource>();
                    bool valid = loadSource(src, name, folderPath);
                    if (valid)
                        sources.push_back(src);
                }

                teller++;
                name = std::to_string(teller);
                sourcepath = folderPath + "/" + name;
            }
        }
    }
    return sources;
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
        if (!m_file->nameExists(fullname.c_str())) {
            if (create) {
                group = std::make_unique<H5::Group>(m_file->createGroup(fullname.c_str()));
            } else {
                return nullptr;
            }
        } else {
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
    cdims[2] = dims[2];
    ds_createplist.setChunk(rank, cdims);
    ds_createplist.setDeflate(6);

    auto type = H5::PredType::NATIVE_UCHAR;
    if (image->image->GetScalarType() == VTK_DOUBLE)
        type = H5::PredType::NATIVE_DOUBLE;
    else if (image->image->GetScalarType() == VTK_FLOAT)
        type = H5::PredType::NATIVE_FLOAT;
    else if (image->image->GetScalarType() == VTK_UNSIGNED_CHAR)
        type = H5::PredType::NATIVE_UCHAR;
    else if (image->image->GetScalarType() == VTK_UNSIGNED_INT)
        type = H5::PredType::NATIVE_UINT32;
    else
        return nullptr;
    auto dataset = std::make_unique<H5::DataSet>(group->createDataSet(namePath.c_str(), type, *dataspace, ds_createplist));
    dataset->write(image->image->GetScalarPointer(), type);

    //saving attributes
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
    //writing ID
    const hsize_t dim1 = 1;
    auto id = image->ID;
    H5::DataSpace idSpace6(1, &dim1);
    auto id_attr = dataset->createAttribute("ID", H5::PredType::NATIVE_UINT64, idSpace6);
    id_attr.write(H5::PredType::NATIVE_UINT64, &id);
    //writing data units
    if (image->dataUnits.length() > 0) {
        hsize_t strLen = static_cast<hsize_t>(image->dataUnits.length());
        H5::StrType stringType(0, strLen);
        H5::DataSpace stringSpace(H5S_SCALAR);
        auto dataUnits = dataset->createAttribute("dataUnits", stringType, stringSpace);
        dataUnits.write(stringType, image->dataUnits.c_str());
    }

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
    } catch (H5::FileIException notFoundError) {
        return nullptr;
    }

    auto space = dataset->getSpace();
    auto rank = space.getSimpleExtentNdims();
    if (rank != 3)
        return nullptr;
    hsize_t h5dims[3];
    space.getSimpleExtentDims(h5dims);

    std::array<std::size_t, 3> dim { h5dims[0], h5dims[1], h5dims[2] };
    std::array<double, 3> origin { 0, 0, 0 };
    std::array<double, 3> spacing { 1, 1, 1 };
    std::array<double, 6> direction { 1, 0, 0, 0, 1, 0 };
    std::uint64_t id = 0;
    std::string units("");

    try {
        auto origin_attr = dataset->openAttribute("origin");
        origin_attr.read(H5::PredType::NATIVE_DOUBLE, origin.data());
        auto spacing_attr = dataset->openAttribute("spacing");
        spacing_attr.read(H5::PredType::NATIVE_DOUBLE, spacing.data());
        auto dir_attr = dataset->openAttribute("direction_cosines");
        dir_attr.read(H5::PredType::NATIVE_DOUBLE, direction.data());
        auto id_attr = dataset->openAttribute("ID");
        id_attr.read(H5::PredType::NATIVE_UINT64, &id);
        if (dataset->attrExists("dataUnits")) {
            auto units_attr = dataset->openAttribute("dataUnits");
            std::size_t units_size = units_attr.getInMemDataSize();
            units.resize(units_size);
            H5::StrType stringType(0, units_size);
            units_attr.read(stringType, units.data());
        }
    } catch (...) {
        return nullptr;
    }

    std::shared_ptr<ImageContainer> image = nullptr;

    auto type_class = dataset->getTypeClass();
    auto size = dim[0] * dim[1] * dim[2];
    if (type_class == H5T_FLOAT) {
        auto double_type_class = dataset->getFloatType();
        auto type_size = double_type_class.getSize();
        if (type_size == 4) {
            auto data = std::make_shared<std::vector<float>>(size);
            float* buffer = data->data();
            dataset->read(buffer, H5::PredType::NATIVE_FLOAT);
            image = std::make_shared<ImageContainer>(type, data, dim, spacing, origin);
        } else if (type_size == 8) {
            auto data = std::make_shared<std::vector<double>>(size);
            double* buffer = data->data();
            dataset->read(buffer, H5::PredType::NATIVE_DOUBLE);
            image = std::make_shared<ImageContainer>(type, data, dim, spacing, origin);
        }
    } else if (type_class == H5T_INTEGER) {
        auto int_type_class = dataset->getIntType();
        auto type_size = int_type_class.getSize();
        if (type_size == 1) {
            auto data = std::make_shared<std::vector<unsigned char>>(size);
            unsigned char* buffer = data->data();
            dataset->read(buffer, H5::PredType::NATIVE_UCHAR);
            image = std::make_shared<ImageContainer>(type, data, dim, spacing, origin);
        } else if (type_size == 4) {
            auto data = std::make_shared<std::vector<std::uint32_t>>(size);
            std::uint32_t* buffer = data->data();
            dataset->read(buffer, H5::PredType::NATIVE_UINT32);
            image = std::make_shared<ImageContainer>(type, data, dim, spacing, origin);
        }
    }
    if (!image)
        return nullptr;
    image->directionCosines = direction;
    image->dataUnits = units;
    image->ID = id;
    return image;
}

bool H5Wrapper::saveStringList(const std::vector<std::string>& list, const std::string& name, const std::string& groupPath)
{
    if (list.size() == 0)
        return false;

    auto group = getGroup(groupPath, true);
    if (!group)
        return false;

    std::size_t maxStrLen = 0;
    for (const auto& s : list)
        maxStrLen = std::max(maxStrLen, s.size());
    if (maxStrLen == 0)
        return false;

    std::vector<char> rawData(maxStrLen * list.size(), ' ');
    for (std::size_t i = 0; i < list.size(); ++i) {
        auto idx = i * maxStrLen;
        for (const char& c : list[i])
            rawData[idx++] = c;
    }

    auto namePath = groupPath + "/" + name;

    hsize_t size[2];
    size[0] = list.size();
    size[1] = maxStrLen;

    auto dataspace = std::make_unique<H5::DataSpace>(2, size);
    auto type = H5::PredType::NATIVE_CHAR;
    auto dataset = std::make_unique<H5::DataSet>(group->createDataSet(namePath.c_str(), type, *dataspace));
    dataset->write(rawData.data(), H5::PredType::NATIVE_CHAR);
    return true;
}

std::vector<std::string> H5Wrapper::loadStringList(const std::string& name, const std::string& groupPath)
{
    std::vector<std::string> list;
    auto group = getGroup(groupPath, false);
    if (!group)
        return list;

    auto path = groupPath + "/" + name;

    std::unique_ptr<H5::DataSet> dataset = nullptr;
    try {
        dataset = std::make_unique<H5::DataSet>(m_file->openDataSet(path.c_str()));
    } catch (H5::FileIException notFoundError) {
        return list;
    }

    auto space = dataset->getSpace();
    auto rank = space.getSimpleExtentNdims();
    if (rank != 2)
        return list;
    hsize_t h5dims[2];
    space.getSimpleExtentDims(h5dims);
    std::size_t rawSize(h5dims[0] * h5dims[1]);
    if (rawSize == 0)
        return list;
    std::vector<char> rawData(rawSize, ' ');
    auto type = H5::PredType::NATIVE_CHAR;

    dataset->read(rawData.data(), type);

    const auto nStrings = static_cast<std::size_t>(h5dims[0]);
    const auto nChars = static_cast<std::size_t>(h5dims[1]);

    list.reserve(nStrings);
    for (std::size_t i = 0; i < nStrings; ++i) {
        std::string s(rawData.begin() + i * nChars, rawData.begin() + (i + 1) * nChars);
        list.push_back(string_trim(s));
    }

    return list;
}

bool H5Wrapper::saveDoubleList(const std::vector<double>& values, const std::string& name, const std::string& groupPath)
{
    if (values.size() == 0)
        return false;

    auto group = getGroup(groupPath, true);
    if (!group)
        return false;

    auto namePath = groupPath + "/" + name;

    hsize_t size(values.size());

    auto dataspace = std::make_unique<H5::DataSpace>(1, &size);
    auto type = H5::PredType::NATIVE_DOUBLE;
    auto dataset = std::make_unique<H5::DataSet>(group->createDataSet(namePath.c_str(), type, *dataspace));
    dataset->write(values.data(), H5::PredType::NATIVE_DOUBLE);
    return true;
}

std::vector<double> H5Wrapper::loadDoubleList(const std::string& name, const std::string& groupPath)
{
    std::vector<double> list;
    auto group = getGroup(groupPath, false);
    if (!group)
        return list;

    auto path = groupPath + "/" + name;

    std::unique_ptr<H5::DataSet> dataset = nullptr;
    try {
        dataset = std::make_unique<H5::DataSet>(m_file->openDataSet(path.c_str()));
    } catch (H5::FileIException notFoundError) {
        return list;
    }

    auto space = dataset->getSpace();
    auto rank = space.getSimpleExtentNdims();
    if (rank != 1)
        return list;
    hsize_t h5dims;
    space.getSimpleExtentDims(&h5dims);

    list.resize(static_cast<std::size_t>(h5dims));
    //TODO error catching here
    dataset->read(list.data(), H5::PredType::NATIVE_DOUBLE);
    return list;
}

std::unique_ptr<H5::Group> H5Wrapper::saveTube(const Tube& tube, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return nullptr;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, true);
    if (!group)
        return nullptr;

    const hsize_t dim1 = 1;
    H5::DataSpace doubleSpace1(1, &dim1);
    std::map<std::string, double> valueMap;
    valueMap["voltage"] = tube.voltage();
    valueMap["energyResolution"] = tube.energyResolution();
    valueMap["angle"] = tube.anodeAngle();

    for (const auto& [key, val] : valueMap) {
        auto att = group->createAttribute(key.c_str(), H5::PredType::NATIVE_DOUBLE, doubleSpace1);
        att.write(H5::PredType::NATIVE_DOUBLE, &val);
    }

    auto filtrationMaterials = tube.filtrationMaterials();
    if (filtrationMaterials.size() > 0) {
        std::vector<std::string> matNames;
        std::vector<double> matDensities;
        std::vector<double> matMm;
        for (const auto& [mat, mm] : filtrationMaterials) {
            matNames.push_back(mat.name());
            matDensities.push_back(mat.standardDensity());
            matMm.push_back(mm);
        }
        saveStringList(matNames, "filtrationMaterialNames", path.c_str());
        saveDoubleList(matDensities, "filtrationMaterialDensities", path.c_str());
        saveDoubleList(matMm, "filtrationMaterialThickness", path.c_str());
    }
    return group;
}

void H5Wrapper::loadTube(Tube& tube, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return;

    auto tubePath = groupPath + "/" + name;
    auto tubeGroup = getGroup(tubePath, false);

    if (!tubeGroup)
        return;

    double voltage, energyResolution, angle;

    try {
        auto voltage_attr = tubeGroup->openAttribute("voltage");
        voltage_attr.read(H5::PredType::NATIVE_DOUBLE, &voltage);
        auto er_attr = tubeGroup->openAttribute("energyResolution");
        er_attr.read(H5::PredType::NATIVE_DOUBLE, &energyResolution);
        auto angle_attr = tubeGroup->openAttribute("angle");
        angle_attr.read(H5::PredType::NATIVE_DOUBLE, &angle);
    } catch (...) {
        return;
    }
    tube.setVoltage(voltage);
    tube.setAnodeAngle(angle);
    tube.setEnergyResolution(energyResolution);
    if (tubeGroup->nameExists("filtrationMaterialNames")) {
        auto matNames = loadStringList("filtrationMaterialNames", tubePath.c_str());
        auto matDens = loadDoubleList("filtrationMaterialDensities", tubePath.c_str());
        auto matThick = loadDoubleList("filtrationMaterialThickness", tubePath.c_str());
        if (matDens.size() == matNames.size() && matThick.size() == matNames.size()) {
            for (std::size_t i = 0; i < matNames.size(); ++i) {
                Material m(matNames[i]);
                m.setStandardDensity(matDens[i]);
                tube.addFiltrationMaterial(m, matThick[i]);
            }
        }
    }
    return;
}
bool H5Wrapper::saveSource(std::shared_ptr<Source> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, false);
    if (!group)
        return false;

    const hsize_t dim1 = 1;
    const hsize_t dim3 = 3;
    const hsize_t dim6 = 6;
    H5::DataSpace doubleSpace3(1, &dim3);
    H5::DataSpace doubleSpace6(1, &dim6);
    H5::DataSpace doubleSpace1(1, &dim1);

    auto pos_att = group->createAttribute("position", H5::PredType::NATIVE_DOUBLE, doubleSpace3);
    pos_att.write(H5::PredType::NATIVE_DOUBLE, src->position().data());
    auto dir_att = group->createAttribute("directionCosines", H5::PredType::NATIVE_DOUBLE, doubleSpace6);
    dir_att.write(H5::PredType::NATIVE_DOUBLE, src->directionCosines().data());
    auto he = src->historiesPerExposure();
    auto he_att = group->createAttribute("historiesPerExposure", H5::PredType::NATIVE_UINT64, doubleSpace1);
    he_att.write(H5::PredType::NATIVE_UINT64, &he);

    return true;
}

bool H5Wrapper::loadSource(std::shared_ptr<Source> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, false);
    if (!group)
        return false;

    std::array<double, 3> position;
    std::array<double, 6> cosines;
    std::uint64_t he;

    const hsize_t dim3 = 3;
    const hsize_t dim6 = 6;
    H5::DataSpace doubleSpace3(1, &dim3);
    H5::DataSpace doubleSpace6(1, &dim6);

    try {
        auto pos_attr = group->openAttribute("position");
        pos_attr.read(H5::PredType::NATIVE_DOUBLE, position.data());
        auto co_attr = group->openAttribute("directionCosines");
        co_attr.read(H5::PredType::NATIVE_DOUBLE, cosines.data());
        auto he_attr = group->openAttribute("historiesPerExposure");
        he_attr.read(H5::PredType::NATIVE_UINT64, &he);
    } catch (...) {
        return false;
    }
    src->setPosition(position);
    src->setDirectionCosines(cosines);
    src->setHistoriesPerExposure(he);
    return true;
}

bool H5Wrapper::saveSource(std::shared_ptr<DXSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto srcPath = groupPath + "/" + name;
    auto srcGroup = getGroup(srcPath, true);
    if (!srcGroup)
        return false;

    if (!saveSource(std::static_pointer_cast<Source>(src), name, groupPath))
        return false;

    auto tubeGroup = saveTube(src->tube(), "Tube", srcPath);
    if (!tubeGroup)
        return false;

    const hsize_t dim1 = 1;
    const hsize_t dim2 = 2;
    H5::DataSpace doubleSpace1(1, &dim1);
    H5::DataSpace doubleSpace2(1, &dim2);

    double sdd = src->sourceDetectorDistance();
    auto sdd_att = srcGroup->createAttribute("sdd", H5::PredType::NATIVE_DOUBLE, doubleSpace1);
    sdd_att.write(H5::PredType::NATIVE_DOUBLE, &sdd);

    double dap = src->dap();
    auto dap_att = srcGroup->createAttribute("dap", H5::PredType::NATIVE_DOUBLE, doubleSpace1);
    dap_att.write(H5::PredType::NATIVE_DOUBLE, &dap);

    auto fs_att = srcGroup->createAttribute("fieldSize", H5::PredType::NATIVE_DOUBLE, doubleSpace2);
    fs_att.write(H5::PredType::NATIVE_DOUBLE, src->fieldSize().data());

    auto ca_att = srcGroup->createAttribute("collimationAngles", H5::PredType::NATIVE_DOUBLE, doubleSpace2);
    ca_att.write(H5::PredType::NATIVE_DOUBLE, src->collimationAngles().data());

    auto te = src->totalExposures();
    auto te_att = srcGroup->createAttribute("totalExposures", H5::PredType::NATIVE_UINT64, doubleSpace1);
    te_att.write(H5::PredType::NATIVE_UINT64, &te);

    return true;
}

bool H5Wrapper::saveSource(std::shared_ptr<CTSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto srcPath = groupPath + "/" + name;
    auto srcGroup = getGroup(srcPath, true);
    if (!srcGroup)
        return false;

    if (!saveSource(std::static_pointer_cast<Source>(src), name, groupPath))
        return false;

    auto tubeGroup = saveTube(src->tube(), "Tube", srcPath);
    if (!tubeGroup)
        return false;

    //saving AEC profile
    if (auto filter = src->aecFilter(); filter) {
        auto aecPath = srcPath + "/" + "AECData";
        saveDoubleList(filter->mass(), "AECmass", aecPath);
        saveDoubleList(filter->massIntensity(), "AECintensity", aecPath);
        auto aecGroup = getGroup(aecPath, false);
        if (aecGroup) {
            auto aecName = filter->filterName();
            if (aecName.length() == 0) {
                aecName = "Unknown";
            }
            hsize_t strLen = static_cast<hsize_t>(aecName.length());
            H5::StrType stringType(0, strLen);
            H5::DataSpace stringSpace(H5S_SCALAR);
            auto dataUnits = aecGroup->createAttribute("filterName", stringType, stringSpace);
            dataUnits.write(stringType, aecName.c_str());
        }
    }
    //save bowtiefilter
    if (auto filter = src->bowTieFilter(); filter) {
        auto bowtiePath = srcPath + "/" + "BowTieData";
        auto data = filter->data();
        std::vector<double> x, y;
        for (auto [angle, weight] : data) {
            x.push_back(angle);
            y.push_back(weight);
        }
        saveDoubleList(x, "BowTieAngle", bowtiePath);
        saveDoubleList(y, "BowTieWeight", bowtiePath);
        auto bowGroup = getGroup(bowtiePath, false);
        if (bowGroup) {
            auto bowName = filter->filterName();
            if (bowName.length() == 0) {
                bowName = "Unknown";
            }
            hsize_t strLen = static_cast<hsize_t>(bowName.length());
            H5::StrType stringType(0, strLen);
            H5::DataSpace stringSpace(H5S_SCALAR);
            auto dataUnits = bowGroup->createAttribute("filterName", stringType, stringSpace);
            dataUnits.write(stringType, bowName.c_str());
        }
    }

    const hsize_t dim1 = 1;
    H5::DataSpace doubleSpace1(1, &dim1);

    std::map<std::string, double> d1par;
    d1par["sdd"] = src->sourceDetectorDistance();
    d1par["collimation"] = src->collimation();
    d1par["fov"] = src->fieldOfView();
    d1par["gantryTiltAngle"] = src->gantryTiltAngle();
    d1par["startAngle"] = src->startAngle();
    d1par["exposureAngleStep"] = src->exposureAngleStep();
    d1par["scanLenght"] = src->scanLenght();
    d1par["ctdivol"] = src->ctdiVol();

    auto xcarefilter = src->xcareFilter();
    d1par["filterAngle"] = xcarefilter.filterAngle();
    d1par["spanAngle"] = xcarefilter.spanAngle();
    d1par["rampAngle"] = xcarefilter.rampAngle();
    d1par["lowWeight"] = xcarefilter.lowWeight();

    for (auto [key, val] : d1par) {
        auto att = srcGroup->createAttribute(key.c_str(), H5::PredType::NATIVE_DOUBLE, doubleSpace1);
        att.write(H5::PredType::NATIVE_DOUBLE, &val);
    }

    auto pd = src->ctdiPhantomDiameter();
    auto pd_att = srcGroup->createAttribute("ctdiPhantomDiameter", H5::PredType::NATIVE_UINT64, doubleSpace1);
    pd_att.write(H5::PredType::NATIVE_UINT64, &pd);

    auto xc = src->useXCareFilter();
    auto xc_att = srcGroup->createAttribute("useXCareFilter", H5::PredType::NATIVE_HBOOL, doubleSpace1);
    xc_att.write(H5::PredType::NATIVE_HBOOL, &xc);
    return true;
}

bool H5Wrapper::saveSource(std::shared_ptr<CTSpiralSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto srcPath = groupPath + "/" + name;
    auto srcGroup = getGroup(srcPath, true);
    if (!srcGroup)
        return false;

    if (!saveSource(std::static_pointer_cast<CTSource>(src), name, groupPath))
        return false;

    const hsize_t dim1 = 1;
    H5::DataSpace doubleSpace1(1, &dim1);
    auto pitch = src->pitch();
    auto att = srcGroup->createAttribute("pitch", H5::PredType::NATIVE_DOUBLE, doubleSpace1);
    att.write(H5::PredType::NATIVE_DOUBLE, &pitch);
    return true;
}
bool H5Wrapper::saveSource(std::shared_ptr<CTAxialSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto srcPath = groupPath + "/" + name;
    auto srcGroup = getGroup(srcPath, true);
    if (!srcGroup)
        return false;

    if (!saveSource(std::static_pointer_cast<CTSource>(src), name, groupPath))
        return false;

    const hsize_t dim1 = 1;
    H5::DataSpace doubleSpace1(1, &dim1);
    auto step = src->step();
    auto att = srcGroup->createAttribute("step", H5::PredType::NATIVE_DOUBLE, doubleSpace1);
    att.write(H5::PredType::NATIVE_DOUBLE, &step);
    return true;
}

bool H5Wrapper::saveSource(std::shared_ptr<CTSpiralDualSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto srcPath = groupPath + "/" + name;
    auto srcGroup = getGroup(srcPath, true);
    if (!srcGroup)
        return false;

    if (!saveSource(std::static_pointer_cast<CTSource>(src), name, groupPath))
        return false;
    auto tubeB = src->tubeB();
    if (!saveTube(tubeB, "TubeB", srcPath))
        return false;

    //save bowtiefilter
    if (auto filter = src->bowTieFilterB(); filter) {
        auto bowtiePath = srcPath + "/" + "BowTieDataB";
        auto data = filter->data();
        std::vector<double> x, y;
        for (auto [angle, weight] : data) {
            x.push_back(angle);
            y.push_back(weight);
        }
        saveDoubleList(x, "BowTieAngle", bowtiePath);
        saveDoubleList(y, "BowTieWeight", bowtiePath);

        auto bowGroup = getGroup(bowtiePath, false);
        if (bowGroup) {
            auto bowName = filter->filterName();
            if (bowName.length() == 0) {
                bowName = "Unknown";
            }
            hsize_t strLen = static_cast<hsize_t>(bowName.length());
            H5::StrType stringType(0, strLen);
            H5::DataSpace stringSpace(H5S_SCALAR);
            auto dataUnits = bowGroup->createAttribute("filterName", stringType, stringSpace);
            dataUnits.write(stringType, bowName.c_str());
        }
    }

    const hsize_t dim1 = 1;
    H5::DataSpace doubleSpace1(1, &dim1);

    std::map<std::string, double> d1par;
    d1par["sddB"] = src->sourceDetectorDistanceB();
    d1par["fovB"] = src->fieldOfView();
    d1par["startAngleB"] = src->startAngleB();
    d1par["pitch"] = src->pitch();
    d1par["tubeAmas"] = src->tubeAmas();
    d1par["tubeBmas"] = src->tubeBmas();

    for (auto [key, val] : d1par) {
        auto att = srcGroup->createAttribute(key.c_str(), H5::PredType::NATIVE_DOUBLE, doubleSpace1);
        att.write(H5::PredType::NATIVE_DOUBLE, &val);
    }
    return true;
}

bool H5Wrapper::loadSource(std::shared_ptr<DXSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, false);
    if (!group)
        return false;

    if (!loadSource(std::static_pointer_cast<Source>(src), name.c_str(), groupPath.c_str()))
        return false;
    loadTube(src->tube(), "Tube", path.c_str());

    double sdd = 0;
    double dap = 0;
    std::uint64_t te = 0;
    std::array<double, 2> fe;
    std::array<double, 2> ca;
    try {

        auto sdd_attr = group->openAttribute("sdd");
        sdd_attr.read(H5::PredType::NATIVE_DOUBLE, &sdd);
        auto dap_attr = group->openAttribute("dap");
        dap_attr.read(H5::PredType::NATIVE_DOUBLE, &dap);
        auto te_attr = group->openAttribute("totalExposures");
        te_attr.read(H5::PredType::NATIVE_UINT64, &te);
        auto fe_attr = group->openAttribute("fieldSize");
        fe_attr.read(H5::PredType::NATIVE_DOUBLE, fe.data());
        auto ca_attr = group->openAttribute("collimationAngles");
        ca_attr.read(H5::PredType::NATIVE_DOUBLE, ca.data());
    } catch (const H5::DataTypeIException e) {
        auto msg = e.getDetailMsg();
        return false;
    } catch (const H5::AttributeIException e) {
        auto msg = e.getDetailMsg();
        return false;
    }
    src->setSourceDetectorDistance(sdd);
    src->setDap(dap);
    src->setTotalExposures(te);
    src->setFieldSize(fe);
    src->setCollimationAngles(ca);
    return true;
}

bool H5Wrapper::loadSource(std::shared_ptr<CTSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, false);
    if (!group)
        return false;

    if (!loadSource(std::static_pointer_cast<Source>(src), name.c_str(), groupPath.c_str()))
        return false;
    loadTube(src->tube(), "Tube", path.c_str());

    //loading aec data
    auto aecPath = path + "/" + "AECData";
    auto aecGroup = getGroup(aecPath, false);
    if (aecGroup) {
        auto aecMass = loadDoubleList("AECmass", aecPath);
        auto aecIntensity = loadDoubleList("AECintensity", aecPath);
        auto aec = std::make_shared<AECFilter>(aecMass, aecIntensity);
        std::string name;
        if (aecGroup->attrExists("filterName")) {
            auto units_attr = aecGroup->openAttribute("filterName");
            std::size_t units_size = units_attr.getInMemDataSize();
            name.resize(units_size);
            H5::StrType stringType(0, units_size);
            units_attr.read(stringType, name.data());
        }
        aec->setFilterName(name);
        src->setAecFilter(aec);
    }
    //loading bowtie data
    auto bowtiePath = path + "/" + "BowTieData";
    auto bowtieGroup = getGroup(bowtiePath, false);
    if (bowtieGroup) {
        auto angles = loadDoubleList("BowTieAngle", bowtiePath);
        auto weight = loadDoubleList("BowTieWeight", bowtiePath);
        auto bowtie = std::make_shared<BowTieFilter>(angles, weight);
        std::string name;
        if (bowtieGroup->attrExists("filterName")) {
            auto units_attr = bowtieGroup->openAttribute("filterName");
            std::size_t units_size = units_attr.getInMemDataSize();
            name.resize(units_size);
            H5::StrType stringType(0, units_size);
            units_attr.read(stringType, name.data());
        } else {
            name = "Unknown";
        }
        bowtie->setFilterName(name);
        src->setBowTieFilter(bowtie);
    }

    std::map<std::string, double> d1par;
    d1par["sdd"] = src->sourceDetectorDistance();
    d1par["collimation"] = src->collimation();
    d1par["fov"] = src->fieldOfView();
    d1par["gantryTiltAngle"] = src->gantryTiltAngle();
    d1par["startAngle"] = src->startAngle();
    d1par["exposureAngleStep"] = src->exposureAngleStep();
    d1par["scanLenght"] = src->scanLenght();
    d1par["ctdivol"] = src->ctdiVol();

    auto xcarefilter = src->xcareFilter();
    d1par["filterAngle"] = xcarefilter.filterAngle();
    d1par["spanAngle"] = xcarefilter.spanAngle();
    d1par["rampAngle"] = xcarefilter.rampAngle();
    d1par["lowWeight"] = xcarefilter.lowWeight();

    for (auto& [key, val] : d1par) {
        try {
            auto attr = group->openAttribute(key.c_str());
            attr.read(H5::PredType::NATIVE_DOUBLE, &val);
        } catch (...) {
            return false;
        }
    }
    auto pd = src->ctdiPhantomDiameter();
    auto xc = src->useXCareFilter();
    try {

        auto pd_attr = group->openAttribute("ctdiPhantomDiameter");
        pd_attr.read(H5::PredType::NATIVE_UINT64, &pd);
        auto xc_attr = group->openAttribute("useXCareFilter");
        xc_attr.read(H5::PredType::NATIVE_HBOOL, &xc);
    } catch (...) {
        return false;
    }
    src->setCtdiPhantomDiameter(pd);
    src->setUseXCareFilter(xc);
    src->setSourceDetectorDistance(d1par["sdd"]);
    src->setCollimation(d1par["collimation"]);
    src->setFieldOfView(d1par["fov"]);
    src->setGantryTiltAngle(d1par["gantryTiltAngle"]);
    src->setStartAngle(d1par["startAngle"]);
    src->setExposureAngleStep(d1par["exposureAngleStep"]);
    src->setScanLenght(d1par["scanLenght"]);
    src->setCtdiVol(d1par["ctdivol"]);
    xcarefilter.setFilterAngle(d1par["filterAngle"]);
    xcarefilter.setRampAngle(d1par["rampAngle"]);
    xcarefilter.setSpanAngle(d1par["spanAngle"]);
    xcarefilter.setLowWeight(d1par["lowWeight"]);

    return true;
}
bool H5Wrapper::loadSource(std::shared_ptr<CTSpiralSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, false);
    if (!group)
        return false;

    if (!loadSource(std::static_pointer_cast<CTSource>(src), name.c_str(), groupPath.c_str()))
        return false;

    double pitch = src->pitch();
    try {

        auto attr = group->openAttribute("pitch");
        attr.read(H5::PredType::NATIVE_DOUBLE, &pitch);
    } catch (...) {
        return false;
    }
    src->setPitch(pitch);
    return true;
}

bool H5Wrapper::loadSource(std::shared_ptr<CTAxialSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, false);
    if (!group)
        return false;

    if (!loadSource(std::static_pointer_cast<CTSource>(src), name.c_str(), groupPath.c_str()))
        return false;

    double step = src->step();
    try {

        auto attr = group->openAttribute("step");
        attr.read(H5::PredType::NATIVE_DOUBLE, &step);
    } catch (...) {
        return false;
    }
    src->setStep(step);
    return true;
}

bool H5Wrapper::loadSource(std::shared_ptr<CTSpiralDualSource> src, const std::string& name, const std::string& groupPath)
{
    if (!m_file)
        return false;
    auto path = groupPath + "/" + name;
    auto group = getGroup(path, false);
    if (!group)
        return false;

    if (!loadSource(std::static_pointer_cast<CTSource>(src), name.c_str(), groupPath.c_str()))
        return false;
    loadTube(src->tubeB(), "TubeB", path.c_str());

    //loading bowtie data
    auto bowtiePath = path + "/" + "BowTieDataB";
    auto bowtieGroup = getGroup(bowtiePath, false);
    if (bowtieGroup) {
        auto angles = loadDoubleList("BowTieAngle", bowtiePath);
        auto weight = loadDoubleList("BowTieWeight", bowtiePath);
        auto bowtie = std::make_shared<BowTieFilter>(angles, weight);
        std::string name;
        if (bowtieGroup->attrExists("filterName")) {
            auto units_attr = bowtieGroup->openAttribute("filterName");
            std::size_t units_size = units_attr.getInMemDataSize();
            name.resize(units_size);
            H5::StrType stringType(0, units_size);
            units_attr.read(stringType, name.data());
        } else {
            name = "Unknown";
        }
        bowtie->setFilterName(name);
        src->setBowTieFilterB(bowtie);
    }

    std::map<std::string, double> d1par;
    d1par["sddB"] = src->sourceDetectorDistanceB();
    d1par["fovB"] = src->fieldOfView();
    d1par["startAngleB"] = src->startAngleB();
    d1par["pitch"] = src->pitch();
    d1par["tubeAmas"] = src->tubeAmas();
    d1par["tubeBmas"] = src->tubeBmas();
    for (auto& [key, val] : d1par) {
        try {
            auto attr = group->openAttribute(key.c_str());
            attr.read(H5::PredType::NATIVE_DOUBLE, &val);
        } catch (...) {
            return false;
        }
    }
    src->setSourceDetectorDistanceB(d1par["sddB"]);
    src->setFieldOfViewB(d1par["fovB"]);
    src->setStartAngleB(d1par["startAngleB"]);
    src->setPitch(d1par["pitch"]);
    src->setTubeAmas(d1par["tubeAmas"]);
    src->setTubeBmas(d1par["tubeBmas"]);
    return true;
}
