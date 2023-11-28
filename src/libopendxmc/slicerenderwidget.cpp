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

#include <beamactorcontainer.hpp>
#include <colormaps.hpp>
#include <slicerenderwidget.hpp>

#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkImageProperty.h>
#include <vtkImageSliceMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRendererCollection.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>

#include <charconv>
#include <string>

constexpr std::array<double, 3> TEXT_COLOR = { 0.6, 0.5, 0.1 };

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
                textActorCorner->SetInput(txt.c_str());
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
    vtkSmartPointer<vtkTextActor> textActorCorner = nullptr;

protected:
private:
    TextModifiedCallback()
    {
        textActorCorner = vtkSmartPointer<vtkTextActor>::New();
        textActorCorner->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    }

    vtkSmartPointer<vtkCellPicker> picker = nullptr;
    TextModifiedCallback(const TextModifiedCallback&) = delete;
    void operator=(const TextModifiedCallback&) = delete;
};

SliceRenderWidget::SliceRenderWidget(int orientation, QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    openGLWidget = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(openGLWidget);

    this->setLayout(layout);

    // lut
    lut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();

    lut_windowing[DataContainer::ImageType::CT] = std::make_pair(100.0, 300.0);
    lut_windowing[DataContainer::ImageType::Density] = std::make_pair(1.0, 0.5);

    setupSlicePipeline(orientation);
}

void SliceRenderWidget::setupSlicePipeline(int orientation)
{

    // renderers
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->GetActiveCamera()->ParallelProjectionOn();
    renderer->SetBackground(0, 0, 0);

    // interaction style
    interactorStyle = vtkSmartPointer<CustomInteractorStyleImage>::New();
    interactorStyle->SetDefaultRenderer(renderer);
    interactorStyle->SetInteractionModeToImageSlicing();
    interactorStyle->AutoAdjustCameraClippingRangeOn();

    auto renderWindowInteractor = openGLWidget->interactor();
    renderWindowInteractor->SetInteractorStyle(interactorStyle);
    auto renWin = openGLWidget->renderWindow();
    renWin->AddRenderer(renderer);

    // reslice mapper
    auto imageMapper = vtkSmartPointer<vtkImageSliceMapper>::New();
    imageMapper->SliceFacesCameraOn();
    imageMapper->SliceAtFocalPointOn();
    imageMapper->ReleaseDataFlagOn();

    // image slice
    imageSlice = vtkSmartPointer<vtkImageActor>::New();
    imageSlice->SetMapper(imageMapper);
    renderer->AddActor(imageSlice);

    if (auto cam = renderer->GetActiveCamera(); orientation == 0) {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, 0, -1);
        cam->SetViewUp(0, 1, 0);

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
    sliceProperty->UseLookupTableScalarRangeOff();
    lut->SetMinimumTableValue(0, 0, 0, 1);
    lut->SetMaximumTableValue(1, 1, 1, 1);
    lut->UseBelowRangeColorOn();
    lut->SetBelowRangeColor(0, 0, 0, 0);
    lut->Build();

    lowerLeftText = vtkSmartPointer<vtkTextActor>::New();
    renderer->AddActor2D(lowerLeftText);
    auto txtStyle = lowerLeftText->GetTextProperty();
    txtStyle->SetColor(TEXT_COLOR.data());
    txtStyle->BoldOn();
}

void SliceRenderWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateTextPositions(true);
}

void SliceRenderWidget::updateTextPositions(bool render)
{
    if (lowerLeftText) {
        double size[2];
        lowerLeftText->GetSize(renderer, size);
        auto rsize = renderer->GetSize();
        lowerLeftText->SetPosition(rsize[0] - size[0] - 5, 5);
        if (render)
            Render();
    }
}

