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

#include <datacontainer.hpp>

#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkImageActor.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageSincInterpolator.h>
#include <vtkImageStack.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>
#include <vtkWindowLevelLookupTable.h>

#include <map>
#include <memory>
#include <vector>

class BeamActorContainer;
class vtkCallbackCommand;
class vtkCommand;

class SliceRenderWidget : public QWidget {
    Q_OBJECT
public:
    SliceRenderWidget(int orientation, bool lowerLeftText, bool colorbar, QWidget* parent = nullptr);
    void updateImageData(std::shared_ptr<DataContainer>);
    void useFXAA(bool on);
    void setMultisampleAA(int samples);
    void setInterpolationType(int type = 1);
    void setBackgroundColor(double r, double g, double b);
    void setUseCTDataBackground(bool on);
    void setImageSmoothing(double pixels);
    void registerStyleCallback(vtkSmartPointer<vtkCallbackCommand> cmd, const std::vector<vtkCommand::EventIds>& events);
    void showData(DataContainer::ImageType type);
    void Render(bool reset_camera = false);

    void addActor(vtkSmartPointer<vtkActor> actor);
    void removeActor(vtkSmartPointer<vtkActor> actor);

    vtkSmartPointer<vtkImageActor> imageSlice() { return m_imageSliceFront; }
    vtkSmartPointer<vtkRenderer> renderer() { return m_renderer; }

protected:
    void setNewImageData(vtkSmartPointer<vtkImageData> data, bool rezoom_camera = false);
    void setupSlicePipeline(int orientation, bool lowerLeftText, bool colorbar);
    void switchLUTtable(DataContainer::ImageType type);
    void resizeEvent(QResizeEvent* event) override;
    void updateTextPositions(bool render = false);
    void resetCamera();

private:
    std::shared_ptr<DataContainer> m_data = nullptr;
    vtkSmartPointer<vtkImageStack> m_imageStack = nullptr;
    vtkSmartPointer<vtkImageActor> m_imageSliceFront = nullptr;
    vtkSmartPointer<vtkImageActor> m_imageSliceBack = nullptr;
    vtkSmartPointer<vtkImageSincInterpolator> m_interpolatorSinc = nullptr;
    vtkSmartPointer<vtkImageGaussianSmooth> m_smoother = nullptr;
    vtkSmartPointer<vtkRenderer> m_renderer = nullptr;
    QVTKOpenGLNativeWidget* openGLWidget = nullptr;
    vtkSmartPointer<vtkTextActor> m_unitText = nullptr;
    vtkSmartPointer<vtkTextActor> m_windowText = nullptr;
    vtkSmartPointer<vtkWindowLevelLookupTable> m_lut = nullptr;
    std::map<DataContainer::ImageType, std::pair<double, double>> lut_windowing;
    DataContainer::ImageType lut_current_type = DataContainer::ImageType::CT;
    bool m_useCTBackground = false;
};
