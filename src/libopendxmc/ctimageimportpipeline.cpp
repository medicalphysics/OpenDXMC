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

#include <ctimageimportpipeline.hpp>

#include <vtkDICOMApplyRescale.h>
#include <vtkDICOMCTRectifier.h>
#include <vtkDICOMMetaData.h>
#include <vtkDICOMReader.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageResize.h>
#include <vtkIntArray.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>

CTImageImportPipeline::CTImageImportPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void CTImageImportPipeline::updateImageData(std::shared_ptr<DataContainer> data)
{
    // we should only generate data, never recive it.
}

std::array<double, 3> cross(const std::array<double, 3>& v1, const std::array<double, 3> v2) noexcept
{
    return std::array<double, 3> {
        v1[1] * v2[2] - v1[2] * v2[1],
        v1[2] * v2[0] - v1[0] * v2[2],
        v1[0] * v2[1] - v1[1] * v2[0]
    };
}

// extract CT AEC data
DataContainer::AECData readExposureData(vtkSmartPointer<vtkDICOMReader>& dicomReader)
{
    DataContainer::AECData res;

    if (!dicomReader)
        return res;

    vtkDICOMMetaData* meta = dicomReader->GetMetaData();
    if (!meta)
        return res;
    const auto n = meta->GetNumberOfInstances();
    if (n < 2)
        return res;

    if (!meta->Has(DC::Exposure)) {
        return res;
    }

    auto directionCosinesValue = meta->GetAttributeValue(0, DC::ImageOrientationPatient);
    std::array<double, 3> xCos;
    std::array<double, 3> yCos;
    for (std::size_t p = 0; p < 3; ++p) {
        xCos[p] = directionCosinesValue.GetDouble(p);
        yCos[p] = directionCosinesValue.GetDouble(p + 3);
    }

    const auto imageDir = cross(xCos, yCos);
    std::size_t imageDirIdx = 0;
    if (std::abs(imageDir[imageDirIdx]) < std::abs(imageDir[1]))
        imageDirIdx = 1;
    if (std::abs(imageDir[imageDirIdx]) < std::abs(imageDir[2]))
        imageDirIdx = 2;

    std::vector<std::pair<std::array<double, 3>, double>> data(n);

    // Get the arrays that map slice to file and frame.
    // vtkIntArray* fileMap = dicomReader->GetFileIndexArray();

    for (int i = 0; i < n; ++i) {

        const auto& etag = meta->Get(i, DC::Exposure);
        data[i].second = etag.GetDouble(0);

        const auto& ptag = meta->Get(i, DC::ImagePositionPatient);
        for (std::size_t j = 0; j < 3; ++j)
            data[i].first[j] = ptag.GetDouble(j);
    }
    res.weights.resize(n);
    std::sort(data.begin(), data.end(), [=](const auto& lh, const auto& rh) { return lh.first[imageDirIdx] < rh.first[imageDirIdx]; });
    std::transform(data.cbegin(), data.cend(), res.weights.begin(), [](const auto& v) { return v.second; });

    res.startPosition = data.front().first;
    res.stopPosition = data.back().first;

    // from mm to cm
    for (std::size_t i = 0; i < 3; ++i) {
        res.startPosition[i] /= 10.0;
        res.stopPosition[i] /= 10.0;
        if (i == imageDirIdx) {
            const auto d = (res.stopPosition[i] - res.startPosition[i]) / 2;
            res.startPosition[i] = -d;
            res.stopPosition[i] = d;
        }
    }
    return res;
}

