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

#include <datacontainer.hpp>

#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkImageGaussianSmooth.h>
#include <vtkImageSlice.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkWindowLevelLookupTable.h>

class SliceRenderWidget : public QWidget {
    Q_OBJECT
public:
    SliceRenderWidget(QWidget* parent = nullptr);
    void updateImageData(std::shared_ptr<DataContainer>);
    void useFXAA(bool on);
    void setMultisampleAA(int samples);
    void setInteractionStyleToSlicing();
    void setInteractionStyleTo3D();

protected:
    void setNewImageData(vtkImageData* data);
    void setupPipeline();
    void Render();

private:
    std::shared_ptr<DataContainer> m_data = nullptr;
    vtkSmartPointer<vtkImageGaussianSmooth> imageSmoother = nullptr;
    std::array<vtkSmartPointer<vtkImageSlice>, 3> imageSlice = { nullptr, nullptr, nullptr };
    std::array<vtkSmartPointer<vtkRenderer>, 3> renderer = { nullptr, nullptr, nullptr };
    std::array<QVTKOpenGLNativeWidget*, 3> openGLWidget = { nullptr, nullptr, nullptr };
    vtkSmartPointer<vtkWindowLevelLookupTable> lut = nullptr;
};
