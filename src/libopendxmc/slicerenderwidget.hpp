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
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>
#include <vtkWindowLevelLookupTable.h>

#include <map>
#include <memory>

class BeamActorContainer;

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
    void Render(bool reset_camera = false);
    void sharedViews(SliceRenderWidget* other1, SliceRenderWidget* other2);
    void sharedViews(std::vector<SliceRenderWidget*> others);

    void addActor(vtkSmartPointer<vtkActor> actor);
    void removeActor(vtkSmartPointer<vtkActor> actor);

protected:
    void setNewImageData(vtkSmartPointer<vtkImageData> data, bool rezoom_camera = false);
    void setupSlicePipeline(int orientation);
    void switchLUTtable(DataContainer::ImageType type);
    void resizeEvent(QResizeEvent* event) override;
    void updateTextPositions(bool render = false);
    void resetCamera();

private:
    std::shared_ptr<DataContainer> m_data = nullptr;
    vtkSmartPointer<vtkImageActor> imageSlice = nullptr;
    vtkSmartPointer<vtkRenderer> renderer = nullptr;
    QVTKOpenGLNativeWidget* openGLWidget = nullptr;
    vtkSmartPointer<vtkTextActor> lowerLeftText = nullptr;
    vtkSmartPointer<CustomInteractorStyleImage> interactorStyle = nullptr;
    vtkSmartPointer<vtkWindowLevelLookupTable> lut = nullptr;
    std::map<DataContainer::ImageType, std::pair<double, double>> lut_windowing;
    DataContainer::ImageType lut_current_type = DataContainer::ImageType::CT;
};
