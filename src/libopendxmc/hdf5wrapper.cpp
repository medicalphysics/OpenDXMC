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

#include <hdf5wrapper.hpp>

#include <array>
#include <concepts>
#include <optional>
#include <ranges>
#include <span>
#include <string>

std::vector<std::string> split(const std::string& str, const std::string& sep)
{
    // This works for std ranges, but std::ranges::to is not in GCC yet :(
    //    std::vector<std::string> s = str | std::ranges::views::split(std::string_view(sep)) | std::ranges::to<std::vector<std::string>>();
    //    return s;

    std::vector<std::string> res;
    std::string token = "";
    for (int i = 0; i < str.size(); i++) {
        bool flag = true;
        for (int j = 0; j < sep.size(); j++) {
            if (str[i + j] != sep[j]) {
                flag = false;
            }
        }
        if (flag) {
            if (token.size() > 0) {
                res.push_back(token);
                token = "";
                i += sep.size() - 1;
            }
        } else {
            token += str[i];
        }
    }
    res.push_back(token);
    return res;
}

std::string join(std::span<const std::string> v, const std::string& sep)
{
    // This works for std ranges, but std::ranges::to is not in GCC yet :(
    // std::string s = v | std::ranges::views::join_with(std::string_view(sep)) | std::ranges::to<std::string>();
    // return s;

    std::string res;
    if (v.size() > 0) {
        for (std::size_t i = 0; i < v.size() - 1; ++i) {
            res += v[i] + sep;
        }
        res += v.back();
    }
    return res;
}

std::unique_ptr<H5::Group> getGroup(std::unique_ptr<H5::H5File>& file, const std::vector<std::string>& names, bool create = false)
{
    if (!file)
        return nullptr;
    std::unique_ptr<H5::Group> g = nullptr;
    std::string fullname;
    for (const auto& name : names) {
        fullname += "/" + name;
        if (!file->nameExists(fullname.c_str())) {
            if (create) {
                g = std::make_unique<H5::Group>(file->createGroup(fullname.c_str()));
            } else {
                return nullptr;
            }
        } else {
            g = std::make_unique<H5::Group>(file->openGroup(fullname.c_str()));
        }
    }
    return g;
}
std::unique_ptr<H5::Group> getGroup(std::unique_ptr<H5::H5File>& file, const std::string& path, bool create = false)
{
    const auto names = split(path, "/");
    return getGroup(file, names, create);
}

template <typename T, std::size_t N>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::vector<std::string>& names, std::span<const T> v, const std::array<std::size_t, N>& dims, bool compress = true)
{
    if (!file)
        return false;
    if (names.size() < 1)
        return false;

    // creating groups if needed
    if (names.size() > 1) {
        std::vector<std::string> gnames(names.cbegin(), names.cend() - 1);
        try {
            getGroup(file, gnames, true);
        } catch (...) {
            return false;
        }
    }

    constexpr int rank = static_cast<int>(N);

    std::array<hsize_t, N> h5dims;
    std::copy(dims.cbegin(), dims.cend(), h5dims.begin());
    H5::DataSpace dataspace(rank, h5dims.data());

    H5::PredType h5type = H5::PredType::NATIVE_DOUBLE;
    if constexpr (std::is_same_v<T, std::uint64_t>)
        h5type = H5::PredType::NATIVE_UINT64;
    else if constexpr (std::is_same_v<T, std::uint8_t>)
        h5type = H5::PredType::NATIVE_UINT8;

    auto path = join(names, "/");

    H5::Exception::dontPrint();
    const auto datasize = std::reduce(dims.cbegin(), dims.cend(), std::size_t { 1 }, std::multiplies());
    try {
        if (datasize > 128 && compress) {
            // Modify dataset creation property to enable chunking
            H5::DSetCreatPropList plist;
            plist.setChunk(rank, h5dims.data());
            plist.setDeflate(6);
            auto dataset = file->createDataSet(path.c_str(), h5type, dataspace, plist);
            dataset.write(v.data(), h5type);
        } else {
            auto dataset = file->createDataSet(path.c_str(), h5type, dataspace);
            dataset.write(v.data(), h5type);
        }
    }

    // catch failure caused by the H5File operations
    catch (H5::FileIException error) {
        error.printErrorStack();
        return false;
    }

    // catch failure caused by the DataSet operations
    catch (H5::DataSetIException error) {
        error.printErrorStack();
        return false;
    }

    // catch failure caused by the DataSpace operations
    catch (H5::DataSpaceIException error) {
        error.printErrorStack();
        return false;
    }
    return true;
}

template <typename T, std::size_t N>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::string& path, std::span<const T> v, const std::array<std::size_t, N>& dims, bool compress = false)
{
    const auto names = split(path, "/");
    return saveArray(file, names, v, dims, compress);
}

template <typename T>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::vector<std::string>& names, std::span<const T> v, bool compress = true)
{
    const std::array<std::size_t, 1> dims { v.size() };
    return saveArray<T, 1>(file, names, v, dims, compress);
}

template <typename T, std::size_t N>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::string& path, std::span<const T> v, bool compress = false)
{
    const auto names = split(path, "/");
    return saveArray<T>(file, names, v, compress);
}

bool saveArray(std::unique_ptr<H5::H5File>& file, const std::vector<std::string>& names, const std::vector<std::string>& v)
{
    if (!file)
        return false;
    if (names.size() < 1)
        return false;

    // creating groups if needed
    if (names.size() > 1) {
        std::vector<std::string> gnames(names.cbegin(), names.cend() - 1);
        try {
            getGroup(file, gnames, true);
        } catch (...) {
            return false;
        }
    }

    H5::Exception::dontPrint();

    std::vector<const char*> arr_c_str;
    arr_c_str.reserve(v.size());
    for (const auto& vv : v)
        arr_c_str.push_back(vv.c_str());

    hsize_t str_dimsf[1] { arr_c_str.size() };
    H5::DataSpace dataspace(1, str_dimsf);

    // Variable length string
    H5::StrType datatype(H5::PredType::C_S1, H5T_VARIABLE);

    auto path = join(names, "/");

    try {
        H5::DataSet str_dataset = file->createDataSet(path.c_str(), datatype, dataspace);

        str_dataset.write(arr_c_str.data(), datatype);
    } catch (H5::Exception& err) {
        throw std::runtime_error(std::string("HDF5 Error in ")
            + err.getFuncName()
            + ": "
            + err.getDetailMsg());

        return false;
    }
    return true;
}
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::string& path, const std::vector<std::string>& v)
{
    const auto names = split(path, "/");
    return saveArray(file, names, v);
}