void SliceRenderWidget::sharedViews(std::vector<SliceRenderWidget*> wids)
{
    wids.insert(wids.begin(), this);
    // setting same lut
    for (auto& w : wids) {
        w->lut = this->lut;
        w->imageSlice->GetProperty()->SetLookupTable(lut);
    }

    for (std::size_t i = 0; i < wids.size(); ++i) {
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

    auto txtStyle = vtkSmartPointer<vtkTextProperty>::New();
    txtStyle->SetColor(TEXT_COLOR.data());
    txtStyle->BoldOn();

    // windowlevel text
    {
        auto callback = vtkSmartPointer<TextModifiedCallback>::New();
        callback->textActorCorner->SetTextProperty(txtStyle);
        for (std::size_t i = 0; i < wids.size(); ++i) {
            auto style = wids[i]->interactorStyle;
            for (auto ev : callback->eventTypes())
                style->AddObserver(ev, callback);
            if (i == 0)
                wids[i]->renderer->AddActor(callback->textActorCorner);
        }
    }

    // colorbar
    {
        auto scalarColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
        scalarColorBar->SetNumberOfLabels(2);
        scalarColorBar->SetLookupTable(wids[0]->lut);
        scalarColorBar->SetUnconstrainedFontSize(true);
        scalarColorBar->SetBarRatio(0.1);
        scalarColorBar->SetLabelTextProperty(txtStyle);
        scalarColorBar->SetTextPositionToPrecedeScalarBar();
        scalarColorBar->AnnotationTextScalingOff();
        wids[0]->renderer->AddActor(scalarColorBar);
    }

    // remove all but one lower left text actor
    for (std::size_t i = 1; i < wids.size(); ++i)
        wids[i]->lowerLeftText = nullptr;
}
void SliceRenderWidget::sharedViews(SliceRenderWidget* other1, SliceRenderWidget* other2)
{
    std::vector<SliceRenderWidget*> w = { other1, other2 };
    w.push_back(other1);
    w.push_back(other2);
    sharedViews(w);
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

void SliceRenderWidget::setInterpolationType(int type)
{
    imageSlice->GetProperty()->SetInterpolationType(type);
    Render();
}

void SliceRenderWidget::switchLUTtable(DataContainer::ImageType type, int n_colors)
{
    auto prop = imageSlice->GetProperty();
    lut_windowing[lut_current_type] = std::make_pair(prop->GetColorLevel(), prop->GetColorWindow());

    if (type == DataContainer::ImageType::Material || type == DataContainer::ImageType::Organ) {
        if (n_colors > 1 && lut->GetNumberOfColors() != n_colors) {
            lut->SetNumberOfTableValues(n_colors);
            lut->SetTableValue(0, 0, 0, 0, 0);
            for (int i = 1; i < n_colors; ++i) {
                auto rgba = Colormaps::discreetColor(i);
                lut->SetTableValue(i, rgba.data());
            }
            lut->SetTableRange(0, n_colors - 1);
            lut->Build();
        }
        prop->UseLookupTableScalarRangeOn();
    } else {
        lut->IndexedLookupOff();
        lut->SetNumberOfTableValues(256);
        lut->SetValueRange(0, 1);
        lut->SetMinimumTableValue(0, 0, 0, 1);
        lut->SetMaximumTableValue(1, 1, 1, 1);
        lut->ForceBuild();

        if (type == DataContainer::ImageType::Density) {
            // replacing with turbo cmap
            const auto map = Colormaps::colormapLongForm("TURBO");
            for (int i = 0; i < map.size() / 3; ++i) {
                const auto ii = i * 3;
                lut->SetTableValue(i, map[ii], map[ii + 1], map[ii + 2], 1.0);
            }
        } else if (type == DataContainer::ImageType::Dose) {
            // replacing with turbo cmap
            const auto map = Colormaps::colormapLongForm("TURBO");
            for (int i = 0; i < map.size() / 3; ++i) {
                const auto ii = i * 3;
                lut->SetTableValue(i, map[ii], map[ii + 1], map[ii + 2], 1.0);
            }
        }

        prop->UseLookupTableScalarRangeOff();

        if (lut_windowing.contains(type)) {
            prop->SetColorLevel(lut_windowing[type].first);
            prop->SetColorWindow(lut_windowing[type].second);
        }
    }
    lut_current_type = type;
    imageSlice->Update();
}

void SliceRenderWidget::showData(DataContainer::ImageType type)
{
    if (!m_data)
        return;
    if (m_data->hasImage(type)) {
        auto vtkimage = m_data->vtkImage(type);
        if (type == DataContainer::ImageType::Material) {
            auto max_val = static_cast<int>(vtkimage->GetScalarRange()[1]);
            switchLUTtable(type, max_val + 1);
        } else {
            switchLUTtable(type);
        }
        if (lowerLeftText) {
            lowerLeftText->SetInput(m_data->units(type).c_str());
            updateTextPositions();
        }
        setNewImageData(vtkimage, false);
    }
}

int argmax3(double v[3])
{
    if (std::abs(v[0]) > std::abs(v[1]) && std::abs(v[0]) > std::abs(v[2]))
        return 0;
    else if (std::abs(v[1]) > std::abs(v[0]) && std::abs(v[1]) > std::abs(v[2]))
        return 1;
    return 2;
}

void SliceRenderWidget::Render(bool reset_camera)
{
    if (reset_camera) {
        renderer->ResetCamera();
        auto camera = renderer->GetActiveCamera();
        double dir[3];
        camera->GetDirectionOfProjection(dir);
        int dIdx = argmax3(dir);

        double fpoint[3], pos[3];
        camera->GetPosition(pos);
        camera->GetFocalPoint(pos);
        for (int i = 0; i < 3; ++i) {
            if (i != dIdx) {
                fpoint[i] = 0;
                pos[i] = 0;
            }
        }
        camera->SetPosition(pos);
        camera->SetFocalPoint(fpoint);
    }
    openGLWidget->renderWindow()->Render();
}

void SliceRenderWidget::setNewImageData(vtkSmartPointer<vtkImageData> data, bool rezoom_camera)
{
    if (data) {
        imageSlice->GetMapper()->SetInputData(data);
        imageSlice->SetDisplayExtent(data->GetExtent());
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
        } else if (data->hasImage(DataContainer::ImageType::Density) && uid_is_new) {
            auto vtkimage = data->vtkImage(DataContainer::ImageType::Density);
            setNewImageData(vtkimage, true);
        }
    }

    m_data = data;
}

void SliceRenderWidget::addActor(vtkSmartPointer<vtkActor> actor)
{
    renderer->AddActor(actor);
    Render();
}

void SliceRenderWidget::removeActor(vtkSmartPointer<vtkActor> actor)
{
    renderer->RemoveActor(actor);
    Render();
}