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

#include <concepts>
#include <ranges>
#include <span>

std::vector<std::string> split(const std::string& str, const std::string& sep)
{
    std::vector<std::string> s = str | std::ranges::views::split(std::string_view(sep)) | std::ranges::to<std::vector<std::string>>();
    return s;
}
/* std::string join(const std::vector<std::string>& v, const std::string& sep)
{
    std::string s = v | std::ranges::views::join_with(std::string_view(sep)) | std::ranges::to<std::string>();
    return s;
}*/
std::string join(std::span<const std::string> v, const std::string& sep)
{
    std::string s = v | std::ranges::views::join_with(std::string_view(sep)) | std::ranges::to<std::string>();
    return s;
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

    // H5::Exception::dontPrint();
    //  HDF5 only understands vector of char* :-(
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
        else if constexpr (std::is_same_v<T, std::uint8_t>)
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

/* bool slett()
{
    // finding longest string
    std::size_t N = 0;
    for (const auto& s : v)
        N = std::max(N, s.size());
    if (N == 0)
        return false;
    std::vector<std::uint8_t> s(v.size() * N, ' ');
    auto s_beg = s.begin();
    for (std::size_t i = 0; i < v.size(); ++i) {
        std::copy(v[i].cbegin(), v[i].cend(), s_beg);
        s_beg += N;
    }
    std::array<std::size_t, 2> dims = { v.size(), N };
    return saveArray<std::uint8_t, 2>(file, names, s, dims);
}*/
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::string& path, const std::vector<std::string>& v)
{
    const auto names = split(path, "/");
    return saveArray(file, names, v);
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

    std::vector<std::string> names(2);
    names[0] = std::to_string(data->ID());
    {
        names[1] = "dimensions";
        success = success && saveArray<std::size_t, 1>(m_file, names, std::span { dim }, { 3 });
        const auto& spacing = data->spacing();
        names[1] = "spacing";
        success = success && saveArray<double, 1>(m_file, names, std::span { spacing }, { 3 });
    }
    if (const auto& v = data->getDensityArray(); v.size() > 0) {
        names[1] = "density";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getCTArray(); v.size() > 0) {
        names[1] = "ct";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getDoseArray(); v.size() > 0) {
        names[1] = "dose";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getMaterialArray(); v.size() > 0) {
        names[1] = "materialarray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getOrganArray(); v.size() > 0) {
        names[1] = "organarray";
        success = success && saveArray(m_file, names, std::span { v }, dim, true);
    }
    if (const auto& v = data->getOrganNames(); v.size() > 0) {
        names[1] = "organnames";
        success = success && saveArray(m_file, names, v);
    }
    if (const auto& v = data->getMaterials(); v.size() > 0) {
        names[1] = "materialnames";
        std::vector<std::string> mat_names(v.size());
        std::transform(v.cbegin(), v.cend(), mat_names.begin(), [](const auto& n) { return n.name; });
        success = success && saveArray(m_file, names, mat_names);
        names[1] = "materialcomposition";
        std::transform(std::execution::par_unseq, v.cbegin(), v.cend(), mat_names.begin(), [](const auto& n) {
            std::string res;
            for (const auto& [Z, frac] : n.Z) {
                res += dxmc::AtomHandler<double>::toSymbol(Z);
                res += std::to_string(frac);
            }
            return res;
        });
        success = success && saveArray(m_file, names, mat_names);
    }

    return success;
}