void CTImageImportPipeline::readImages(const QStringList& dicomPaths)
{
    emit dataProcessingStarted();

    auto image = std::make_shared<DataContainer>();

    { // scope to clean up smart pointers before rendering (we need the memory)
        vtkSmartPointer<vtkStringArray> fileNameArray = vtkSmartPointer<vtkStringArray>::New();
        fileNameArray->SetNumberOfValues(dicomPaths.size());
        for (int i = 0; i < dicomPaths.size(); ++i) {
            auto path = dicomPaths[i].toStdString();
            fileNameArray->SetValue(i, path);
        }

        // Dicom file reader
        vtkSmartPointer<vtkDICOMReader> dicomReader = vtkSmartPointer<vtkDICOMReader>::New();
        dicomReader->SetMemoryRowOrderToFileNative();
        dicomReader->AutoRescaleOff();
        dicomReader->ReleaseDataFlagOn();

        // apply scaling to Hounfield units
        vtkSmartPointer<vtkDICOMApplyRescale> dicomRescaler = vtkSmartPointer<vtkDICOMApplyRescale>::New();
        dicomRescaler->SetInputConnection(dicomReader->GetOutputPort());
        dicomRescaler->SetOutputScalarType(VTK_DOUBLE);
        dicomRescaler->ReleaseDataFlagOn();

        // if images aquired with gantry tilt we correct it
        vtkSmartPointer<vtkDICOMCTRectifier> dicomRectifier = vtkSmartPointer<vtkDICOMCTRectifier>::New();
        dicomRectifier->SetInputConnection(dicomRescaler->GetOutputPort());
        dicomRectifier->ReleaseDataFlagOn();

        // image smoothing filter for volume rendering and segmentation
        vtkSmartPointer<vtkImageGaussianSmooth> smoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
        smoother->SetDimensionality(3);
        smoother->SetStandardDeviations(m_blurRadius[0], m_blurRadius[1], m_blurRadius[2]);
        smoother->SetRadiusFactors(m_blurRadius[0] * 2, m_blurRadius[1] * 2, m_blurRadius[2] * 2);
        smoother->ReleaseDataFlagOn();
        smoother->SetInputConnection(dicomRectifier->GetOutputPort());

        // rescale if we want to
        vtkSmartPointer<vtkImageResize> rescaler = vtkSmartPointer<vtkImageResize>::New();
        rescaler->SetInputConnection(smoother->GetOutputPort());
        rescaler->SetResizeMethodToOutputSpacing();
        rescaler->SetOutputSpacing(m_outputSpacing.data());
        rescaler->ReleaseDataFlagOn();

        dicomReader->SetFileNames(fileNameArray);
        dicomReader->SortingOn();
        dicomReader->Update();

        auto orientationMatrix = dicomReader->GetPatientMatrix();
        dicomRectifier->SetVolumeMatrix(orientationMatrix);

        dicomRectifier->Update();
        auto rectifiedMatrix = dicomRectifier->GetVolumeMatrix();

        // selecting image data i.e are we rescaling or not
        vtkSmartPointer<vtkImageData> data;
        if (!m_useOutputSpacing) {
            smoother->Update();
            data = smoother->GetOutput();
        } else {
            rescaler->Update();
            data = rescaler->GetOutput();
        }

        std::array<int, 3> dims_int;
        data->GetDimensions(dims_int.data());
        std::array<std::size_t, 3> dims;
        for (std::size_t i = 0; i < 3; ++i)
            dims[i] = static_cast<std::size_t>(dims_int[i]);
        image->setDimensions(dims);

        std::array<double, 3> spacing;
        data->GetSpacing(spacing.data());
        image->setSpacingInmm(spacing);

        image->setImageArray(DataContainer::ImageType::CT, data);

        image->setAecData(readExposureData(dicomReader));
    }

    emit imageDataChanged(image);
    emit dataProcessingFinished();
}

void CTImageImportPipeline::setBlurRadius(const double* d)
{
    for (std::size_t i = 0; i < 3; ++i)
        m_blurRadius[i] = d[i];
}

void CTImageImportPipeline::setUseOutputSpacing(bool trigger)
{
    m_useOutputSpacing = trigger;
}

void CTImageImportPipeline::setOutputSpacing(const double* d)
{
    for (std::size_t i = 0; i < 3; ++i)
        m_outputSpacing[i] = d[i];
}
