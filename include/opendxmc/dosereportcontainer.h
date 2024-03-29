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

#include <memory>
#include <string>
#include <vector>

#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/imagecontainer.h"

struct DoseReportElement {
    std::size_t voxels = 0;
    floating mass = 0;
    floating volume = 0;
    floating dose = 0;
    floating variance = 0;
    floating doseStd = 0;
    floating doseMax = 0;
    std::size_t ID = 0;
    std::size_t nEvents = 0;
    std::string name;
};

class DoseReportContainer {
public:
    DoseReportContainer()
    {
        m_materialValues = std::make_shared<std::vector<DoseReportElement>>();
        m_organValues = std::make_shared<std::vector<DoseReportElement>>();
    }
    DoseReportContainer(
        const std::vector<Material>& materialMap,
        std::shared_ptr<MaterialImageContainer> materialImage,
        std::shared_ptr<DensityImageContainer> densityImage,
        std::shared_ptr<DoseImageContainer> doseImage,
        std::shared_ptr<TallyImageContainer> tallyImage,
        std::shared_ptr<VarianceImageContainer> varianceImage);

    DoseReportContainer(
        const std::vector<Material>& materialMap,
        const std::vector<std::string>& organMap,
        std::shared_ptr<MaterialImageContainer> materialImage,
        std::shared_ptr<OrganImageContainer> organImage,
        std::shared_ptr<DensityImageContainer> densityImage,
        std::shared_ptr<DoseImageContainer> doseImage,
        std::shared_ptr<TallyImageContainer> tallyImage,
        std::shared_ptr<VarianceImageContainer> varianceImage);
    std::shared_ptr<std::vector<DoseReportElement>> organData() const { return m_organValues; }
    std::shared_ptr<std::vector<DoseReportElement>> materialData() const { return m_materialValues; }
    const std::string& doseUnits(void) const { return m_doseUnits; }

protected:
    template <typename RegionImage>
    std::vector<DoseReportElement> createData(const std::vector<std::string>& nameMap,
        RegionImage regionImage,
        std::shared_ptr<DensityImageContainer> densityImage,
        std::shared_ptr<DoseImageContainer> doseImage,
        std::shared_ptr<TallyImageContainer> tallyImage,
        std::shared_ptr<VarianceImageContainer> varianceImage) const;
    template <typename RegionImage>
    std::vector<DoseReportElement> createData(const std::vector<Material>& nameMap,
        RegionImage regionImage,
        std::shared_ptr<DensityImageContainer> densityImage,
        std::shared_ptr<DoseImageContainer> doseImage,
        std::shared_ptr<TallyImageContainer> tallyImage,
        std::shared_ptr<VarianceImageContainer> varianceImage) const;
    void setDoseUnits(const std::string& units) { m_doseUnits = units; }

private:
    std::shared_ptr<std::vector<DoseReportElement>> m_materialValues;
    std::shared_ptr<std::vector<DoseReportElement>> m_organValues;
    std::string m_doseUnits = "mGy";
};
