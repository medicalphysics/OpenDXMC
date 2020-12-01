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

#include "opendxmc/imageimportpipeline.h"
#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/stringmanipulation.h"

#include <QString>

#include <vtkDICOMApplyRescale.h>
#include <vtkDICOMCTRectifier.h>
#include <vtkDICOMMetaData.h>
#include <vtkDICOMReader.h>
#include <vtkDICOMValue.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageResize.h>
#include <vtkIntArray.h>
#include <vtkMatrix4x4.h>
#include <vtkStringArray.h>

#include <algorithm>
#include <array>
#include <execution>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

ImageImportPipeline::ImageImportPipeline(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
    qRegisterMetaType<std::vector<Material>>();
    qRegisterMetaType<std::vector<std::string>>();
    qRegisterMetaType<std::shared_ptr<AECFilter>>();

    m_outputSpacing[0] = 1.0;
    m_outputSpacing[1] = 1.0;
    m_outputSpacing[2] = 1.0;
    m_blurRadius[0] = 0.0;
    m_blurRadius[1] = 0.0;
    m_blurRadius[2] = 0.0;
    m_tube.setVoltage(120.0);
    m_tube.setAlFiltration(7.0);
}

void ImageImportPipeline::setDicomData(QStringList dicomPaths)
{
    emit processingDataStarted();

    auto constexpr vtkType = VTK_FLOAT;

    vtkSmartPointer<vtkStringArray> fileNameArray = vtkSmartPointer<vtkStringArray>::New();
    fileNameArray->SetNumberOfValues(dicomPaths.size());
    for (int i = 0; i < dicomPaths.size(); ++i) {
        auto path = dicomPaths[i].toStdString();
        fileNameArray->SetValue(i, path);
    }

    //Dicom file reader
    vtkSmartPointer<vtkDICOMReader> dicomReader = vtkSmartPointer<vtkDICOMReader>::New();
    dicomReader->SetMemoryRowOrderToFileNative();
    dicomReader->AutoRescaleOff();
    dicomReader->SetReleaseDataFlag(1);

    //apply scaling to Hounfield units
    vtkSmartPointer<vtkDICOMApplyRescale> dicomRescaler = vtkSmartPointer<vtkDICOMApplyRescale>::New();
    dicomRescaler->SetInputConnection(dicomReader->GetOutputPort());
    dicomRescaler->SetOutputScalarType(vtkType);
    dicomRescaler->SetReleaseDataFlag(1);

    // if images aquired with gantry tilt we correct it
    vtkSmartPointer<vtkDICOMCTRectifier> dicomRectifier = vtkSmartPointer<vtkDICOMCTRectifier>::New();
    dicomRectifier->SetInputConnection(dicomRescaler->GetOutputPort());
    dicomRectifier->SetReleaseDataFlag(1);

    //image smoothing filter for volume rendering and segmentation
    vtkSmartPointer<vtkImageGaussianSmooth> smoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    smoother->SetDimensionality(3);
    smoother->SetStandardDeviations(m_blurRadius[0], m_blurRadius[1], m_blurRadius[2]);
    smoother->SetRadiusFactors(m_blurRadius[0] * 2, m_blurRadius[1] * 2, m_blurRadius[2] * 2);
    smoother->SetReleaseDataFlag(1);
    smoother->SetInputConnection(dicomRectifier->GetOutputPort());

    //rescale if we want to
    vtkSmartPointer<vtkImageResize> rescaler = vtkSmartPointer<vtkImageResize>::New();
    rescaler->SetInputConnection(smoother->GetOutputPort());
    rescaler->SetResizeMethodToOutputSpacing();
    rescaler->SetOutputSpacing(m_outputSpacing.data());
    rescaler->SetReleaseDataFlag(1);

    dicomReader->SetFileNames(fileNameArray);
    dicomReader->SortingOn();
    dicomReader->Update();

    auto orientationMatrix = dicomReader->GetPatientMatrix();
    dicomRectifier->SetVolumeMatrix(orientationMatrix);

    dicomRectifier->Update();
    auto rectifiedMatrix = dicomRectifier->GetVolumeMatrix();
    std::array<double, 6> directionCosines;
    for (std::size_t i = 0; i < 3; i++) {
        directionCosines[i] = rectifiedMatrix->GetElement(static_cast<int>(i), 0);
        directionCosines[i + 3] = rectifiedMatrix->GetElement(static_cast<int>(i), 1);
    }

    // We must construct a supported type
    static_assert(vtkType == 10, "VTK image type is not required float");

    //selecting image data i.e are we rescaling or not
    vtkSmartPointer<vtkImageData> data;
    if (!m_useOutputSpacing) {
        smoother->Update();
        data = smoother->GetOutput();
    } else {
        rescaler->Update();
        data = rescaler->GetOutput();
    }
    data->GetScalarRange(); // to compute scalar range in this thread contra gui thread

    //setting origin to image senter
    auto spacing = data->GetSpacing();
    auto dimensions = data->GetDimensions();
    double origin[3] = { 0, 0, 0 };
    for (int i = 0; i < 3; ++i)
        origin[i] = -0.5 * spacing[i] * dimensions[i];
    data->SetOrigin(origin);

    auto imageContainer = std::make_shared<ImageContainer>(ImageContainer::ImageType::CTImage, data, "HU");
    imageContainer->directionCosines = directionCosines;
    imageContainer->ID = ImageContainer::generateID();

    emit imageDataChanged(imageContainer);
    auto exposure = readExposureData(dicomReader);
    this->processCTData(imageContainer, exposure);
    emit processingDataEnded();
}