template <typename T>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::string>)
std::vector<T> loadArray(std::unique_ptr<H5::H5File>& file, const std::string& path)
{
    std::vector<T> res;
    if (file->nameExists(path)) {

        H5::DataSet dataset = file->openDataSet(path.c_str());
        H5::DataSpace space = dataset.getSpace();
        int rank = space.getSimpleExtentNdims();
        std::vector<hsize_t> dims(rank);
        int ndims = space.getSimpleExtentDims(dims.data());

        const auto size = std::reduce(dims.cbegin(), dims.cend(), hsize_t { 1 }, std::multiplies());

        res.resize(size);

        auto h5type = H5::PredType::NATIVE_DOUBLE;
        if constexpr (std::is_same_v<T, std::uint8_t>)
            h5type = H5::PredType::NATIVE_UINT8;
        else if constexpr (std::is_same_v<T, std::uint64_t>)
            h5type = H5::PredType::NATIVE_UINT64;

        if constexpr (std::is_same_v<T, std::string>) {
            std::vector<const char*> tmpvect(size, nullptr);
            H5::StrType datatype(H5::PredType::C_S1, H5T_VARIABLE);
            dataset.read(tmpvect.data(), datatype);
            for (std::size_t i = 0; i < tmpvect.size(); ++i) {
                res[i] = tmpvect[i];
            }
        } else {
            dataset.read(res.data(), h5type);
        }
    }
    return res;
}

template <typename T>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_same_v<T, std::string>)
std::vector<T> loadArray(std::unique_ptr<H5::H5File>& file, const std::vector<std::string>& names)
{
    const auto path = join(names, "/");
    return loadArray<T>(file, path);
}

template <typename T>
    requires(std::is_same_v<T, double> || std::is_same_v<T, std::uint64_t>)
void saveAttribute(std::unique_ptr<H5::Group>& group, const std::string& name, std::span<const T> val)
{
    auto size = static_cast<hsize_t>(val.size());
    H5::DataSpace space(1, &size);

    auto type = H5::PredType::NATIVE_DOUBLE;
    if constexpr (std::is_same_v<T, std::uint64_t>)
        type = H5::PredType::NATIVE_UINT64;

    auto att = group->createAttribute(name.c_str(), type, space);
    att.write(type, val.data());
}

template <typename T>
    requires(std::is_same_v<T, double> || std::is_same_v<T, std::uint64_t>)
void saveAttribute(std::unique_ptr<H5::Group>& group, const std::string& name, const T val)
{
    H5::DataSpace space;
    auto type = H5::PredType::NATIVE_DOUBLE;
    if constexpr (std::is_same_v<T, std::uint64_t>)
        type = H5::PredType::NATIVE_UINT64;

    auto att = group->createAttribute(name.c_str(), type, space);
    att.write(type, &val);
}

template <typename T, std::size_t N = 1>
    requires(std::is_same_v<T, double> || std::is_same_v<T, std::uint64_t>)
std::optional<std::array<T, N>> loadAttribute(std::unique_ptr<H5::Group>& group, const std::string& name)
{
    if (group->attrExists(name.c_str())) {
        auto att = group->openAttribute(name.c_str());
        auto space = att.getSpace();
        auto rank = space.getSimpleExtentNdims();
        auto n_elements = space.getSimpleExtentNpoints();
        if (rank > 1 || n_elements != N)
            return std::nullopt;

        auto type_class = att.getTypeClass();

        if constexpr (std::is_same_v<T, double>) {
            auto float_type_class = att.getFloatType();
            auto type_size = float_type_class.getSize();
            if (type_class != H5T_FLOAT || type_size != 8)
                return std::nullopt;
        } else {
            auto int_type_class = att.getIntType();
            auto type_size = int_type_class.getSize();
            if (type_class != H5T_INTEGER || type_size != 8)
                return std::nullopt;
        }

        std::array<T, N> res;
        if constexpr (std::is_same_v<T, double>)
            att.read(H5::PredType::NATIVE_DOUBLE, res.data());
        else
            att.read(H5::PredType::NATIVE_UINT64, res.data());
        return std::make_optional(res);
    }
    return std::nullopt;
}

HDF5Wrapper::HDF5Wrapper(const std::string& path, FileOpenMode mode)
    : m_currentMode(mode)
{
    try {
        if (m_currentMode == FileOpenMode::WriteOver)
            m_file = std::make_unique<H5::H5File>(path.c_str(), H5F_ACC_TRUNC);
        else if (m_currentMode == FileOpenMode::ReadOnly)
            m_file = std::make_unique<H5::H5File>(path.c_str(), H5F_ACC_RDONLY);
    } catch (...) {
        m_file = nullptr;
    }
}

HDF5Wrapper::~HDF5Wrapper()
{
    if (m_file) {
        m_file->close();
        m_file = nullptr;
    }
}

