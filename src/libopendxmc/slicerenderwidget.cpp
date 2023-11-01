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

#include <slicerenderwidget.hpp>

#include <QTimer>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSincInterpolator.h>
#include <vtkOpenGLImageSliceMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>

// #include <vtkRenderer.h>
// #include <vtkResliceCursor.h>
// #include <vtkResliceCursorActor.h>
// #include <vtkResliceCursorLineRepresentation.h>
// #include <vtkResliceCursorPolyDataAlgorithm.h>
// #include <vtkResliceCursorWidget.h>
//  #include <vtkResliceImageViewer.h>
// #include <vtkScalarBarActor.h>
// #include <vtkTextProperty.h>
//  #include <vtkCellPicker.h>
//  #include <vtkCornerAnnotation.h>
//  #include <vtkImageActor.h>
//  #include <vtkPlane.h>
//  #include <vtkPlaneSource.h>
//  #include <vtkLookupTable.h>
//  #include <vtkMatrix4x4.h>
//  #include <vtkImageMapToColors.h>
//  #include <vtkImageMapper3D.h>
//  #include <vtkImagePlaneWidget.h>

class WindowLevelModifiedCallback : public vtkCallbackCommand {
public:
    static WindowLevelModifiedCallback* New()
    {
        return new WindowLevelModifiedCallback;
    }
    // Here we Create a vtkCallbackCommand and reimplement it.
    void Execute(vtkObject* caller, unsigned long evId, void*) override
    {
        if (evId == vtkCommand::EndWindowLevelEvent) {
            // Note the use of reinterpret_cast to cast the caller to the expected type.
            auto style = reinterpret_cast<vtkInteractorStyleImage*>(caller);

            auto property = style->GetCurrentImageProperty();
            if (property) {
                for (auto& slice : imageSlices) {
                    auto p = slice->GetProperty();
                    p->SetColorWindow(property->GetColorWindow());
                    p->SetColorLevel(property->GetColorLevel());
                }
                for (auto& wid : widgets)
                    wid->renderWindow()->Render();
            }
        } else if (evId == vtkCommand::PickEvent) {
            auto style = reinterpret_cast<vtkInteractorStyleImage*>(caller);
            auto currentRenderer = style->GetCurrentRenderer();
            auto currentCamera = currentRenderer->GetActiveCamera();
            const auto currentFocalPoint = currentCamera->GetFocalPoint();

            for (auto& wid : widgets) {
                auto renderer = wid->renderWindow()->GetInteractor()->FindPokedRenderer(0, 0);
                if (renderer) {
                    auto camera = renderer->GetActiveCamera();
                    // camera->SetFocalPoint(currentFocalPoint);
                    // TODO set proper focal point to reslizing
                    wid->renderWindow()->Render();
                }
            }
        }
    }

    // Convinietn function to get commands this callback is intended for
    constexpr static std::vector<vtkCommand::EventIds> eventTypes()
    {
        std::vector<vtkCommand::EventIds> cmds;
        cmds.push_back(vtkCommand::EndWindowLevelEvent);
        cmds.push_back(vtkCommand::PickEvent);
        return cmds;
    }

    WindowLevelModifiedCallback()
    {
    }

    // Set pointers to any clientData or callData here.
    std::vector<vtkSmartPointer<vtkImageSlice>> imageSlices;
    std::vector<QVTKOpenGLNativeWidget*> widgets;

private:
    WindowLevelModifiedCallback(const WindowLevelModifiedCallback&) = delete;
    void operator=(const WindowLevelModifiedCallback&) = delete;
};

std::shared_ptr<DataContainer> generateSampleData()
{
    auto data = std::make_shared<DataContainer>();
    data->setDimensions({ 8, 8, 8 });
    data->setSpacing({ 1, 1, 1 });
    std::vector<double> im(8 * 8 * 8, 0);
    for (int i = 0; i < im.size(); ++i)
        im[i] = i;
    data->setImageArray(DataContainer::ImageType::CT, im);
    return data;
}

SliceRenderWidget::SliceRenderWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    for (std::size_t i = 0; i < 3; ++i)
        openGLWidget[i] = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(openGLWidget[0], 0, 0);
    layout->addWidget(openGLWidget[1], 1, 0);
    layout->addWidget(openGLWidget[2], 1, 1);

    this->setLayout(layout);

    setupSlicePipeline();

    // setting dummy data to avoid pipeline errors
    updateImageData(generateSampleData());
}

