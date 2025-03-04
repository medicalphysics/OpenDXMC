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

#include "vtkImageShiftScale.h"
#include "vtkNIFTIWriter.h"

NiftiWrapper::NiftiWrapper()
{
}

bool NiftiWrapper::save(const std::string& filepath, vtkSmartPointer<vtkImageData> image, DataContainer::ImageType type)
{
    if (!image) {
        return false;
    }

    // lets correct spacing to millimeters
    std::array<double, 3> spacing;
    for (std::size_t i = 0; i < 3; ++i) {
        spacing[i] = image->GetSpacing()[i] * 10;
    }
    image->SetSpacing(spacing.data());

    auto writer = vtkSmartPointer<vtkNIFTIWriter>::New();
    writer->SetFileDimensionality(3);
    writer->SetNIFTIVersion(1);
    writer->SetDescription(DataContainer::getImageAsString(type).c_str());
    writer->SetFileName(filepath.c_str());
    if (type == DataContainer::ImageType::CT) {
        constexpr double intercept = -1024;
        vtkNew<vtkImageShiftScale> shiftScaleFilter;
        shiftScaleFilter->SetOutputScalarTypeToUnsignedShort();
        shiftScaleFilter->SetShift(-intercept);
        shiftScaleFilter->SetInputData(image);

        writer->SetInputConnection(shiftScaleFilter->GetOutputPort());
        writer->SetRescaleIntercept(intercept);
        writer->Update();
        writer->Write();
    } else {
        writer->SetInputData(image);
        writer->Update();
        writer->Write();
    }

    // revert spacing to cm
    for (std::size_t i = 0; i < 3; ++i)
        spacing[i] /= 10;
    image->SetSpacing(spacing.data());
    return true;
}