bool HDF5Wrapper::save(std::shared_ptr<DataContainer> data)
{
    if (!m_file || !data)
        return false;
    bool success = true;

    const auto& dim = data->dimensions();

    std::vector<std::string> names(1);
    {
        names[0] = "dimensions";
        success = success && saveArray<std::size_t, 1>(m_file, names, std::span { dim }, { 3 });
        const auto& spacing = data->spacing();
        names[0] = "spacing";
        success = success && saveArray<double, 1>(m_file, names, std::span { spacing }, { 3 });
    }
    if (const auto& v = data->getDensityArray(); v.size() > 0) {
        names[0] = "densityarray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getCTArray(); v.size() > 0) {
        names[0] = "ctarray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getMaterialArray(); v.size() > 0) {
        names[0] = "materialarray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getOrganArray(); v.size() > 0) {
        names[0] = "organarray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getOrganNames(); v.size() > 0) {
        names[0] = "organnames";
        success = success && saveArray(m_file, names, v);
    }
    if (const auto& v = data->getMaterials(); v.size() > 0) {
        names[0] = "materialnames";
        std::vector<std::string> mat_names(v.size());
        std::transform(v.cbegin(), v.cend(), mat_names.begin(), [](const auto& n) { return n.name; });
        success = success && saveArray(m_file, names, mat_names);
        names[0] = "materialcomposition";
        std::transform(std::execution::par_unseq, v.cbegin(), v.cend(), mat_names.begin(), [](const auto& n) {
            std::string res;
            for (const auto& [Z, frac] : n.Z) {
                res += dxmc::AtomHandler::toSymbol(Z);
                res += std::to_string(frac);
            }
            return res;
        });
        success = success && saveArray(m_file, names, mat_names);
    }
    if (const auto& v = data->getDoseArray(); v.size() > 0) {
        names[0] = "dosearray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getDoseVarianceArray(); v.size() > 0) {
        names[0] = "dosevariancearray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getDoseEventCountArray(); v.size() > 0) {
        names[0] = "doseeventcountarray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->aecData(); v.size() > 2) {
        names[0] = "aecweights";
        const auto& w = v.weights();
        success = success && saveArray<double>(m_file, names, w);
        names[0] = "aecstart";
        success = success && saveArray<double>(m_file, names, v.start());
        names[0] = "aecstop";
        success = success && saveArray<double>(m_file, names, v.stop());
    }

    return success;
}

bool HDF5Wrapper::save(DXBeam& beam)
{
    auto beamgroup = getGroup(m_file, "/beams/DXBeams/1", false);
    int teller = 1;
    while (beamgroup) {
        teller++;
        auto path = std::string { "/beams/DXBeams/" } + std::to_string(teller);
        beamgroup = getGroup(m_file, path, false);
    }
    beamgroup = getGroup(m_file, std::string { "/beams/DXBeams/" } + std::to_string(teller), true);

    saveAttribute<double>(beamgroup, "rotation_center", beam.rotationCenter());
    saveAttribute<double>(beamgroup, "source_center_distance", beam.sourcePatientDistance());
    saveAttribute<double>(beamgroup, "primary_angle", beam.primaryAngleDeg());
    saveAttribute<double>(beamgroup, "secondary_angle", beam.secondaryAngleDeg());
    saveAttribute<double>(beamgroup, "source_detector_distance", beam.sourceDetectorDistance());
    saveAttribute<double>(beamgroup, "collimation_angles", beam.collimationAnglesDeg());
    saveAttribute<double>(beamgroup, "DAPvalue", beam.DAPvalue());
    saveAttribute<std::uint64_t>(beamgroup, "number_of_exposures", beam.numberOfExposures());
    saveAttribute<std::uint64_t>(beamgroup, "particles_per_exposure", beam.numberOfParticlesPerExposure());
    saveAttribute<double>(beamgroup, "tube_voltage", beam.tube().voltage());
    saveAttribute<double>(beamgroup, "tube_anode_angle", beam.tube().anodeAngleDeg());
    saveAttribute<double>(beamgroup, "tube_Al_filtration", beam.tube().filtration(13));
    saveAttribute<double>(beamgroup, "tube_Cu_filtration", beam.tube().filtration(29));
    saveAttribute<double>(beamgroup, "tube_Sn_filtration", beam.tube().filtration(50));
    return true;
}

bool HDF5Wrapper::save(CBCTBeam& beam)
{
    auto beamgroup = getGroup(m_file, "/beams/CBCTBeams/1", false);
    int teller = 1;
    while (beamgroup) {
        teller++;
        auto path = std::string { "/beams/CBCTBeams/" } + std::to_string(teller);
        beamgroup = getGroup(m_file, path, false);
    }
    beamgroup = getGroup(m_file, std::string { "/beams/CBCTBeams/" } + std::to_string(teller), true);

    saveAttribute<double>(beamgroup, "isocenter", beam.isocenter());
    saveAttribute<double>(beamgroup, "source_detector_distance", beam.sourceDetectorDistance());
    saveAttribute<double>(beamgroup, "start_angle", beam.startAngle());
    saveAttribute<double>(beamgroup, "stop_angle", beam.stopAngle());
    saveAttribute<double>(beamgroup, "step_angle", beam.stepAngle());
    saveAttribute<double>(beamgroup, "collimation_angles", beam.collimationAnglesDeg());
    saveAttribute<double>(beamgroup, "DAPvalue", beam.DAPvalue());
    saveAttribute<std::uint64_t>(beamgroup, "particles_per_exposure", beam.numberOfParticlesPerExposure());
    saveAttribute<double>(beamgroup, "tube_voltage", beam.tube().voltage());
    saveAttribute<double>(beamgroup, "tube_anode_angle", beam.tube().anodeAngleDeg());
    saveAttribute<double>(beamgroup, "tube_Al_filtration", beam.tube().filtration(13));
    saveAttribute<double>(beamgroup, "tube_Cu_filtration", beam.tube().filtration(29));
    saveAttribute<double>(beamgroup, "tube_Sn_filtration", beam.tube().filtration(50));
    return true;
}

