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

#include <slicerenderwidget.hpp>

#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCornerAnnotation.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSincInterpolator.h>
#include <vtkOpenGLImageSliceMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkTextProperty.h>

#include <charconv>
#include <string>

class WindowLevelSlicingModifiedCallback : public vtkCallbackCommand {
public:
    static WindowLevelSlicingModifiedCallback* New()
    {
        return new WindowLevelSlicingModifiedCallback;
    }
    // Here we Create a vtkCallbackCommand and reimplement it.
    void Execute(vtkObject* caller, unsigned long evId, void*) override
    {
        if (evId == vtkCommand::EndWindowLevelEvent || evId == vtkCommand::WindowLevelEvent) {
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

            std::array<int, 2> eventPos;
            style->GetInteractor()->GetLastEventPosition(eventPos.data());
            if (this->picker->Pick(eventPos[0], eventPos[1], 0, currentRenderer) > 0) {
                std::array<double, 3> pos;
                picker->GetPickPosition(pos.data());
                for (auto& wid : widgets) {
                    auto renderer = wid->renderWindow()->GetRenderers()->GetFirstRenderer();
                    auto camera = renderer->GetActiveCamera();
                    std::array<double, 3> normal;
                    camera->GetViewPlaneNormal(normal.data());
                    auto posIdx = argmax(normal);
                    std::array<double, 3> focalPoint;
                    camera->GetFocalPoint(focalPoint.data());
                    focalPoint[posIdx] = pos[posIdx];
                    camera->SetFocalPoint(focalPoint.data());
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
        cmds.push_back(vtkCommand::WindowLevelEvent);
        cmds.push_back(vtkCommand::PickEvent);
        return cmds;
    }

    // Set pointers to any clientData or callData here.
    std::vector<vtkSmartPointer<vtkImageSlice>> imageSlices;
    std::vector<QVTKOpenGLNativeWidget*> widgets;

protected:
    int argmax(const std::array<double, 3>& v)
    {
        if (std::abs(v[0]) > std::abs(v[1])) {
            return std::abs(v[0]) > std::abs(v[2]) ? 0 : 2;
        } else {
            return std::abs(v[1]) > std::abs(v[2]) ? 1 : 2;
        }
    }

private:
    WindowLevelSlicingModifiedCallback()
    {
        picker = vtkSmartPointer<vtkCellPicker>::New();
    }

    vtkSmartPointer<vtkCellPicker> picker = nullptr;
    WindowLevelSlicingModifiedCallback(const WindowLevelSlicingModifiedCallback&) = delete;
    void operator=(const WindowLevelSlicingModifiedCallback&) = delete;
};

class TextModifiedCallback : public vtkCallbackCommand {
public:
    static TextModifiedCallback* New()
    {
        return new TextModifiedCallback;
    }
    // Here we Create a vtkCallbackCommand and reimplement it.
    void Execute(vtkObject* caller, unsigned long evId, void*) override
    {
        if (vtkCommand::WindowLevelEvent) {
            // Note the use of reinterpret_cast to cast the caller to the expected type.
            auto style = reinterpret_cast<vtkInteractorStyleImage*>(caller);
            auto property = style->GetCurrentImageProperty();
            if (property) {
                auto ww = property->GetColorWindow();
                auto wl = property->GetColorLevel();

                std::array<char, 10> buffer;
                std::string txt = "WL: ";
                if (auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), wl, std::chars_format::general, 3); ec == std::errc()) {
                    txt += std::string_view(buffer.data(), ptr);
                }
                txt += " WW: ";
                if (auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), ww, std::chars_format::general, 3); ec == std::errc()) {
                    txt += std::string_view(buffer.data(), ptr);
                }
                textActorCorner->SetText(1, txt.c_str());
            }
        }
    }

    // Convinietn function to get commands this callback is intended for
    constexpr static std::vector<vtkCommand::EventIds> eventTypes()
    {
        std::vector<vtkCommand::EventIds> cmds;
        cmds.push_back(vtkCommand::WindowLevelEvent);
        return cmds;
    }

    // Set pointers to any clientData or callData here.
    vtkSmartPointer<vtkCornerAnnotation> textActorCorner = nullptr;

    std::vector<vtkSmartPointer<vtkImageSlice>> imageSlices;
    std::vector<QVTKOpenGLNativeWidget*> widgets;

