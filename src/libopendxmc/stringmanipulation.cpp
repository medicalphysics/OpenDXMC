
/* This file is part of OpenDXMC.

OpenDXMC is free software : you can redistribute itand /or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenDXMC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenDXMC.If not, see < https://www.gnu.org/licenses/>.

Copyright 2019 Erlend Andersen
*/

#include "opendxmc/stringmanipulation.h"

std::string string_trim(const std::string& str, const std::string& chars)
{
    auto strc = str;
    strc.erase(0, strc.find_first_not_of(chars));
    strc.erase(strc.find_last_not_of(chars) + 1);
    return strc;
}

std::vector<std::string> string_split(const std::string& text, char sep)
{
    // this function splits a string into a vector based on a sep character
    // it will skip empty tokens
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        if (end != start) {
            tokens.push_back(text.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start) {
        tokens.push_back(text.substr(start));
    }
    return tokens;
}