bool HDF5Wrapper::save(CTSequentialBeam& beam)
{
    auto beamgroup = getGroup(m_file, "/beams/CTSequentialBeams/1", false);
    int teller = 1;
    while (beamgroup) {
        teller++;

        auto path = std::string { "/beams/CTSequentialBeams/" } + std::to_string(teller);
        beamgroup = getGroup(m_file, path, false);
    }
    beamgroup = getGroup(m_file, std::string { "/beams/CTSequentialBeams/" } + std::to_string(teller), true);

    saveAttribute<double>(beamgroup, "start_position", beam.position());
    saveAttribute<double>(beamgroup, "scan_normal", beam.scanNormal());
    saveAttribute<double>(beamgroup, "scan_field_view", beam.scanFieldOfView());
    saveAttribute<double>(beamgroup, "source_detector_distance", beam.sourceDetectorDistance());
    saveAttribute<double>(beamgroup, "collimation", beam.collimation());
    saveAttribute<double>(beamgroup, "start_angle", beam.startAngleDeg());
    saveAttribute<double>(beamgroup, "step_angle", beam.stepAngleDeg());
    saveAttribute<double>(beamgroup, "slice_spacing", beam.sliceSpacing());
    saveAttribute<double>(beamgroup, "CTDIw", beam.CTDIw());
    saveAttribute<double>(beamgroup, "CTDIdiameter", beam.CTDIdiameter());
    saveAttribute<double>(beamgroup, "tube_voltage", beam.tube().voltage());
    saveAttribute<double>(beamgroup, "tube_anode_angle", beam.tube().anodeAngleDeg());
    saveAttribute<double>(beamgroup, "tube_Al_filtration", beam.tube().filtration(13));
    saveAttribute<double>(beamgroup, "tube_Cu_filtration", beam.tube().filtration(29));
    saveAttribute<double>(beamgroup, "tube_Sn_filtration", beam.tube().filtration(50));
    saveAttribute<std::uint64_t>(beamgroup, "particles_per_exposure", beam.numberOfParticlesPerExposure());
    saveAttribute<std::uint64_t>(beamgroup, "number_of_slices", beam.numberOfSlices());
    return true;
}

bool HDF5Wrapper::save(CTSpiralBeam& beam)
{
    auto beamgroup = getGroup(m_file, "/beams/CTSpiralBeams/1", false);
    int teller = 1;
    while (beamgroup) {
        teller++;

        auto path = std::string { "/beams/CTSpiralBeams/" } + std::to_string(teller);
        beamgroup = getGroup(m_file, path, false);
    }
    beamgroup = getGroup(m_file, std::string { "/beams/CTSpiralBeams/" } + std::to_string(teller), true);

    saveAttribute<double>(beamgroup, "start_position", beam.startPosition());
    saveAttribute<double>(beamgroup, "stop_position", beam.stopPosition());
    saveAttribute<double>(beamgroup, "scan_field_view", beam.scanFieldOfView());
    saveAttribute<double>(beamgroup, "source_detector_distance", beam.sourceDetectorDistance());
    saveAttribute<double>(beamgroup, "collimation", beam.collimation());
    saveAttribute<double>(beamgroup, "start_angle", beam.startAngleDeg());
    saveAttribute<double>(beamgroup, "step_angle", beam.stepAngleDeg());
    saveAttribute<double>(beamgroup, "pitch", beam.pitch());
    saveAttribute<double>(beamgroup, "CTDIvol", beam.CTDIvol());
    saveAttribute<double>(beamgroup, "CTDIdiameter", beam.CTDIdiameter());
    saveAttribute<double>(beamgroup, "tube_voltage", beam.tube().voltage());
    saveAttribute<double>(beamgroup, "tube_anode_angle", beam.tube().anodeAngleDeg());
    saveAttribute<double>(beamgroup, "tube_Al_filtration", beam.tube().filtration(13));
    saveAttribute<double>(beamgroup, "tube_Cu_filtration", beam.tube().filtration(29));
    saveAttribute<double>(beamgroup, "tube_Sn_filtration", beam.tube().filtration(50));
    saveAttribute<std::uint64_t>(beamgroup, "particles_per_exposure", beam.numberOfParticlesPerExposure());
    return true;
}

bool HDF5Wrapper::save(CTSpiralDualEnergyBeam& beam)
{
    auto beamgroup = getGroup(m_file, "/beams/CTSpiralDualEnergyBeams/1", false);
    int teller = 1;
    while (beamgroup) {
        teller++;
        auto path = std::string { "/beams/CTSpiralDualEnergyBeams/" } + std::to_string(teller);
        beamgroup = getGroup(m_file, path, false);
    }
    beamgroup = getGroup(m_file, std::string { "/beams/CTSpiralDualEnergyBeams/" } + std::to_string(teller), true);

    saveAttribute<double>(beamgroup, "start_position", beam.startPosition());
    saveAttribute<double>(beamgroup, "stop_position", beam.stopPosition());
    saveAttribute<double>(beamgroup, "scan_field_viewA", beam.scanFieldOfViewA());
    saveAttribute<double>(beamgroup, "scan_field_viewB", beam.scanFieldOfViewB());
    saveAttribute<double>(beamgroup, "source_detector_distance", beam.sourceDetectorDistance());
    saveAttribute<double>(beamgroup, "collimation", beam.collimation());
    saveAttribute<double>(beamgroup, "start_angle", beam.startAngleDeg());
    saveAttribute<double>(beamgroup, "step_angle", beam.stepAngleDeg());
    saveAttribute<double>(beamgroup, "pitch", beam.pitch());
    saveAttribute<double>(beamgroup, "CTDIvol", beam.CTDIvol());
    saveAttribute<double>(beamgroup, "CTDIdiameter", beam.CTDIdiameter());
    saveAttribute<double>(beamgroup, "tube_voltageB", beam.tubeB().voltage());
    saveAttribute<double>(beamgroup, "tube_Al_filtrationB", beam.tubeB().filtration(13));
    saveAttribute<double>(beamgroup, "tube_Cu_filtrationB", beam.tubeB().filtration(29));
    saveAttribute<double>(beamgroup, "tube_Sn_filtrationB", beam.tubeB().filtration(50));
    saveAttribute<double>(beamgroup, "tube_anode_angle", beam.tubeA().anodeAngleDeg());
    saveAttribute<double>(beamgroup, "tube_voltageA", beam.tubeA().voltage());
    saveAttribute<double>(beamgroup, "tube_Al_filtrationA", beam.tubeA().filtration(13));
    saveAttribute<double>(beamgroup, "tube_Cu_filtrationA", beam.tubeA().filtration(29));
    saveAttribute<double>(beamgroup, "tube_Sn_filtrationA", beam.tubeA().filtration(50));
    saveAttribute<double>(beamgroup, "tube_masA", beam.relativeMasTubeA());
    saveAttribute<double>(beamgroup, "tube_masB", beam.relativeMasTubeB());
    saveAttribute<std::uint64_t>(beamgroup, "particles_per_exposure", beam.numberOfParticlesPerExposure());
    return true;
}