void ImageImportPipeline::setOutputSpacing(const double* spacing)
{
    for (std::size_t i = 0; i < 3; ++i)
        m_outputSpacing[i] = spacing[i];
}

void ImageImportPipeline::setBlurRadius(const double* blur)
{
    for (std::size_t i = 0; i < 3; ++i)
        m_blurRadius[i] = blur[i];
}

template <class Iter>
std::pair<std::shared_ptr<std::vector<unsigned char>>, std::shared_ptr<std::vector<floating>>> ImageImportPipeline::calculateMaterialAndDensityFromCTData(Iter first, Iter last)
{
    CalculateCTNumberFromMaterials worker(m_ctImportMaterialMap, m_tube);
    auto materialIndex = std::make_shared<std::vector<unsigned char>>(std::distance(first, last)); // we must make new vector to not invalidate old vector

    //worker.generateMaterialMap(first, last, materialIndex->begin(), nThreads);
    worker.generateMaterialMap(first, last, materialIndex->begin());
    auto density = std::make_shared<std::vector<floating>>(std::distance(first, last)); // we must make new vector to not invalidate old vector
    worker.generateDensityMap(first, last, materialIndex->begin(), density->begin());
    return std::make_pair(materialIndex, density);
}

void ImageImportPipeline::processCTData(std::shared_ptr<ImageContainer> ctImage, const std::pair<std::string, std::vector<floating>>& exposureData)
{
    if (ctImage->imageType != ImageContainer::ImageType::CTImage) {
        return;
    }
    if (!ctImage->image) {
        return; // if ctimage is empty return;
    }
    std::shared_ptr<std::vector<unsigned char>> materialIndex;
    std::shared_ptr<std::vector<floating>> density;

    std::array<std::size_t, 3> dimensions;
    for (std::size_t i = 0; i < 3; ++i)
        dimensions[i] = (ctImage->image->GetDimensions())[i];

    if (ctImage->image->GetScalarType() == VTK_DOUBLE) {
        auto begin = static_cast<double*>(ctImage->image->GetScalarPointer());
        auto end = begin + dimensions[0] * dimensions[1] * dimensions[2];
        auto p = calculateMaterialAndDensityFromCTData(begin, end);
        materialIndex = p.first;
        density = p.second;
    } else if (ctImage->image->GetScalarType() == VTK_FLOAT) {
        auto begin = static_cast<float*>(ctImage->image->GetScalarPointer());
        auto end = begin + dimensions[0] * dimensions[1] * dimensions[2];
        auto p = calculateMaterialAndDensityFromCTData(begin, end);
        materialIndex = p.first;
        density = p.second;
    }

    std::array<double, 3> origo;
    std::array<double, 3> spacing_d;
    std::array<std::size_t, 3> dimensionsArray;
    ctImage->image->GetOrigin(origo.data());
    ctImage->image->GetSpacing(spacing_d.data());
    auto spacing = convert_array_to<floating>(spacing_d);
    auto dim = ctImage->image->GetDimensions();
    for (std::size_t i = 0; i < 3; ++i)
        dimensionsArray[i] = static_cast<std::size_t>(dim[i]);

    std::shared_ptr<ImageContainer> materialImage = std::make_shared<MaterialImageContainer>(materialIndex, dimensionsArray, spacing_d, origo);
    std::shared_ptr<ImageContainer> densityImage = std::make_shared<DensityImageContainer>(density, dimensionsArray, spacing_d, origo);
    materialImage->directionCosines = ctImage->directionCosines;
    densityImage->directionCosines = ctImage->directionCosines;
    materialImage->ID = ctImage->ID;
    densityImage->ID = ctImage->ID;
    densityImage->dataUnits = "g/cm3";

    emit imageDataChanged(materialImage);
    emit imageDataChanged(densityImage);
    emit materialDataChanged(m_ctImportMaterialMap);

    //making exposure map for CT AEC
    const auto& exposurename = exposureData.first;
    const auto& exposure = exposureData.second;

    //interpolating exposure to reduced image data
    std::vector<floating> exposure_interp(dimensionsArray[2], 1.0);
    if (dimensionsArray[2] > 1) {
        const std::size_t exposureSize = exposure.size();
        for (std::size_t i = 0; i < dimensionsArray[2]; ++i) {
            const std::size_t ind = static_cast<std::size_t>(static_cast<double>(i) / (dimensionsArray[2] - 1) * (exposureSize - 1));
            exposure_interp[i] = exposure[ind];
        }
    }
    if (exposure.size() > 0) {
        auto aecFilter = std::make_shared<AECFilter>(density, spacing, dimensionsArray, exposure_interp);
        aecFilter->setFilterName(exposurename);
        emit aecFilterChanged(aecFilter);
    }
}

