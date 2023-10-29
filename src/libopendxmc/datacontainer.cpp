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

Copyright 2024 Erlend Andersen
*/

#include <datacontainer.hpp>

#include <vtkImageExport.h>
#include <vtkImageImport.h>
#include <vtkSmartPointer.h>

#include <chrono>

std::uint64_t generateID(void)
{
    auto timepoint = std::chrono::system_clock::now();
    auto interval = timepoint.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(interval).count();
}

DataContainer::DataContainer()
{
    m_uid = generateID();
}

std::uint64_t DataContainer::ID() const { return m_uid; }

vtkSmartPointer<vtkImageData> DataContainer::vtkImage(ImageType type)
{
    if (!hasImage(type))
        return nullptr;

    void* data = nullptr;

    auto vtkimport = vtkSmartPointer<vtkImageImport>::New();
    vtkimport->ReleaseDataFlagOn();

    switch (type) {
    case DataContainer::ImageType::CT:
        data = static_cast<void*>(m_ct_array.data());
        vtkimport->SetDataScalarTypeToDouble();
        break;
    case DataContainer::ImageType::Density:
        data = static_cast<void*>(m_density_array.data());
        vtkimport->SetDataScalarTypeToDouble();
        break;
    case DataContainer::ImageType::Material:
        data = static_cast<void*>(m_material_array.data());
        vtkimport->SetDataScalarTypeToUnsignedChar();
        break;
    case DataContainer::ImageType::Organ:
        data = static_cast<void*>(m_organ_array.data());
        vtkimport->SetDataScalarTypeToUnsignedChar();
        break;
    case DataContainer::ImageType::Dose:
        data = static_cast<void*>(m_dose_array.data());
        vtkimport->SetDataScalarTypeToDouble();
        break;
    case DataContainer::ImageType::DoseVariance:
        data = static_cast<void*>(m_dose_variance_array.data());
        vtkimport->SetDataScalarTypeToDouble();
        break;
    case DataContainer::ImageType::DoseCount:
        data = static_cast<void*>(m_dose_count_array.data());
        vtkimport->SetDataScalarType(VTK_UNSIGNED_LONG_LONG);
        break;
    default:
        break;
    }

    if (!data)
        return nullptr;

    vtkimport->SetNumberOfScalarComponents(1);
    std::array<int, 6> extent;
    for (std::size_t i = 0; i < 3; ++i) {
        extent[2 * i] = 0;
        extent[2 * i + 1] = static_cast<int>(m_dimensions[i] - 1);
    }

    vtkimport->SetWholeExtent(extent.data());
    vtkimport->SetDataExtent(extent.data());
    vtkimport->SetDataExtentToWholeExtent();

    vtkimport->SetDataSpacing(m_spacing.data());
    // only a shallow reference
    vtkimport->SetImportVoidPointer(data);
    vtkimport->Update();

    vtkSmartPointer<vtkImageData> image = vtkimport->GetOutput();

    // for returning vtksmart pointer with copy of vtkimagedata
    /* vtkimport->CopyImportVoidPointer(data, size());
    vtkSmartPointer<vtkImageData> image;
    image.TakeReference(vtkimport->GetOutput());
    */
    return image;
}

void DataContainer::setSpacing(const std::array<double, 3>& cm)
{
    m_spacing = cm;
}

void DataContainer::setDimensions(const std::array<std::size_t, 3>& dim)
{
    m_dimensions = dim;
}

void DataContainer::setMaterials(const std::vector<DataContainer::Material>& materials)
{
    m_materials = materials;
}

bool DataContainer::setImageArray(ImageType type, const std::vector<double>& image)
{
    const auto N = size();
    if (N != image.size())
        return false;

    switch (type) {
    case DataContainer::ImageType::CT:
        m_ct_array = image;
        return true;
    case DataContainer::ImageType::Density:
        m_density_array = image;
        return true;
    case DataContainer::ImageType::Dose:
        m_dose_array = image;
        return true;
    case DataContainer::ImageType::DoseVariance:
        m_dose_variance_array = image;
        return true;
    default:
        return false;
    }
    return false;
}