std::shared_ptr<Beam> loadDXBeam(std::unique_ptr<H5::Group>& group)
{
    auto beam = std::make_shared<Beam>(DXBeam());
    auto& dx = std::get<DXBeam>(*beam);

    auto rotation_center = loadAttribute<double, 3>(group, "rotation_center");
    if (rotation_center)
        dx.setRotationCenter(rotation_center.value());

    auto source_patient_distance = loadAttribute<double, 1>(group, "source_center_distance");
    if (source_patient_distance)
        dx.setSourcePatientDistance(source_patient_distance.value()[0]);

    auto primary_angle = loadAttribute<double, 1>(group, "primary_angle");
    if (primary_angle)
        dx.setPrimaryAngleDeg(primary_angle.value()[0]);

    auto secondary_angle = loadAttribute<double, 1>(group, "secondary_angle");
    if (secondary_angle)
        dx.setSecondaryAngleDeg(secondary_angle.value()[0]);

    auto source_detector_distance = loadAttribute<double, 1>(group, "source_detector_distance");
    if (source_detector_distance)
        dx.setSourceDetectorDistance(source_detector_distance.value()[0]);

    auto collimation_angles = loadAttribute<double, 2>(group, "collimation_angles");
    if (collimation_angles)
        dx.setCollimationAnglesDeg(collimation_angles.value());

    auto DAPvalue = loadAttribute<double, 1>(group, "DAPvalue");
    if (DAPvalue)
        dx.setDAPvalue(DAPvalue.value()[0]);

    auto number_of_exposures = loadAttribute<std::uint64_t, 1>(group, "number_of_exposures");
    if (number_of_exposures)
        dx.setNumberOfExposures(number_of_exposures.value()[0]);

    auto particles_per_exposure = loadAttribute<std::uint64_t, 1>(group, "particles_per_exposure");
    if (particles_per_exposure)
        dx.setNumberOfParticlesPerExposure(particles_per_exposure.value()[0]);

    auto tube_voltage = loadAttribute<double, 1>(group, "tube_voltage");
    if (tube_voltage)
        dx.setTubeVoltage(tube_voltage.value()[0]);

    auto tube_anode_angle = loadAttribute<double, 1>(group, "tube_anode_angle");
    if (tube_anode_angle)
        dx.setTubeAnodeAngleDeg(tube_anode_angle.value()[0]);

    auto tube_Al_filtration = loadAttribute<double, 1>(group, "tube_Al_filtration");
    if (tube_Al_filtration)
        dx.addTubeFiltrationMaterial(13, tube_Al_filtration.value()[0]);

    auto tube_Cu_filtration = loadAttribute<double, 1>(group, "tube_Cu_filtration");
    if (tube_Cu_filtration)
        dx.addTubeFiltrationMaterial(29, tube_Cu_filtration.value()[0]);

    auto tube_Sn_filtration = loadAttribute<double, 1>(group, "tube_Sn_filtration");
    if (tube_Sn_filtration)
        dx.addTubeFiltrationMaterial(50, tube_Sn_filtration.value()[0]);

    return beam;
}

std::shared_ptr<Beam> loadCBCTBeam(std::unique_ptr<H5::Group>& group)
{
    auto beam = std::make_shared<Beam>(CBCTBeam());
    auto& dx = std::get<CBCTBeam>(*beam);

    auto center = loadAttribute<double, 3>(group, "isocenter");
    if (center)
        dx.setIsocenter(center.value());

    auto source_detector_distance = loadAttribute<double, 1>(group, "source_detector_distance");
    if (source_detector_distance)
        dx.setSourceDetectorDistance(source_detector_distance.value()[0]);

    auto start_angle = loadAttribute<double, 1>(group, "start_angle");
    if (start_angle)
        dx.setStartAngle(start_angle.value()[0]);
    auto stop_angle = loadAttribute<double, 1>(group, "stop_angle");
    if (stop_angle)
        dx.setStopAngle(stop_angle.value()[0]);
    auto step_angle = loadAttribute<double, 1>(group, "step_angle");
    if (step_angle)
        dx.setStepAngle(step_angle.value()[0]);

    auto collimation_angles = loadAttribute<double, 2>(group, "collimation_angles");
    if (collimation_angles)
        dx.setCollimationAnglesDeg(collimation_angles.value());

    auto DAPvalue = loadAttribute<double, 1>(group, "DAPvalue");
    if (DAPvalue)
        dx.setDAPvalue(DAPvalue.value()[0]);

    auto particles_per_exposure = loadAttribute<std::uint64_t, 1>(group, "particles_per_exposure");
    if (particles_per_exposure)
        dx.setNumberOfParticlesPerExposure(particles_per_exposure.value()[0]);

    auto tube_voltage = loadAttribute<double, 1>(group, "tube_voltage");
    if (tube_voltage)
        dx.setTubeVoltage(tube_voltage.value()[0]);

    auto tube_anode_angle = loadAttribute<double, 1>(group, "tube_anode_angle");
    if (tube_anode_angle)
        dx.setTubeAnodeAngleDeg(tube_anode_angle.value()[0]);

    auto tube_Al_filtration = loadAttribute<double, 1>(group, "tube_Al_filtration");
    if (tube_Al_filtration)
        dx.addTubeFiltrationMaterial(13, tube_Al_filtration.value()[0]);

    auto tube_Cu_filtration = loadAttribute<double, 1>(group, "tube_Cu_filtration");
    if (tube_Cu_filtration)
        dx.addTubeFiltrationMaterial(29, tube_Cu_filtration.value()[0]);

    auto tube_Sn_filtration = loadAttribute<double, 1>(group, "tube_Sn_filtration");
    if (tube_Sn_filtration)
        dx.addTubeFiltrationMaterial(50, tube_Sn_filtration.value()[0]);

    return beam;
}

