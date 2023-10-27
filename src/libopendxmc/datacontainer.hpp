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

#pragma once

#include <QMetaType>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <dxmc_specialization.hpp>

#include <array>
#include <string>
#include <vector>

class DataContainer {
public:
    enum class ImageType {
        CT,
        Density,
        Material,
        Organ,
        Dose,
        DoseVariance,
        DoseCount
    };

    struct Material {
        std::string name;
        std::string composition;
        std::map<std::uint64_t, double> Z;
    };

    DataContainer();
    void setSpacing(const std::array<double, 3>& cm);
    void setDimensions(const std::array<std::size_t, 3>&);
    void setMaterials(const std::vector<DataContainer::Material>& materials);
    bool setImageArray(ImageType type, const std::vector<double>& image);
    bool setImageArray(ImageType type, const std::vector<std::uint8_t>& image);
    bool setImageArray(ImageType type, const std::vector<std::uint64_t>& image);
    bool setImageArray(ImageType type, vtkImageData* image);
    std::size_t size() const;
    bool hasImage(ImageType type);
    std::uint64_t ID() const;

    vtkSmartPointer<vtkImageData> vtkImage(ImageType);

private:
    std::uint64_t m_uid = 0;
    std::array<double, 3> m_spacing = { 0, 0, 0 };
    std::array<std::size_t, 3> m_dimensions = { 0, 0, 0 };
    std::vector<double> m_ct_array;
    std::vector<double> m_density_array;
    std::vector<std::uint8_t> m_material_array;
    std::vector<std::uint8_t> m_organ_array;
    std::vector<double> m_dose_array;
    std::vector<double> m_dose_variance_array;
    std::vector<std::uint64_t> m_dose_count_array;
    std::vector<DataContainer::Material> m_materials;
};

// Allow std::shared_ptr<DataContainer> to be used in signal/slots
Q_DECLARE_METATYPE(std::shared_ptr<DataContainer>)