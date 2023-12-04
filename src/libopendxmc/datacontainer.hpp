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
#include <map>
#include <string>
#include <vector>

class DataContainer {
public:
    enum class ImageType : int {
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
        std::map<std::uint64_t, double> Z;
    };

    /* struct AECData {
        std::array<double, 3> startPosition;
        std::array<double, 3> stopPosition;
        std::vector<double> weights;
    };*/

    DataContainer();
    void setSpacing(const std::array<double, 3>& cm);
    void setSpacingInmm(const std::array<double, 3>& mm);
    void setDimensions(const std::array<std::size_t, 3>&);
    void setMaterials(const std::vector<DataContainer::Material>& materials);
    void setOrganNames(const std::vector<std::string>& names);
    void setAecData(const std::array<double, 3>& start, const std::array<double, 3>& stop, const std::vector<double>& weights);
    void setAecData(const CTAECFilter&);
    bool setImageArray(ImageType type, const std::vector<double>& image);
    bool setImageArray(ImageType type, const std::vector<std::uint8_t>& image);
    bool setImageArray(ImageType type, const std::vector<std::uint64_t>& image);
    bool setImageArray(ImageType type, vtkSmartPointer<vtkImageData> image);

    std::size_t size() const;
    bool hasImage(ImageType type) const;
    std::uint64_t ID() const;
    const std::array<double, 3>& spacing() const { return m_spacing; }
    const std::array<std::size_t, 3>& dimensions() const { return m_dimensions; }
    const CTAECFilter& aecData() const { return m_aecdata; }
    [[nodiscard]] CTAECFilter calculateAECfilterFromWaterEquivalentDiameter(bool useDensity = false) const;
    [[nodiscard]] std::vector<double> calculateWaterEquivalentDiameter(bool useDensity = false) const;

    vtkSmartPointer<vtkImageData> vtkImage(ImageType);

    const std::vector<double>& getCTArray() const { return m_ct_array; }
    const std::vector<double>& getDensityArray() const { return m_density_array; }
    const std::vector<double>& getDoseArray() const { return m_dose_array; }
    const std::vector<std::uint8_t>& getMaterialArray() const { return m_material_array; }
    const std::vector<std::uint8_t>& getOrganArray() const { return m_organ_array; }

    static std::string getImageAsString(ImageType type);
    std::vector<ImageType> getAvailableImages() const;

    const std::vector<DataContainer::Material>& getMaterials() const { return m_materials; }
    const std::vector<std::string>& getOrganNames() const { return m_organ_names; }

    std::string units(ImageType type) const;
    void setDoseUnits(const std::string& unit);

protected:
    vtkSmartPointer<vtkImageData> generate_vtkImage(ImageType);

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
    CTAECFilter m_aecdata;
    std::vector<std::uint64_t> m_dose_count_array;
    std::vector<DataContainer::Material> m_materials;
    std::vector<std::string> m_organ_names;
    std::map<ImageType, vtkSmartPointer<vtkImageData>> m_vtk_shallow_buffer;
    std::string m_doseUnits = "mGy";
};

// Allow std::shared_ptr<DataContainer> to be used in signal/slots
Q_DECLARE_METATYPE(std::shared_ptr<DataContainer>)