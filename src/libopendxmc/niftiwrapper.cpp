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

Copyright 2025 Erlend Andersen
*/

#include "niftiwrapper.hpp"

#include "vtkNIFTIWriter.h"

NiftiWrapper::NiftiWrapper()
{
}

bool NiftiWrapper::save(const std::string& filepath, vtkSmartPointer<vtkImageData> image, DataContainer::ImageType type)
{
    if (!image) {
        return false;
    }
    auto writer = vtkSmartPointer<vtkNIFTIWriter>::New();
    writer->SetFileDimensionality(3);
    writer->SetNIFTIVersion(1);
    writer->SetDescription(DataContainer::getImageAsString(type).c_str());
    writer->SetFileName(filepath.c_str());
    writer->SetInputData(image);
    writer->Update();
    writer->Write();
    return true;
}