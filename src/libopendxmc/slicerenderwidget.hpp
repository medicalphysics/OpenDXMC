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

#pragma once

#include <custominteractorstyleimage.hpp>
#include <datacontainer.hpp>

#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkImageActor.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkOpenGLRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkWindowLevelLookupTable.h>

class SliceRenderWidget : public QWidget {
    Q_OBJECT
public:
    SliceRenderWidget(int orientation = 0, QWidget* parent = nullptr);
    void updateImageData(std::shared_ptr<DataContainer>);
    void useFXAA(bool on);
    void setMultisampleAA(int samples);
    void setInterpolationType(int type = 1);
    void setInteractionStyleToSlicing();
    void setInteractionStyleTo3D();

    void showData(DataContainer::ImageType type);

    void sharedViews(SliceRenderWidget* other1, SliceRenderWidget* other2);
    void sharedViews(std::vector<SliceRenderWidget*> others);

protected:
    void setNewImageData(vtkSmartPointer<vtkImageData> data, bool rezoom_camera = false);
    void setupSlicePipeline(int orientation);
    void Render(bool rezoom_camera = false);
    void switchLUTtable(bool discrete = false, int n_colors = -1);

private:
    std::shared_ptr<DataContainer> m_data = nullptr;
    vtkSmartPointer<vtkImageActor> imageSlice = nullptr;
    vtkSmartPointer<vtkOpenGLRenderer> renderer = nullptr;
    QVTKOpenGLNativeWidget* openGLWidget = nullptr;
    vtkSmartPointer<CustomInteractorStyleImage> interactorStyle = nullptr;
    vtkSmartPointer<vtkWindowLevelLookupTable> lut = nullptr;
};