protected:
private:
    TextModifiedCallback()
    {
        textActorCorner = vtkSmartPointer<vtkCornerAnnotation>::New();
        textActorCorner->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    }

    vtkSmartPointer<vtkCellPicker> picker = nullptr;
    TextModifiedCallback(const TextModifiedCallback&) = delete;
    void operator=(const TextModifiedCallback&) = delete;
};

SliceRenderWidget::SliceRenderWidget(int orientation, QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    for (std::size_t i = 0; i < 4; ++i)
        openGLWidget = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(openGLWidget);

    this->setLayout(layout);

    // lut
    lut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();

    setupSlicePipeline(orientation);
}

void SliceRenderWidget::setupSlicePipeline(int orientation)
{
    // https://github.com/sankhesh/FourPaneViewer/blob/master/QtVTKRenderWindows.cxx

    // for (int i = 0; i < 3; ++i) {

    // renderers
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->GetActiveCamera()->ParallelProjectionOn();
    renderer->SetBackground(0, 0, 0);

    // interaction style
    interactorStyle = vtkSmartPointer<CustomInteractorStyleImage>::New();
    interactorStyle->SetDefaultRenderer(renderer);
    interactorStyle->SetInteractionModeToImageSlicing();
    // interactionStyle->SetInteractionModeToImage3D();

    auto renderWindowInteractor = openGLWidget->interactor();
    renderWindowInteractor->SetInteractorStyle(interactorStyle);
    auto renWin = openGLWidget->renderWindow();
    renWin->AddRenderer(renderer);

    // colorbar
    // auto scalarColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
    // scalarColorBar->SetMaximumWidthInPixels(200);
    // scalarColorBar->AnnotationTextScalingOff();

    // reslice mapper
    auto imageMapper = vtkSmartPointer<vtkOpenGLImageSliceMapper>::New();
    imageMapper->SliceFacesCameraOn();
    imageMapper->SetSliceAtFocalPoint(true);

    /* auto imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
    imageMapper->SliceFacesCameraOn();
    imageMapper->SetSliceAtFocalPoint(true);
    imageMapper->StreamingOn();
    imageMapper->SetJumpToNearestSlice(false);
    imageMapper->ResampleToScreenPixelsOff(); // to use custom interpolator
    auto interpolator = vtkSmartPointer<vtkImageSincInterpolator>::New();
    imageMapper->SetInterpolator(interpolator);
    */

    // image slice
    imageSlice = vtkSmartPointer<vtkImageSlice>::New();
    imageSlice->SetMapper(imageMapper);
    renderer->AddActor(imageSlice);

    if (auto cam = renderer->GetActiveCamera(); orientation == 0) {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, 0, -1);
        cam->SetViewUp(0, -1, 0);
    } else if (orientation == 1) {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, -1, 0);
        cam->SetViewUp(0, 0, 1);
    } else {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(1, 0, 0);
        cam->SetViewUp(0, 0, 1);
    }

    auto sliceProperty = imageSlice->GetProperty();
    sliceProperty->SetLookupTable(lut);
    lut->SetMinimumTableValue(0, 0, 0, 1);
    lut->SetMaximumTableValue(1, 1, 1, 1);
    lut->Build();
    sliceProperty->SetUseLookupTableScalarRange(false);
}

