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
#include <vtkDICOMReader.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageResize.h>
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

void CTImageImportPipeline::readImages(const QStringList& dicomPaths)
{
    emit dataProcessingStarted();

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
    dicomReader->SetReleaseDataFlag(1);

    // apply scaling to Hounfield units
    vtkSmartPointer<vtkDICOMApplyRescale> dicomRescaler = vtkSmartPointer<vtkDICOMApplyRescale>::New();
    dicomRescaler->SetInputConnection(dicomReader->GetOutputPort());
    dicomRescaler->SetOutputScalarType(VTK_DOUBLE);
    dicomRescaler->SetReleaseDataFlag(1);

    // if images aquired with gantry tilt we correct it
    vtkSmartPointer<vtkDICOMCTRectifier> dicomRectifier = vtkSmartPointer<vtkDICOMCTRectifier>::New();
    dicomRectifier->SetInputConnection(dicomRescaler->GetOutputPort());
    dicomRectifier->SetReleaseDataFlag(1);

    // image smoothing filter for volume rendering and segmentation
    vtkSmartPointer<vtkImageGaussianSmooth> smoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    smoother->SetDimensionality(3);
    smoother->SetStandardDeviations(m_blurRadius[0], m_blurRadius[1], m_blurRadius[2]);
    smoother->SetRadiusFactors(m_blurRadius[0] * 2, m_blurRadius[1] * 2, m_blurRadius[2] * 2);
    smoother->SetReleaseDataFlag(1);
    smoother->SetInputConnection(dicomRectifier->GetOutputPort());

    // rescale if we want to
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

    // selecting image data i.e are we rescaling or not
    vtkSmartPointer<vtkImageData> data;
    if (!m_useOutputSpacing) {
        smoother->Update();
        data = smoother->GetOutput();
    } else {
        rescaler->Update();
        data = rescaler->GetOutput();
    }

    auto image = std::make_shared<DataContainer>();

    std::array<int, 3> dims_int;
    data->GetDimensions(dims_int.data());
    std::array<std::size_t, 3> dims;
    for (std::size_t i = 0; i < 3; ++i)
        dims[i] = static_cast<std::size_t>(dims_int[i]);
    image->setDimensions(dims);

    std::array<double, 3> spacing;
    data->GetSpacing(spacing.data());
    image->setSpacing(spacing);

    image->setImageArray(DataContainer::ImageType::CT, data);

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