std::pair<std::string, std::vector<floating>> ImageImportPipeline::readExposureData(vtkSmartPointer<vtkDICOMReader>& dicomReader)
{
    vtkDICOMMetaData* meta = dicomReader->GetMetaData();
    const int n = meta->GetNumberOfInstances();
    std::vector<std::pair<floating, floating>> posExposure(n);

    vtkDICOMTag directionCosinesTag(32, 555);
    auto directionCosinesValue = meta->GetAttributeValue(directionCosinesTag);
    std::array<double, 6> directionCosines;
    for (std::size_t p = 0; p < 6; ++p)
        directionCosines[p] = directionCosinesValue.GetDouble(p);
    std::array<double, 3> imagedirection;
    dxmc::vectormath::cross(directionCosines.data(), imagedirection.data());
    const std::size_t dirIdx = dxmc::vectormath::argmax3<std::size_t, double>(imagedirection.data());

    if (!meta->Has(DC::Exposure)) {
        return std::make_pair(std::string(), std::vector<floating>(n, 1.0));
    }

    // Get the arrays that map slice to file and frame.
    vtkIntArray* fileMap = dicomReader->GetFileIndexArray();

    for (int i = 0; i < n; ++i) {
        int fileIndex = fileMap->GetComponent(i, 0);
        //logger->debug("Reading exposure from file number {}", i);

        const auto pv = meta->Get(fileIndex, DC::Exposure);
        floating exposure = 1.0;
        if (pv.IsValid())
            exposure = pv.GetDouble(0);

        // Get the position for that slice.
        floating position = 0.0;
        const auto pos = meta->Get(fileIndex, DC::ImagePositionPatient);
        if (pos.IsValid()) {
            position = pos.GetDouble(dirIdx);
        }
        posExposure[i] = std::make_pair(position, exposure);
    }

    //sorting on position
    std::sort(posExposure.begin(), posExposure.end());

    std::vector<floating> exposure(n);
    std::transform(posExposure.cbegin(), posExposure.cend(), exposure.begin(), [](auto el) -> auto { return el.second; });

    vtkDICOMTag seriesDescriptionTag(8, 4158);
    auto seriesDescriptionValue = meta->GetAttributeValue(seriesDescriptionTag);
    std::string desc = seriesDescriptionValue.GetString(0);

    return std::make_pair(desc, exposure);
}