void SliceRenderWidget::sharedViews(SliceRenderWidget* other1, SliceRenderWidget* other2)
{

    // adding callbacks to share windowlevel
    // windowing and slicing
    std::array<SliceRenderWidget*, 3> wids = { this, other1, other2 };

    // setting same lut
    for (auto& w : wids)
        w->lut = this->lut;

    for (std::size_t i = 0; i < 3; ++i) {
        auto w = wids[i];
        auto callback = vtkSmartPointer<WindowLevelSlicingModifiedCallback>::New();
        auto style = w->interactorStyle;
        for (std::size_t j = 0; j < 3; ++j) {
            if (i != j) {
                callback->imageSlices.push_back(wids[j]->imageSlice);
                callback->widgets.push_back(wids[j]->openGLWidget);
            }
        }
        for (auto ev : callback->eventTypes())
            style->AddObserver(ev, callback);
    }

    // windowlevel text
    {
        auto callback = vtkSmartPointer<TextModifiedCallback>::New();
        callback->textActorCorner->GetTextProperty()->SetColor(0.6, 0.5, 0.1);
        for (std::size_t i = 0; i < 3; ++i) {
            auto style = wids[i]->interactorStyle;
            for (auto ev : callback->eventTypes())
                style->AddObserver(ev, callback);
            if (i == 0)
                wids[i]->renderer->AddViewProp(callback->textActorCorner);
        }
    }
}
/*
void SliceRenderWidget::setupVolumePipeline()
{

    auto widget = openGLWidget[3];

    auto ren = vtkSmartPointer<vtkRenderer>::New();
    widget->renderWindow()->AddRenderer(ren);

    auto renderWindowInteractor = widget->interactor();
    auto interactorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    renderWindowInteractor->SetInteractorStyle(interactorStyle);

    // create tables
    auto ctf = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
    ctf->SetIndexedLookup(false); // true for indexed lookup
    ctf->SetDiscretize(false);
    auto otf = vtkSmartPointer<vtkPiecewiseFunction>::New();

    // create volume and mapper
    auto mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    mapper->SetInputConnection(imageSmoother->GetOutputPort());
    auto volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);
    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetIndependentComponents(true);
    volumeProperty->SetColor(ctf);
    volumeProperty->SetScalarOpacity(otf);
    volumeProperty->SetInterpolationTypeToLinear();
    volume->SetProperty(volumeProperty);

    ctf->AddRGBPoint(-3024, 0, 0, 0, 0.5, 0.0);
    ctf->AddRGBPoint(-1000, .62, .36, .18, 0.5, 0.0);
    ctf->AddRGBPoint(-500, .88, .60, .29, 0.33, 0.45);
    ctf->AddRGBPoint(3071, .83, .66, 1, 0.5, 0.0);

    otf->AddPoint(-3024, 0, 0.5, 0.0);
    otf->AddPoint(-1000, 0, 0.5, 0.0);
    otf->AddPoint(-500, 1.0, 0.33, 0.45);
    otf->AddPoint(3071, 1.0, 0.5, 0.0);

    ren->AddVolume(volume);
    ren->ResetCamera();

    auto camera = ren->GetActiveCamera();
    camera->SetPosition(56.8656, -297.084, 78.913);
    camera->SetFocalPoint(109.139, 120.604, 63.5486);
    camera->SetViewUp(-0.00782421, -0.0357807, -0.999329);
    camera->SetDistance(421.227);
    camera->SetClippingRange(146.564, 767.987);
}
*/

void SliceRenderWidget::Render(bool rezoom_camera)
{
    auto ps = renderer->GetActiveCamera()->GetParallelScale();
    renderer->ResetCamera();
    if (!rezoom_camera)
        renderer->GetActiveCamera()->SetParallelScale(ps);

    openGLWidget->renderWindow()->Render();
}

void SliceRenderWidget::setInteractionStyleTo3D()
{
    interactorStyle->SetInteractionModeToImage3D();
}

void SliceRenderWidget::setInteractionStyleToSlicing()
{
    interactorStyle->SetInteractionModeToImageSlicing();
}

void SliceRenderWidget::useFXAA(bool use)
{
    renderer->SetUseFXAA(use);
}

void SliceRenderWidget::setMultisampleAA(int samples)
{
    int s = std::max(samples, int { 0 });
    openGLWidget->renderWindow()->SetMultiSamples(s);
}

void SliceRenderWidget::setNewImageData(vtkImageData* data, bool rezoom_camera)
{
    if (data) {
        imageSlice->GetMapper()->SetInputData(data);
        Render(rezoom_camera);
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
            setNewImageData(vtkimage, true);
        }
    }

    m_data = data;
}