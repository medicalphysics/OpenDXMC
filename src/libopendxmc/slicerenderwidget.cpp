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
#include <vtkCellPicker.h>
#include <vtkCornerAnnotation.h>
#include <vtkImageGaussianSmooth.h>
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

        // colorbar
        // auto scalarColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
        // scalarColorBar->SetMaximumWidthInPixels(200);
        // scalarColorBar->AnnotationTextScalingOff();

        // reslice mapper
        auto imageMapper = vtkSmartPointer<vtkOpenGLImageSliceMapper>::New();
        imageMapper->SetInputConnection(imageSmoother->GetOutputPort());
        imageMapper->SliceFacesCameraOn();
        imageMapper->SetSliceAtFocalPoint(true);

        /* auto imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
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
    // windowing and slicing
    for (std::size_t i = 0; i < 3; ++i) {
        auto callback = vtkSmartPointer<WindowLevelSlicingModifiedCallback>::New();
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
    // windowlevel text
    {
        auto callback = vtkSmartPointer<TextModifiedCallback>::New();
        callback->textActorCorner->GetTextProperty()->SetColor(0.6, 0.5, 0.1);
        for (std::size_t i = 0; i < 3; ++i) {
            auto style = interactorStyle[i];
            for (auto ev : callback->eventTypes())
                style->AddObserver(ev, callback);
            if (i == 0)
                renderer[i]->AddViewProp(callback->textActorCorner);
        }
    }
}

void SliceRenderWidget::Render(bool rezoom_camera)
{
    for (auto& ren : renderer) {
        auto ps = ren->GetActiveCamera()->GetParallelScale();
        ren->ResetCamera();
        if (!rezoom_camera)
            ren->GetActiveCamera()->SetParallelScale(ps);
    }
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

void SliceRenderWidget::setNewImageData(vtkImageData* data, bool rezoom_camera)
{
    if (data) {
        imageSmoother->SetInputData(data);
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
