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
    m_aecdata.setData({ 0, 0, 0 }, { 0, 0, 0 }, { 1.0, 1.0 });
}

std::uint64_t DataContainer::ID() const { return m_uid; }

std::vector<double> aecProfileFromWED(const std::vector<double>& wed)
{
    auto aec = wed;

    constexpr auto u_water = 0.2; // about for 60-70 kev photons
    for (auto& v : aec)
        v = std::exp(u_water * v);

    return aec;
}

CTAECFilter DataContainer::calculateAECfilterFromWaterEquivalentDiameter(bool useDensity) const
{
    double l = m_spacing[2] * m_dimensions[2] / 2.0;
    std::array<double, 3> start = { 0, 0, -l };
    std::array<double, 3> stop = { 0, 0, l };

    CTAECFilter filter(start, stop, aecProfileFromWED(calculateWaterEquivalentDiameter(useDensity)));

    return filter;
}

std::vector<double> DataContainer::calculateWaterEquivalentDiameter(bool useDensity) const
{
    DataContainer::ImageType type = DataContainer::ImageType::CT;
    if (useDensity)
        type = DataContainer::ImageType::Density;
    else if (!hasImage(DataContainer::ImageType::CT))
        type = DataContainer::ImageType::Density;

    std::vector<double> r;
    if (!hasImage(type))
        return r;
    const auto& dim = dimensions();
    const auto dd = spacing();
    const auto step = dim[0] * dim[1];

    const auto& arr = type == DataContainer::ImageType::CT ? getCTArray() : getDensityArray();

    r.reserve(dim[2]);

    for (std::size_t i = 0; i < dim[2]; ++i) {
        const auto start = step * i;
        const auto stop = start + step;
        if (type == DataContainer::ImageType::CT) {
            const auto mean = std::reduce(std::execution::par_unseq, arr.cbegin() + start, arr.cbegin() + stop, 0.0) / step;
            const double Aw = (mean / 1000 + 1) * step * dd[0] * dd[1];
            const auto Dw = 2 * std::sqrt(std::max(Aw, 0.0) / std::numbers::pi_v<double>);
            r.push_back(Dw);
        } else {
            const auto sum = std::reduce(std::execution::par_unseq, arr.cbegin() + start, arr.cbegin() + stop, 0.0);
            const auto Aw = sum * dd[0] * dd[1];
            const auto Dw = 2 * std::sqrt(Aw / std::numbers::pi_v<double>);
            r.push_back(Dw);
        }
    }

    return r;
}

vtkSmartPointer<vtkImageData> DataContainer::vtkImage(ImageType type)
{
    if (!m_vtk_shallow_buffer.contains(type)) {
        m_vtk_shallow_buffer[type] = generate_vtkImage(type);
    }
    return m_vtk_shallow_buffer[type];
}

vtkSmartPointer<vtkImageData> DataContainer::generate_vtkImage(ImageType type)
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

    std::array<double, 3> origin = {
        -(m_spacing[0] * m_dimensions[0]) / 2,
        -(m_spacing[1] * m_dimensions[1]) / 2,
        -(m_spacing[2] * m_dimensions[2]) / 2
    };

    image->SetOrigin(origin.data());
    return image;
}

void DataContainer::setOrganNames(const std::vector<std::string>& names)
{
    m_organ_names = names;
}

std::vector<DataContainer::ImageType> DataContainer::getAvailableImages() const
{

    std::array types = {
        ImageType::CT,
        ImageType::Density,
        ImageType::Organ,
        ImageType::Material,
        ImageType::Dose,
        ImageType::DoseVariance,
        ImageType::DoseCount
    };
    std::vector<ImageType> type_avail;
    for (const auto t : types)
        if (hasImage(t))
            type_avail.push_back(t);

    return type_avail;
}

std::string DataContainer::getImageAsString(ImageType type)
{

    std::string name = "Unknown";
    switch (type) {
    case DataContainer::ImageType::CT:
        name = "CT";
        break;
    case DataContainer::ImageType::Density:
        name = "Density";
        break;
    case DataContainer::ImageType::Material:
        name = "Material";
        break;
    case DataContainer::ImageType::Organ:
        name = "Organ";
        break;
    case DataContainer::ImageType::Dose:
        name = "Dose";
        break;
    case DataContainer::ImageType::DoseVariance:
        name = "Dose variance";
        break;
    case DataContainer::ImageType::DoseCount:
        name = "Dose tally";
        break;
    default:
        break;
    }
    return name;
}

