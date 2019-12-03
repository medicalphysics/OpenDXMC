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

#include <hdf5_hl.h>
#include <string>

class H5Wrapper
{
public:
	H5Wrapper(const std::string& filePath);
	~H5Wrapper();
	hid_t getGroup(const std::string& groupPath, bool create = true);
	bool saveImage()

protected:
private:
	hid_t m_fileID = -1;


};