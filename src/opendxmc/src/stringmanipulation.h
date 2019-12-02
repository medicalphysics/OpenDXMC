
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
#pragma once
#ifndef OPENDXMC_STRINGMANIPULATION_H
#define OPENDXMC_STRINGMANIPULATION_H
#include <string>
#include <vector>

std::string trim(const std::string& str, const std::string& chars = "\t\n\v\f\r ");
std::vector<std::string> string_split(const std::string& text, char sep);


#endif //OPENDXMC_STRINGMANIPULATION_H