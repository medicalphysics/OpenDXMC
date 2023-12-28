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

template <typename T, std::size_t N>
    requires std::is_integral_v<T> || std::is_floating_point_v<T>
bool saveArray(std::unique_ptr<H5::H5File>& file, const std::string& path, const std::vector<T>& v, const std::array<std::size_t, N>& dims)
{
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

    return false;
}