void SliceRenderWidget::setupSlicePipeline()
{
    // https://github.com/sankhesh/FourPaneViewer/blob/master/QtVTKRenderWindows.cxx

    // gaussian smooth mapper
    imageSmoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    imageSmoother->SetDimensionality(3);
    imageSmoother->SetStandardDeviations(0.0, 0.0, 0.0);
    imageSmoother->ReleaseDataFlagOff();

    // lut
    lut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();

    for (int i = 0; i < 3; ++i) {

        // renderers
        renderer[i] = vtkSmartPointer<vtkRenderer>::New();
        renderer[i]->GetActiveCamera()->ParallelProjectionOn();
        renderer[i]->SetBackground(0, 0, 0);

        // interaction style
        interactorStyle[i] = vtkSmartPointer<CustomInteractorStyleImage>::New();
        interactorStyle[i]->SetDefaultRenderer(renderer[i]);
        interactorStyle[i]->SetInteractionModeToImageSlicing();
        // interactionStyle->SetInteractionModeToImage3D();

        auto renderWindowInteractor = openGLWidget[i]->interactor();
        renderWindowInteractor->SetInteractorStyle(interactorStyle[i]);
        auto renWin = openGLWidget[i]->renderWindow();
        renWin->AddRenderer(renderer[i]);

        // auto textActorCorners = vtkSmartPointer<vtkCornerAnnotation>::New();
        // textActorCorners->SetText(1, "");
        // textActorCorners->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
        //  interactionStyle->setCornerAnnotation(textActorCorners);

        // colorbar
        // auto scalarColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
        // scalarColorBar->SetMaximumWidthInPixels(200);
        // scalarColorBar->AnnotationTextScalingOff();

        // reslice mapper
        auto imageMapper = vtkSmartPointer<vtkOpenGLImageSliceMapper>::New();
        imageMapper->SetInputConnection(imageSmoother->GetOutputPort());
        imageMapper->SliceFacesCameraOn();
        imageMapper->SetSliceAtFocalPoint(true);
        /*
        auto imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
        imageMapper->SetInputConnection(imageSmoother->GetOutputPort());
        imageMapper->SliceFacesCameraOn();
        imageMapper->SetSliceAtFocalPoint(true);
        imageMapper->StreamingOn();
        imageMapper->SetJumpToNearestSlice(false);
        imageMapper->ResampleToScreenPixelsOff(); // to use custom interpolator
        auto interpolator = vtkSmartPointer<vtkImageSincInterpolator>::New();
        imageMapper->SetInterpolator(interpolator);
        */

        // image slice
        imageSlice[i] = vtkSmartPointer<vtkImageSlice>::New();
        imageSlice[i]->SetMapper(imageMapper);
        renderer[i]->AddActor(imageSlice[i]);

        if (auto cam = renderer[i]->GetActiveCamera(); i == 0) {
            cam->SetFocalPoint(0, 0, 0);
            cam->SetPosition(0, 0, -1);
            cam->SetViewUp(0, -1, 0);
        } else if (i == 1) {
            cam->SetFocalPoint(0, 0, 0);
            cam->SetPosition(0, -1, 0);
            cam->SetViewUp(0, 0, 1);
        } else {
            cam->SetFocalPoint(0, 0, 0);
            cam->SetPosition(1, 0, 0);
            cam->SetViewUp(0, 0, 1);
        }

        auto sliceProperty = imageSlice[i]->GetProperty();
        sliceProperty->SetLookupTable(lut);
        lut->SetMinimumTableValue(0, 0, 0, 1);
        lut->SetMaximumTableValue(1, 1, 1, 1);
        lut->Build();
        sliceProperty->SetUseLookupTableScalarRange(false);
    }

    // adding callbacks to share windowlevel

    for (std::size_t i = 0; i < 3; ++i) {
        auto callback = vtkSmartPointer<WindowLevelModifiedCallback>::New();
        auto style = interactorStyle[i];
        for (std::size_t j = 0; j < 3; ++j) {
            if (i != j) {
                callback->imageSlices.push_back(imageSlice[j]);
                callback->widgets.push_back(openGLWidget[j]);
            }
        }
        for (auto ev : callback->eventTypes())
            style->AddObserver(ev, callback);
    }
}

void SliceRenderWidget::Render()
{
    for (auto& slic : imageSlice)
        slic->Update();
    for (auto& ren : renderer)
        ren->ResetCamera();
    for (auto& w : openGLWidget)
        w->renderWindow()->Render();
}

void SliceRenderWidget::setInteractionStyleTo3D()
{
    for (auto& style : interactorStyle)
        style->SetInteractionModeToImage3D();
}

void SliceRenderWidget::setInteractionStyleToSlicing()
{
    for (auto& style : interactorStyle)
        style->SetInteractionModeToImageSlicing();
}

void SliceRenderWidget::useFXAA(bool use)
{
    for (auto& ren : renderer)
        ren->SetUseFXAA(use);
}

void SliceRenderWidget::setMultisampleAA(int samples)
{
    int s = std::max(samples, int { 0 });
    for (auto& wid : openGLWidget) {
        auto renWin = wid->renderWindow();
        renWin->SetMultiSamples(s);
    }
}

void SliceRenderWidget::setNewImageData(vtkImageData* data)
{
    if (data) {
        imageSmoother->SetInputData(data);
        Render();
    }
}

void SliceRenderWidget::updateImageData(std::shared_ptr<DataContainer> data)
{
    if (data && m_data) {
        if (data->ID() == m_data->ID()) {
            // no changes
            return;
        }
    }
    bool uid_is_new = true;
    if (m_data && data)
        uid_is_new = data->ID() != m_data->ID();

    // updating images before replacing old buffer
    if (data) {
        if (data->hasImage(DataContainer::ImageType::CT) && uid_is_new) {
            auto vtkimage = data->vtkImage(DataContainer::ImageType::CT);
            setNewImageData(vtkimage);
        }
    }
    m_data = data;
}