bool DataContainer::setImageArray(ImageType type, const std::vector<std::uint8_t>& image)
{
    const auto N = size();
    if (N != image.size())
        return false;

    switch (type) {
    case DataContainer::ImageType::Material:
        m_material_array = image;
        return true;
    case DataContainer::ImageType::Organ:
        m_organ_array = image;
        return true;
    default:
        return false;
    }
    return false;
}

bool DataContainer::setImageArray(ImageType type, const std::vector<std::uint64_t>& image)
{
    const auto N = size();
    if (N != image.size())
        return false;
    if (type == ImageType::DoseCount) {
        m_dose_count_array = image;
        return true;
    }
    return false;
}

bool DataContainer::setImageArray(ImageType type, vtkImageData* image)
{
    if (image == nullptr)
        return false;

    std::array<int, 3> image_dim;
    image->GetDimensions(image_dim.data());
    if (image_dim[0] != m_dimensions[0] || image_dim[1] != m_dimensions[1] || image_dim[2] != m_dimensions[2])
        return false;

    if (image->GetNumberOfScalarComponents() != 1)
        return false;

    // Checking for correct scalar type
    if (type == ImageType::Material || type == ImageType::Organ) {
        if (image->GetScalarType() != VTK_UNSIGNED_CHAR)
            return false;
    } else if (type == ImageType::DoseCount) {
        if (image->GetScalarType() != VTK_UNSIGNED_LONG_LONG)
            return false;
    } else {
        if (image->GetScalarType() != VTK_DOUBLE)
            return false;
    }

    // Oh horrors, we must have a void pointer to copy data from vtkImageData
    void* buffer = nullptr;
    auto vtkexport = vtkSmartPointer<vtkImageExport>::New();
    vtkexport->SetInputData(image);

    switch (type) {
    case DataContainer::ImageType::CT:
        m_ct_array.resize(size());
        buffer = static_cast<void*>(m_ct_array.data());
        vtkexport->Export(buffer);
        return true;
    case DataContainer::ImageType::Density:
        m_density_array.resize(size());
        buffer = static_cast<void*>(m_density_array.data());
        vtkexport->Export(buffer);
        return true;
    case DataContainer::ImageType::Material:
        m_material_array.resize(size());
        buffer = static_cast<void*>(m_material_array.data());
        vtkexport->Export(buffer);
        return true;
    case DataContainer::ImageType::Organ:
        m_organ_array.resize(size());
        buffer = static_cast<void*>(m_organ_array.data());
        vtkexport->Export(buffer);
        return true;
    case DataContainer::ImageType::Dose:
        m_dose_array.resize(size());
        buffer = static_cast<void*>(m_dose_array.data());
        vtkexport->Export(buffer);
        return true;
    case DataContainer::ImageType::DoseVariance:
        m_dose_variance_array.resize(size());
        buffer = static_cast<void*>(m_dose_variance_array.data());
        vtkexport->Export(buffer);
        return true;
    case DataContainer::ImageType::DoseCount:
        m_dose_count_array.resize(size());
        buffer = static_cast<void*>(m_dose_count_array.data());
        vtkexport->Export(buffer);
        return true;
    default:
        break;
    }
    return false;
}

std::size_t DataContainer::size() const
{
    return m_dimensions[0] * m_dimensions[1] * m_dimensions[2];
}

bool DataContainer::hasImage(ImageType type)
{
    if (m_uid == 0)
        return false;

    const auto N = size();
    std::size_t N_image = 0;
    switch (type) {
    case DataContainer::ImageType::CT:
        N_image = m_ct_array.size();
        break;
    case DataContainer::ImageType::Density:
        N_image = m_density_array.size();
        break;
    case DataContainer::ImageType::Material:
        N_image = m_material_array.size();
        break;
    case DataContainer::ImageType::Organ:
        N_image = m_organ_array.size();
        break;
    case DataContainer::ImageType::Dose:
        N_image = m_dose_array.size();
        break;
    case DataContainer::ImageType::DoseVariance:
        N_image = m_dose_variance_array.size();
        break;
    case DataContainer::ImageType::DoseCount:
        N_image = m_dose_count_array.size();
        break;
    default:
        N_image = 0;
        break;
    }

    if (N == 0 || N != N_image)
        return false;
    return true;
}