std::shared_ptr<Beam> loadCTSequentialBeam(std::unique_ptr<H5::Group>& group)
{
    auto beam = std::make_shared<Beam>(CTSequentialBeam());
    auto& ct = std::get<CTSequentialBeam>(*beam);

    auto position = loadAttribute<double, 3>(group, "position");
    if (position)
        ct.setPosition(position.value());

    auto scan_normal = loadAttribute<double, 3>(group, "scan_normal");
    if (scan_normal)
        ct.setScanNormal(scan_normal.value());

    auto slice_spacing = loadAttribute<double, 1>(group, "slice_spacing");
    if (slice_spacing)
        ct.setSliceSpacing(slice_spacing.value()[0]);

    auto number_of_slices = loadAttribute<std::uint64_t, 1>(group, "number_of_slices");
    if (number_of_slices)
        ct.setNumberOfSlices(number_of_slices.value()[0]);

    auto scan_field_view = loadAttribute<double, 1>(group, "scan_field_view");
    if (scan_field_view)
        ct.setScanFieldOfView(scan_field_view.value()[0]);

    auto source_detector_distance = loadAttribute<double, 1>(group, "source_detector_distance");
    if (source_detector_distance)
        ct.setSourceDetectorDistance(source_detector_distance.value()[0]);

    auto collimation = loadAttribute<double, 1>(group, "collimation");
    if (collimation)
        ct.setCollimation(collimation.value()[0]);

    auto start_angle = loadAttribute<double, 1>(group, "start_angle");
    if (start_angle)
        ct.setStartAngleDeg(start_angle.value()[0]);

    auto step_angle = loadAttribute<double, 1>(group, "step_angle");
    if (step_angle)
        ct.setStepAngleDeg(step_angle.value()[0]);

    auto ctdi = loadAttribute<double, 1>(group, "CTDIw");
    if (ctdi)
        ct.setCTDIw(ctdi.value()[0]);

    auto ctdid = loadAttribute<double, 1>(group, "CTDIDiameter");
    if (ctdid)
        ct.setCTDIdiameter(ctdid.value()[0]);

    auto particles_per_exposure = loadAttribute<std::uint64_t, 1>(group, "particles_per_exposure");
    if (particles_per_exposure)
        ct.setNumberOfParticlesPerExposure(particles_per_exposure.value()[0]);

    auto tube_voltage = loadAttribute<double, 1>(group, "tube_voltage");
    if (tube_voltage)
        ct.setTubeVoltage(tube_voltage.value()[0]);

    auto tube_anode_angle = loadAttribute<double, 1>(group, "tube_anode_angle");
    if (tube_anode_angle)
        ct.setTubeAnodeAngleDeg(tube_anode_angle.value()[0]);

    auto tube_Al_filtration = loadAttribute<double, 1>(group, "tube_Al_filtration");
    if (tube_Al_filtration)
        ct.addTubeFiltrationMaterial(13, tube_Al_filtration.value()[0]);

    auto tube_Cu_filtration = loadAttribute<double, 1>(group, "tube_Cu_filtration");
    if (tube_Cu_filtration)
        ct.addTubeFiltrationMaterial(29, tube_Cu_filtration.value()[0]);

    auto tube_Sn_filtration = loadAttribute<double, 1>(group, "tube_Sn_filtration");
    if (tube_Sn_filtration)
        ct.addTubeFiltrationMaterial(50, tube_Sn_filtration.value()[0]);

    return beam;
}

std::shared_ptr<Beam> loadCTSpiralBeam(std::unique_ptr<H5::Group>& group)
{
    auto beam = std::make_shared<Beam>(CTSpiralBeam());
    auto& ct = std::get<CTSpiralBeam>(*beam);

    auto start_position = loadAttribute<double, 3>(group, "start_position");
    if (start_position)
        ct.setStartPosition(start_position.value());

    auto stop_position = loadAttribute<double, 3>(group, "stop_position");
    if (stop_position)
        ct.setStopPosition(stop_position.value());

    auto scan_field_view = loadAttribute<double, 1>(group, "scan_field_view");
    if (scan_field_view)
        ct.setScanFieldOfView(scan_field_view.value()[0]);

    auto source_detector_distance = loadAttribute<double, 1>(group, "source_detector_distance");
    if (source_detector_distance)
        ct.setSourceDetectorDistance(source_detector_distance.value()[0]);

    auto collimation = loadAttribute<double, 1>(group, "collimation");
    if (collimation)
        ct.setCollimation(collimation.value()[0]);

    auto start_angle = loadAttribute<double, 1>(group, "start_angle");
    if (start_angle)
        ct.setStartAngleDeg(start_angle.value()[0]);

    auto step_angle = loadAttribute<double, 1>(group, "step_angle");
    if (step_angle)
        ct.setStepAngleDeg(step_angle.value()[0]);

    auto pitch = loadAttribute<double, 1>(group, "pitch");
    if (pitch)
        ct.setPitch(pitch.value()[0]);

    auto ctdi = loadAttribute<double, 1>(group, "CTDIvol");
    if (ctdi)
        ct.setCTDIvol(ctdi.value()[0]);

    auto ctdid = loadAttribute<double, 1>(group, "CTDIDiameter");
    if (ctdid)
        ct.setCTDIdiameter(ctdid.value()[0]);

    auto particles_per_exposure = loadAttribute<std::uint64_t, 1>(group, "particles_per_exposure");
    if (particles_per_exposure)
        ct.setNumberOfParticlesPerExposure(particles_per_exposure.value()[0]);

    auto tube_voltage = loadAttribute<double, 1>(group, "tube_voltage");
    if (tube_voltage)
        ct.setTubeVoltage(tube_voltage.value()[0]);

    auto tube_anode_angle = loadAttribute<double, 1>(group, "tube_anode_angle");
    if (tube_anode_angle)
        ct.setTubeAnodeAngleDeg(tube_anode_angle.value()[0]);

    auto tube_Al_filtration = loadAttribute<double, 1>(group, "tube_Al_filtration");
    if (tube_Al_filtration)
        ct.addTubeFiltrationMaterial(13, tube_Al_filtration.value()[0]);

    auto tube_Cu_filtration = loadAttribute<double, 1>(group, "tube_Cu_filtration");
    if (tube_Cu_filtration)
        ct.addTubeFiltrationMaterial(29, tube_Cu_filtration.value()[0]);

    auto tube_Sn_filtration = loadAttribute<double, 1>(group, "tube_Sn_filtration");
    if (tube_Sn_filtration)
        ct.addTubeFiltrationMaterial(50, tube_Sn_filtration.value()[0]);

    return beam;
}