void DataContainer::setSpacing(const std::array<double, 3>& cm)
{
    m_spacing = cm;
    m_vtk_shallow_buffer.clear();
}

void DataContainer::setSpacingInmm(const std::array<double, 3>& mm)
{
    m_spacing = mm;
    for (auto& s : m_spacing)
        s /= 10;
    m_vtk_shallow_buffer.clear();
}

void DataContainer::setDimensions(const std::array<std::size_t, 3>& dim)
{
    m_dimensions = dim;
    m_vtk_shallow_buffer.clear();
}

void DataContainer::setMaterials(const std::vector<DataContainer::Material>& materials)
{
    m_materials = materials;
}

void DataContainer::setAecData(const std::array<double, 3>& start, const std::array<double, 3>& stop, const std::vector<double>& weights)
{
    m_aecdata.setData(start, stop, weights);
}

void DataContainer::setAecData(const CTAECFilter& d)
{
    m_aecdata = d;
}

bool DataContainer::setImageArray(ImageType type, const std::vector<double>& image)
{
    const auto N = size();
    if (N != image.size())
        return false;

    m_vtk_shallow_buffer.erase(type);

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

    m_vtk_shallow_buffer.erase(type);

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

    m_vtk_shallow_buffer.erase(type);

    if (type == ImageType::DoseCount) {
        m_dose_count_array = image;
        return true;
    }
    return false;
}

bool DataContainer::setImageArray(ImageType type, vtkSmartPointer<vtkImageData> image)
{
    if (image == nullptr)
        return false;

    m_vtk_shallow_buffer.erase(type);

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
    vtkexport->ReleaseDataFlagOn();
    vtkexport->SetInputData(image);

    switch (type) {
    case DataContainer::ImageType::CT:
        buffer = vtkexport->GetPointerToData();
        m_ct_array = std::vector<double>(static_cast<double*>(buffer), static_cast<double*>(buffer) + size());
        return true;
    case DataContainer::ImageType::Density:
        buffer = vtkexport->GetPointerToData();
        m_density_array = std::vector<double>(static_cast<double*>(buffer), static_cast<double*>(buffer) + size());
        return true;
    case DataContainer::ImageType::Material:
        buffer = vtkexport->GetPointerToData();
        m_material_array = std::vector<std::uint8_t>(static_cast<std::uint8_t*>(buffer), static_cast<std::uint8_t*>(buffer) + size());
        return true;
    case DataContainer::ImageType::Organ:
        buffer = vtkexport->GetPointerToData();
        m_organ_array = std::vector<std::uint8_t>(static_cast<std::uint8_t*>(buffer), static_cast<std::uint8_t*>(buffer) + size());
        return true;
    case DataContainer::ImageType::Dose:
        buffer = vtkexport->GetPointerToData();
        m_dose_array = std::vector<double>(static_cast<double*>(buffer), static_cast<double*>(buffer) + size());
        return true;
    case DataContainer::ImageType::DoseVariance:
        buffer = vtkexport->GetPointerToData();
        m_dose_variance_array = std::vector<double>(static_cast<double*>(buffer), static_cast<double*>(buffer) + size());
        return true;
    case DataContainer::ImageType::DoseCount:
        buffer = vtkexport->GetPointerToData();
        m_dose_count_array = std::vector<std::uint64_t>(static_cast<std::uint64_t*>(buffer), static_cast<std::uint64_t*>(buffer) + size());
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
void DataContainer::setDoseUnits(const std::string& unit)
{
    m_doseUnits = unit;
}
std::string DataContainer::units(ImageType type) const
{
    switch (type) {
    case DataContainer::ImageType::CT:
        return "HU";
    case DataContainer::ImageType::Density:
        return "g/cm3";
    case DataContainer::ImageType::Material:
        return "";
    case DataContainer::ImageType::Organ:
        return "";
    case DataContainer::ImageType::Dose:
        return m_doseUnits;
    case DataContainer::ImageType::DoseVariance:
        return m_doseUnits + "^2";
    case DataContainer::ImageType::DoseCount:
        return "N events";
    default:
        return "";
    }
}

bool DataContainer::hasImage(ImageType type) const
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
