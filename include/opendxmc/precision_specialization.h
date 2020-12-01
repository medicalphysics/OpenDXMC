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

#pragma once

#include <array>

// alter this variable to use either float or double precision
// for MC calculations
using floating = float;



//helper if we need std arrays in other type than floating
template <typename Dest, typename Src, std::size_t N, std::size_t... Is>
auto convert_array_to_impl(const std::array<Src, N>& src, std::index_sequence<Is...>)
{
    return std::array<Dest, N> { { static_cast<Dest>(src[Is])... } };
}

template <typename Dest, typename Src, std::size_t N>
auto convert_array_to(const std::array<Src, N>& src)
{
    return convert_array_to_impl<Dest>(src, std::make_index_sequence<N>());
}