std::shared_ptr<Beam> loadCTSpiralDualEnergyBeam(std::unique_ptr<H5::Group>& group)
{
    auto beam = std::make_shared<Beam>(CTSpiralDualEnergyBeam());
    auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);

    auto start_position = loadAttribute<double, 3>(group, "start_position");
    if (start_position)
        ct.setStartPosition(start_position.value());

    auto stop_position = loadAttribute<double, 3>(group, "stop_position");
    if (stop_position)
        ct.setStopPosition(stop_position.value());

    auto scan_field_view = loadAttribute<double, 1>(group, "scan_field_viewA");
    if (scan_field_view)
        ct.setScanFieldOfViewA(scan_field_view.value()[0]);
    scan_field_view = loadAttribute<double, 1>(group, "scan_field_viewB");
    if (scan_field_view)
        ct.setScanFieldOfViewB(scan_field_view.value()[0]);

    auto source_detector_distance = loadAttribute<double, 1>(group, "source_detector_distance");
    if (source_detector_distance)
        ct.setSourceDetectorDistance(source_detector_distance.value()[0]);

    auto collimation = loadAttribute<double, 1>(group, "collimation");
    if (collimation)
        ct.setCollimation(collimation.value()[0]);

    auto start_angle = loadAttribute<double, 1>(group, "start_angle");
    if (start_angle)
        ct.setStartAngleDeg(start_angle.value()[0]);

    auto step_angle = loadAttribute<double, 1>(group, "step_angle");
    if (step_angle)
        ct.setStepAngleDeg(step_angle.value()[0]);

    auto pitch = loadAttribute<double, 1>(group, "pitch");
    if (pitch)
        ct.setPitch(pitch.value()[0]);

    auto ctdi = loadAttribute<double, 1>(group, "CTDIvol");
    if (ctdi)
        ct.setCTDIvol(ctdi.value()[0]);

    auto ctdid = loadAttribute<double, 1>(group, "CTDIDiameter");
    if (ctdid)
        ct.setCTDIdiameter(ctdid.value()[0]);

    auto particles_per_exposure = loadAttribute<std::uint64_t, 1>(group, "particles_per_exposure");
    if (particles_per_exposure)
        ct.setNumberOfParticlesPerExposure(particles_per_exposure.value()[0]);

    auto tube_anode_angle = loadAttribute<double, 1>(group, "tube_anode_angle");
    if (tube_anode_angle)
        ct.setTubesAnodeAngleDeg(tube_anode_angle.value()[0]);

    auto tube_voltage = loadAttribute<double, 1>(group, "tube_voltageA");
    if (tube_voltage)
        ct.setTubeAVoltage(tube_voltage.value()[0]);

    auto tube_Al_filtration = loadAttribute<double, 1>(group, "tube_Al_filtrationA");
    if (tube_Al_filtration)
        ct.addTubeAFiltrationMaterial(13, tube_Al_filtration.value()[0]);

    auto tube_Cu_filtration = loadAttribute<double, 1>(group, "tube_Cu_filtrationA");
    if (tube_Cu_filtration)
        ct.addTubeAFiltrationMaterial(29, tube_Cu_filtration.value()[0]);

    auto tube_Sn_filtration = loadAttribute<double, 1>(group, "tube_Sn_filtrationA");
    if (tube_Sn_filtration)
        ct.addTubeAFiltrationMaterial(50, tube_Sn_filtration.value()[0]);

    tube_voltage = loadAttribute<double, 1>(group, "tube_voltageB");
    if (tube_voltage)
        ct.setTubeBVoltage(tube_voltage.value()[0]);

    tube_Al_filtration = loadAttribute<double, 1>(group, "tube_Al_filtrationB");
    if (tube_Al_filtration)
        ct.addTubeBFiltrationMaterial(13, tube_Al_filtration.value()[0]);

    tube_Cu_filtration = loadAttribute<double, 1>(group, "tube_Cu_filtrationB");
    if (tube_Cu_filtration)
        ct.addTubeBFiltrationMaterial(29, tube_Cu_filtration.value()[0]);

    tube_Sn_filtration = loadAttribute<double, 1>(group, "tube_Sn_filtrationB");
    if (tube_Sn_filtration)
        ct.addTubeBFiltrationMaterial(50, tube_Sn_filtration.value()[0]);

    auto tube_mas = loadAttribute<double, 1>(group, "tube_masA");
    if (tube_mas)
        ct.setRelativeMasTubeA(tube_mas.value()[0]);
    tube_mas = loadAttribute<double, 1>(group, "tube_masB");
    if (tube_mas)
        ct.setRelativeMasTubeB(tube_mas.value()[0]);

    return beam;
}

