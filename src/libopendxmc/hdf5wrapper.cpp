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
std::string join(const std::vector<std::string>& v, const std::string& sep)
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
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::vector<std::string>& names, std::span<const T> v, const std::array<std::size_t, N>& dims)
{
    if (!file)
        return false;
    if (names.size() < 1)
        return false;
    auto arrname = names.back();
    auto group = getGroup(file, names, true);
    if (!group)
        return false;

    constexpr int rank = static_cast<int>(N);

    auto dataspace = std::make_unique<H5::DataSpace>(rank, dims.data());

    H5::DSetCreatPropList ds_createplist;
    ds_createplist.setChunk(rank, dims.data());
    ds_createplist.setDeflate(6);

    H5::PredType h5type = H5::PredType::NATIVE_DOUBLE;
    if constexpr (std::is_same_v<T, std::uint64_t>)
        h5type = H5::PredType::NATIVE_UINT64;
    else if constexpr (std::is_same_v<T, std::uint8_t>)
        h5type = H5::PredType::NATIVE_UINT8;

    auto path = join(names, "/");

    auto dataset = std::make_unique<H5::DataSet>(group->createDataSet(path.c_str(), h5type, *dataspace, ds_createplist));
    if (!dataset)
        return false;
    dataset->write(v.data(), h5type);
    return true;
}

template <typename T, std::size_t N>
    requires(std::is_integral_v<T> || std::is_floating_point_v<T>)
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::string& path, std::span<const T> v, const std::array<std::size_t, N>& dims)
{
    const auto names = split(path, "/");
    return saveArray(file, names, v, dims);
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

bool HDF5Wrapper::save(std::shared_ptr<DataContainer> data)
{
    if (!m_file || !data)
        return false;
    bool success = true;

    std::vector<std::string> names;
    names.push_back(std::to_string(data->ID()));
    names.push_back("dimensions");
    const auto& dim = data->dimensions();
    {
        std::vector<std::size_t> dimv(dim.cbegin(), dim.cend());
        std::array<std::size_t, 1> dim1 = { 3 };
        success = success && saveArray<std::size_t, 1>(m_file, names, std::span { dim }, {3});
        const auto& spacing = data->spacing();
        names[1] = "spacing";
        success = success && saveArray<double, 1>(m_file, names, std::span { spacing }, {3});
    }
    return true;
}
