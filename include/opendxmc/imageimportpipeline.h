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
#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/imagecontainer.h"

#include <QObject>
#include <QString>
#include <QStringList>
#include <vtkDICOMReader.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkSmartPointer.h>
#include <vtkType.h>

#include <execution>
#include <future>
#include <limits>
#include <memory>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#ifndef Q_DECLARE_METATYPE_IMAGECONTAINER
#define Q_DECLARE_METATYPE_IMAGECONTAINER
Q_DECLARE_METATYPE(std::shared_ptr<ImageContainer>)
#endif
#ifndef Q_DECLARE_METATYPE_MATERIALVECTOR
#define Q_DECLARE_METATYPE_MATERIALVECTOR
Q_DECLARE_METATYPE(std::vector<Material>)
#endif
#ifndef Q_DECLARE_METATYPE_STRINGVECTOR
#define Q_DECLARE_METATYPE_STRINGVECTOR
Q_DECLARE_METATYPE(std::vector<std::string>)
#endif
#ifndef Q_DECLARE_METATYPE_AECFILTER
#define Q_DECLARE_METATYPE_AECFILTER
Q_DECLARE_METATYPE(std::shared_ptr<AECFilter>)
#endif

template <typename CTIter, typename MatIter, typename DensIter>
void calculateMaterialAndDensityFromCTData(const Tube& tube, const std::vector<Material>& materials, CTIter ctBegin, CTIter ctEnd, MatIter matBegin, DensIter densBegin)
{
    Material air("Air, Dry (near sea level)");
    Material water("Water, Liquid");
    auto specter = tube.getSpecter(true);
    using floatingType = typename std::iterator_traits<DensIter>::value_type;
    static_assert(std::is_same<floatingType, typename std::iterator_traits<CTIter>::value_type>::value, "CT data and density data must be of same type");

    const floatingType airAttenuation = static_cast<floatingType>(air.standardDensity()) * std::transform_reduce(std::execution::par_unseq, specter.cbegin(), specter.cend(), floatingType { 0 }, std::plus<>(), [&](const auto s) -> floatingType { return s.second * static_cast<floatingType>(air.getTotalAttenuation(s.first)); });
    const floatingType waterAttenuation = static_cast<floatingType>(water.standardDensity()) * std::transform_reduce(std::execution::par_unseq, specter.cbegin(), specter.cend(), floatingType { 0 }, std::plus<>(), [&](const auto s) -> floatingType { return s.second * static_cast<floatingType>(water.getTotalAttenuation(s.first)); });

    std::vector<floatingType> materialAttenuation(materials.size());
    std::vector<floatingType> materialHUvalue(materials.size());
    for (std::size_t i = 0; i < materials.size(); ++i) {
        const auto dens = static_cast<floatingType>(materials[i].standardDensity());
        const auto attenuation = std::transform_reduce(std::execution::par_unseq, specter.cbegin(), specter.cend(), floatingType { 0 }, std::plus<>(), [&](const auto s) -> floatingType { return s.second * static_cast<floatingType>(materials[i].getTotalAttenuation(s.first)); });
        materialAttenuation[i] = attenuation;
        materialHUvalue[i] = (materialAttenuation[i] * dens - waterAttenuation) / (waterAttenuation - airAttenuation) * 1000;
    }

    // assigning materials
    using matType = typename std::iterator_traits<MatIter>::value_type;
    std::vector<std::pair<matType, floatingType>> CTNumberThreshold(materialHUvalue.size());
    for (matType i = 0; i < CTNumberThreshold.size() - 1; ++i) {
        const auto thres = (materialHUvalue[i] + materialHUvalue[i + 1]) / 2;
        CTNumberThreshold[i] = std::make_pair(i, thres);
    }
    CTNumberThreshold[CTNumberThreshold.size() - 1] = std::make_pair(static_cast<matType>(CTNumberThreshold.size() - 1), std ::numeric_limits<floatingType>::infinity());
    std::sort(CTNumberThreshold.begin(), CTNumberThreshold.end(), [](const auto lh, const auto rh) { return lh.second < rh.second; });
    std::transform(std::execution::par_unseq, ctBegin, ctEnd, matBegin, [&](const auto HU) -> matType {
        for (const auto& [idx, HUthres] : CTNumberThreshold) {
            if (HU <= HUthres)
                return idx;
        }
        return CTNumberThreshold.back().first; // this will never be hit, but we include it to make the compiler happy
    });

    // calculating density
    std::transform(std::execution::par_unseq, ctBegin, ctEnd, matBegin, densBegin, [&](const auto HU, const auto mIdx) -> floatingType {
        const auto att = HU * (waterAttenuation - airAttenuation) / 1000 + waterAttenuation;
        return materialAttenuation[mIdx] > 0 ? att / materialAttenuation[mIdx] : 0;
    });
}

class ImageImportPipeline : public QObject {
    Q_OBJECT
public:
    ImageImportPipeline(QObject* parent = nullptr);
    void setDicomData(QStringList dicomPaths);
    void setOutputSpacing(const double* spacing);
    void setUseOutputSpacing(bool value) { m_useOutputSpacing = value; };
    void setBlurRadius(const double* radius);

    void setCTImportMaterialMap(const std::vector<Material>& map) { m_ctImportMaterialMap = map; }
    void setCTImportAqusitionVoltage(double voltage) { m_tube.setVoltage(static_cast<floating>(voltage)); }
    void setCTImportAqusitionAlFiltration(double mm) { m_tube.setAlFiltration(static_cast<floating>(mm)); }
    void setCTImportAqusitionCuFiltration(double mm) { m_tube.setCuFiltration(static_cast<floating>(mm)); }

signals:
    void processingDataStarted();
    void processingDataEnded();
    void imageDataChanged(std::shared_ptr<ImageContainer> imageData);
    void materialDataChanged(const std::vector<Material>& materials);
    void organDataChanged(const std::vector<std::string>& organs);
    void aecFilterChanged(std::shared_ptr<AECFilter> filter);

protected:
    //template <class Iter>
    //std::pair<std::shared_ptr<std::vector<unsigned char>>, std::shared_ptr<std::vector<floating>>> calculateMaterialAndDensityFromCTData(Iter first, Iter last);
    void processCTData(std::shared_ptr<ImageContainer> ctImage, const std::pair<std::string, std::vector<floating>>& exposureData);
    std::pair<std::string, std::vector<floating>> readExposureData(vtkSmartPointer<vtkDICOMReader>& dicomReader);

private:
    std::array<double, 3> m_outputSpacing = { 2, 2, 2 };
    std::array<double, 3> m_blurRadius = { 1, 1, 1 };
    bool m_useOutputSpacing = false;
    Tube m_tube;
    std::vector<Material> m_ctImportMaterialMap;
};