bool HDF5Wrapper::save(std::shared_ptr<BeamActorContainer> beam)
{
    if (!m_file || !beam)
        return false;

    if (std::holds_alternative<DXBeam>(*beam->getBeam())) {
        return save(std::get<DXBeam>(*beam->getBeam()));
    } else if (std::holds_alternative<CTSpiralBeam>(*beam->getBeam())) {
        return save(std::get<CTSpiralBeam>(*beam->getBeam()));
    } else if (std::holds_alternative<CTSpiralDualEnergyBeam>(*beam->getBeam())) {
        return save(std::get<CTSpiralDualEnergyBeam>(*beam->getBeam()));
    } else if (std::holds_alternative<CBCTBeam>(*beam->getBeam())) {
        return save(std::get<CBCTBeam>(*beam->getBeam()));
    } else if (std::holds_alternative<CTSequentialBeam>(*beam->getBeam())) {
        return save(std::get<CTSequentialBeam>(*beam->getBeam()));
    }

    return false;
}

std::shared_ptr<DataContainer> HDF5Wrapper::load()
{
    auto res = std::make_shared<DataContainer>();
    std::string name;
    {
        auto v = loadArray<std::size_t>(m_file, "dimensions");
        if (v.size() == 3) {
            res->setDimensions({ v[0], v[1], v[2] });
        } else {
            return nullptr;
        }
    }
    {
        auto v = loadArray<double>(m_file, "spacing");
        if (v.size() == 3) {
            res->setSpacing({ v[0], v[1], v[2] });
        } else {
            return nullptr;
        }
    }
    {
        auto v = loadArray<std::uint8_t>(m_file, "materialarray");
        if (v.size() == res->size()) {
            res->setImageArray(DataContainer::ImageType::Material, v);
            auto material_names = loadArray<std::string>(m_file, "materialnames");
            auto material_comp = loadArray<std::string>(m_file, "materialcomposition");
            if (material_names.size() == material_comp.size()) {
                std::vector<DataContainer::Material> materials(material_names.size());
                for (std::size_t i = 0; i < material_names.size(); ++i) {
                    materials[i].name = material_names[i];
                    materials[i].Z = dxmc::Material<5>::parseCompoundStr(material_comp[i]);
                }
                res->setMaterials(materials);
            } else {
                return nullptr;
            }

        } else {
            return nullptr;
        }
        v = loadArray<std::uint8_t>(m_file, "organarray");

        if (v.size() == res->size()) {
            res->setImageArray(DataContainer::ImageType::Organ, v);
            auto o_names = loadArray<std::string>(m_file, "organnames");
            res->setOrganNames(o_names);
        }
    }
    {
        auto v = loadArray<double>(m_file, "densityarray");
        if (v.size() == res->size()) {
            res->setImageArray(DataContainer::ImageType::Density, v);
        } else {
            return nullptr;
        }
        v = loadArray<double>(m_file, "ctarray");
        if (v.size() == res->size())
            res->setImageArray(DataContainer::ImageType::CT, v);
        v = loadArray<double>(m_file, "dosearray");
        if (v.size() == res->size())
            res->setImageArray(DataContainer::ImageType::Dose, v);
        v = loadArray<double>(m_file, "dosevariancearray");
        if (v.size() == res->size())
            res->setImageArray(DataContainer::ImageType::DoseVariance, v);
    }
    {
        auto v = loadArray<double>(m_file, "doseeventcountarray");
        if (v.size() == res->size())
            res->setImageArray(DataContainer::ImageType::DoseCount, v);
    }
    {
        auto start = loadArray<double>(m_file, "aecstart");
        auto stop = loadArray<double>(m_file, "aecstop");
        auto weights = loadArray<double>(m_file, "weights");
        if (start.size() == 3 && stop.size() == 3 && weights.size() > 2) {
            res->setAecData({ start[0], start[1], start[2] }, { stop[0], stop[1], stop[2] }, weights);
        }
    }

    return res;
}

std::vector<std::shared_ptr<BeamActorContainer>> HDF5Wrapper::loadBeams()
{
    std::vector<std::shared_ptr<BeamActorContainer>> res;
    auto beamgroup = getGroup(m_file, "beams", false);
    if (!beamgroup)
        return res;

    const std::array<std::string, 5> beamnames = { "DXBeams", "CTSpiralBeams", "CTDualEnergySpiralBeams", "CBCTBeams", "CTSequentialBeams" };

    for (int i = 0; i < beamnames.size(); ++i)
    // for (const auto& beamname : beamnames)
    {
        const auto beamname = beamnames[i];
        const std::string beamgroupname = "beams/" + beamname;
        auto bgroup = getGroup(m_file, beamgroupname, false);
        if (bgroup) {
            int teller = 1;
            std::string beampath = beamgroupname + "/" + std::to_string(teller);
            auto dx = getGroup(m_file, beampath);
            while (dx) {
                if (i == 0) {
                    if (auto beam = loadDXBeam(dx); beam) {
                        auto actor = std::make_shared<BeamActorContainer>(beam);
                        res.push_back(actor);
                    }
                }
                if (i == 1) {
                    if (auto beam = loadCTSpiralBeam(dx); beam) {
                        auto actor = std::make_shared<BeamActorContainer>(beam);
                        res.push_back(actor);
                    }
                }
                if (i == 2) {
                    if (auto beam = loadCTSpiralDualEnergyBeam(dx); beam) {
                        auto actor = std::make_shared<BeamActorContainer>(beam);
                        res.push_back(actor);
                    }
                }
                if (i == 3) {
                    if (auto beam = loadCBCTBeam(dx); beam) {
                        auto actor = std::make_shared<BeamActorContainer>(beam);
                        res.push_back(actor);
                    }
                }
                if (i == 4) {
                    if (auto beam = loadCTSequentialBeam(dx); beam) {
                        auto actor = std::make_shared<BeamActorContainer>(beam);
                        res.push_back(actor);
                    }
                }
                teller++;
                beampath = beamgroupname + "/" + std::to_string(teller);
                dx = getGroup(m_file, beampath);
            }
        }
    }
    return res;
}
