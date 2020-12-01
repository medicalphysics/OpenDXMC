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

class CalculateCTNumberFromMaterials {
public:
    CalculateCTNumberFromMaterials(std::vector<Material>& materialMap, const Tube& tube)
    {
        materialCTNumbers(materialMap, tube);
    }

    template <class iterT, class iterU>
    void generateMaterialMap(iterT CTArrayFirst, iterT CTArrayLast, iterU destination)
    {
        if (m_materialCTNumbers.size() <= 1) {
            std::fill(destination, destination + std::distance(CTArrayFirst, CTArrayLast), 0);
            return;
        }

        typedef typename std::iterator_traits<iterT>::value_type T;
        typedef typename std::iterator_traits<iterU>::value_type U;

        const std::size_t nThres = m_materialCTNumbers.size();
        std::vector<T> CTThres(nThres);
        for (std::size_t i = 0; i < nThres - 1; ++i) {
            CTThres[i] = (m_materialCTNumbers[i].second + m_materialCTNumbers[i + 1].second) / 2;
        }

        CTThres[nThres - 1] = std::numeric_limits<T>::infinity();

        std::transform(std::execution::par_unseq, CTArrayFirst, CTArrayLast, destination,
            [&](const T ctNumber) -> U {
                for (std::size_t i = 0; i < nThres; ++i) {
                    if (CTThres[i] >= ctNumber)
                        return static_cast<U>(i);
                }
                return static_cast<U>(0);
            });
    }
    template <class iterT, class iterU, class iterD>
    void generateDensityMap(iterT CTArrayFirst, iterT CTArrayLast, iterU materialIndex, iterD destination)
    {
        //calculate density based on voxel_i CT number and estimated CT number from material M in voxel_i

        typedef typename std::iterator_traits<iterD>::value_type D;

        if (m_materialCTNumbers.size() > 0) {
            std::vector<double> ctNumbers(m_materialCTNumbers.size());
            for (auto [index, hu] : m_materialCTNumbers)
                ctNumbers[index] = hu;
            const auto constant = (m_calibrationEnergy[0] * m_calibrationDensity[0] - m_calibrationEnergy[1] * m_calibrationDensity[1]) / 1000.0;

            std::transform(std::execution::par_unseq, CTArrayFirst, CTArrayLast, materialIndex, destination,
                [constant, &ctNumbers, this](auto val, auto index) -> D {
                    const D dens = (val - ctNumbers[index]) * constant / m_materialEnergy[index] + m_materialDensity[index];
                    return dens > 0.0 ? dens : D { 0 };
                });
        } else {
            auto destination_stop = destination;
            std::advance(destination_stop, std::distance(CTArrayFirst, CTArrayLast));
            std::fill(destination, destination_stop, D { 0 });
        }
    }

private:
    void materialCTNumbers(std::vector<Material>& materialMap, const Tube& tube)
    {

        std::vector<Material> calibrationMaterials;
        calibrationMaterials.push_back(Material("Water, Liquid"));
        calibrationMaterials.push_back(Material("Air, Dry (near sea level)"));
        AttenuationLut calibrationLut;
        calibrationLut.generate(calibrationMaterials, 1.0, tube.voltage());

        m_calibrationEnergy.clear();
        auto specterEnergy = std::vector<floating>(calibrationLut.energyBegin(), calibrationLut.energyEnd());
        auto specterIntensity = tube.getSpecter(specterEnergy);
        m_calibrationDensity.clear();
        for (std::size_t i = 0; i < calibrationMaterials.size(); ++i) {
            m_calibrationDensity.push_back(calibrationMaterials[i].standardDensity());
            m_calibrationEnergy.push_back(std::transform_reduce(std::execution::par, calibrationLut.attenuationTotalBegin(i), calibrationLut.attenuationTotalEnd(i), specterIntensity.cbegin(), 0.0));
        }

        AttenuationLut attLut;
        attLut.generate(materialMap, 1.0, tube.voltage());

        //calculating CT number for each matrial based on the materials detector response
        m_materialCTNumbers.clear();
        m_materialCTNumbers.reserve(materialMap.size());
        m_materialEnergy.clear();
        m_materialEnergy.reserve(materialMap.size());
        m_materialDensity.clear();
        m_materialDensity.reserve(materialMap.size());
        for (std::size_t index = 0; index < materialMap.size(); ++index) {
            m_materialEnergy.push_back(std::transform_reduce(std::execution::par, attLut.attenuationTotalBegin(index), attLut.attenuationTotalEnd(index), specterIntensity.cbegin(), 0.0));
            m_materialDensity.push_back((materialMap[index]).standardDensity());
            floating ctNumber = (m_materialEnergy[index] * m_materialDensity[index] - m_calibrationEnergy[0] * m_calibrationDensity[0]) / (m_calibrationEnergy[0] * m_calibrationDensity[0] - m_calibrationEnergy[1] * m_calibrationDensity[1]) * 1000.0; //Houndsfield units
            m_materialCTNumbers.push_back(std::make_pair(index, ctNumber));
        }
        std::sort(m_materialCTNumbers.begin(), m_materialCTNumbers.end(), [](const std::pair<std::size_t, floating>& a, const std::pair<std::size_t, floating>& b) { return a.second < b.second; });
    }

    std::vector<std::pair<std::size_t, floating>> m_materialCTNumbers;
    std::vector<floating> m_calibrationEnergy;
    std::vector<floating> m_calibrationDensity;
    std::vector<floating> m_materialEnergy;
    std::vector<floating> m_materialDensity;
};

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
    void setCTImportAqusitionAlFiltration(double mm) {m_tube.setAlFiltration(static_cast<floating>(mm)); }
    void setCTImportAqusitionCuFiltration(double mm) {m_tube.setCuFiltration(static_cast<floating>(mm)); }

signals:
    void processingDataStarted();
    void processingDataEnded();
    void imageDataChanged(std::shared_ptr<ImageContainer> imageData);
    void materialDataChanged(const std::vector<Material>& materials);
    void organDataChanged(const std::vector<std::string>& organs);
    void aecFilterChanged(std::shared_ptr<AECFilter> filter);

protected:
    template <class Iter>
    std::pair<std::shared_ptr<std::vector<unsigned char>>, std::shared_ptr<std::vector<floating>>> calculateMaterialAndDensityFromCTData(Iter first, Iter last);
    void processCTData(std::shared_ptr<ImageContainer> ctImage, const std::pair<std::string, std::vector<floating>>& exposureData);
    std::pair<std::string, std::vector<floating>> readExposureData(vtkSmartPointer<vtkDICOMReader>& dicomReader);

private:
    std::array<double, 3> m_outputSpacing = { 2, 2, 2 };
    std::array<double, 3> m_blurRadius = { 1, 1, 1 };
    bool m_useOutputSpacing = false;
    Tube m_tube;
    std::vector<Material> m_ctImportMaterialMap;
};