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
#include <renderwidgetscollection.hpp>
#include <volumerendersettingswidget.hpp>

#include <QCheckBox>
#include <QColorDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkImageProperty.h>
#include <vtkInteractorStyleImage.h>

class WindowLevelSlicingModifiedCallback : public vtkCallbackCommand {
public:
    static WindowLevelSlicingModifiedCallback* New()
    {
        return new WindowLevelSlicingModifiedCallback;
    }

    double pointPlaneDistance(const std::array<double, 3>& pointPlane, const std::array<double, 3>& normal, const std::array<double, 3>& point)
    {
        const auto diff = normal[0] * (point[0] - pointPlane[0]) + normal[1] * (point[1] - pointPlane[1]) + normal[2] * (point[2] - pointPlane[2]);
        return diff;
    }

    void translateFocalPoint(std::array<double, 3>& pointPlane, const std::array<double, 3>& normal, const std::array<double, 3>& point)
    {
        const auto d = pointPlaneDistance(pointPlane, normal, point);
        for (int i = 0; i < 3; ++i)
            pointPlane[i] += normal[i] * d;
    }

    // Here we Create a vtkCallbackCommand and reimplement it.
    void Execute(vtkObject* caller, unsigned long evId, void*) override
    {
        if (evId == vtkCommand::EndWindowLevelEvent || evId == vtkCommand::WindowLevelEvent) {
            // Note the use of reinterpret_cast to cast the caller to the expected type.
            auto style = reinterpret_cast<vtkInteractorStyleImage*>(caller);
            auto property = style->GetCurrentImageProperty();
            if (property) {

                auto ww = property->GetColorWindow();
                auto wl = property->GetColorLevel();
                if (windowLevelText) {
                    std::array<char, 10> buffer;
                    std::string txt = "WL: ";
                    if (auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), wl, std::chars_format::general, 3); ec == std::errc()) {
                        txt += std::string_view(buffer.data(), ptr);
                    }
                    txt += " WW: ";
                    if (auto [ptr, ec] = std::to_chars(buffer.data(), buffer.data() + buffer.size(), ww, std::chars_format::general, 3); ec == std::errc()) {
                        txt += std::string_view(buffer.data(), ptr);
                    }
                    windowLevelText->SetInput(txt.c_str());
                }
                for (auto wid : widgets) {
                    auto slice = wid->imageSlice();
                    auto p = slice->GetProperty();
                    p->SetColorWindow(ww);
                    p->SetColorLevel(wl);
                    wid->Render();
                }
            }

        } else if (evId == vtkCommand::StartPickEvent) {
            auto style = reinterpret_cast<vtkInteractorStyleImage*>(caller);
            auto currentRenderer = style->GetCurrentRenderer();
            std::array<int, 2> eventPos;
            style->GetInteractor()->GetLastEventPosition(eventPos.data());
            m_picker->Pick(eventPos[0], eventPos[1], 0, currentRenderer);
            m_picked_actor = m_picker->GetActor();
            m_picker->GetPickPosition(m_picked_actor_pos.data());
        } else if (evId == vtkCommand::EndPickEvent) {
            if (m_picked_actor) {
                std::array<double, 3> dist;
                m_picked_actor->GetPosition(dist.data());
                for (auto& b : *beamItems) {
                    for (std::size_t i = 0; i < b.windowActors.size(); ++i) {
                        if (b.windowActors[i] == m_picked_actor) {
                            b.beam->translate(dist);
                        }
                    }
                }
                m_picked_actor->SetPosition(0, 0, 0);
                m_picked_actor = nullptr;
                for (auto& wid : widgets)
                    wid->Render();
                volumeWidget->Render();
            }
        } else if (evId == vtkCommand::PickEvent) {
            if (m_picked_actor) {
                auto style = reinterpret_cast<vtkInteractorStyleImage*>(caller);
                auto currentRenderer = style->GetCurrentRenderer();
                std::array<int, 2> eventPos;
                style->GetInteractor()->GetLastEventPosition(eventPos.data());
                std::array<double, 3> cpos;
                m_picker->Pick(eventPos[0], eventPos[1], 0, currentRenderer);
                m_picker->GetPickPosition(cpos.data());
                std::array<double, 3> dist = {
                    cpos[0] - m_picked_actor_pos[0],
                    cpos[1] - m_picked_actor_pos[1],
                    cpos[2] - m_picked_actor_pos[2],
                };

                auto camera = currentRenderer->GetActiveCamera();
                std::array<double, 3> normal;
                camera->GetViewPlaneNormal(normal.data());
                const auto dn = normal[0] * dist[0] + normal[1] * dist[1] + normal[2] * dist[2];
                dist[0] -= dn * normal[0];
                dist[1] -= dn * normal[1];
                dist[2] -= dn * normal[2];

                m_picked_actor->SetPosition(dist[0], dist[1], dist[2]);
                for (auto& wid : widgets)
                    wid->Render();

            } else {
                auto style = reinterpret_cast<vtkInteractorStyleImage*>(caller);
                auto currentRenderer = style->GetCurrentRenderer();
                std::array<int, 2> eventPos;
                style->GetInteractor()->GetLastEventPosition(eventPos.data());
                if (this->m_picker->Pick(eventPos[0], eventPos[1], 0, currentRenderer) > 0) {
                    std::array<double, 3> pos;
                    m_picker->GetPickPosition(pos.data());
                    auto actor = m_picker->GetActor();
                    if (!actor) {
                        for (auto& wid : widgets) {
                            auto renderer = wid->renderer();
                            if (renderer != currentRenderer) {
                                auto camera = renderer->GetActiveCamera();
                                std::array<double, 3> normal;
                                camera->GetViewPlaneNormal(normal.data());
                                std::array<double, 3> focalPoint;
                                camera->GetFocalPoint(focalPoint.data());

                                translateFocalPoint(focalPoint, normal, pos);
                                const auto dist = pointPlaneDistance(focalPoint, normal, pos);

                                camera->SetFocalPoint(focalPoint.data());
                                wid->Render();
                            }
                        }
                    }
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
        cmds.push_back(vtkCommand::StartPickEvent);
        cmds.push_back(vtkCommand::EndPickEvent);
        return cmds;
    }

    // Set pointers to any clientData or callData here.
    std::array<SliceRenderWidget*, 3> widgets = { nullptr, nullptr, nullptr };
    VolumerenderWidget* volumeWidget = nullptr;
    std::vector<RenderWidgetsCollection::BeamBufferItem>* beamItems = nullptr;
    vtkSmartPointer<vtkTextActor> windowLevelText = nullptr;

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
        m_picker = vtkSmartPointer<vtkCellPicker>::New();
    }

    vtkSmartPointer<vtkCellPicker> m_picker = nullptr;
    vtkActor* m_picked_actor = nullptr;
    std::array<double, 3> m_picked_actor_pos = { 0, 0, 0 };
    WindowLevelSlicingModifiedCallback(const WindowLevelSlicingModifiedCallback&) = delete;
    void operator=(const WindowLevelSlicingModifiedCallback&) = delete;
};

RenderWidgetsCollection::RenderWidgetsCollection(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    m_slice_widgets[0] = new SliceRenderWidget(0, true, true, this);
    m_slice_widgets[1] = new SliceRenderWidget(1, false, false, this);
    m_slice_widgets[2] = new SliceRenderWidget(2, false, false, this);

    layout->addWidget(m_slice_widgets[0], 0, 0);
    layout->addWidget(m_slice_widgets[1], 1, 0);
    layout->addWidget(m_slice_widgets[2], 1, 1);

    m_volume_widget = new VolumerenderWidget(this);
    layout->addWidget(m_volume_widget, 0, 1);

    // adding callbacks
    auto sliceCollectionCmd = vtkSmartPointer<WindowLevelSlicingModifiedCallback>::New();
    sliceCollectionCmd->widgets = m_slice_widgets;
    sliceCollectionCmd->volumeWidget = m_volume_widget;
    sliceCollectionCmd->beamItems = &m_beamBuffer;
    sliceCollectionCmd->windowLevelText = m_slice_widgets[0]->windowTextItem();
    for (auto w : m_slice_widgets) {
        w->registerStyleCallback(sliceCollectionCmd, sliceCollectionCmd->eventTypes());
    }

    // data type selector
    m_data_type_selector = new QComboBox(this);
    connect(m_data_type_selector, &QComboBox::activated, [this](int idx) {
        auto type = static_cast<DataContainer::ImageType>(m_data_type_selector->currentData().toInt());
        this->showData(type);
    });

    this->setLayout(layout);
}

void RenderWidgetsCollection::addBeam(std::shared_ptr<BeamActorContainer> beam)
{
    for (auto& bm : m_beamBuffer) {
        if (bm.beam == beam) {
            return;
        }
    }

    m_beamBuffer.push_back({ beam });

    if (m_show_beam_actors) {
        auto& b = m_beamBuffer.back();
        m_volume_widget->addActor(b.volumeActor);
        for (std::size_t i = 0; i < b.windowActors.size(); ++i)
            m_slice_widgets[i]->addActor(b.windowActors[i]);
    }
}

void RenderWidgetsCollection::removeBeam(std::shared_ptr<BeamActorContainer> beam)
{
    std::size_t i = 0;
    while (i < m_beamBuffer.size()) {
        if (m_beamBuffer[i].beam == beam) {
            m_volume_widget->removeActor(m_beamBuffer[i].volumeActor);
            for (std::size_t j = 0; j < m_beamBuffer[i].windowActors.size(); ++j)
                m_slice_widgets[j]->removeActor(m_beamBuffer[i].windowActors[j]);
            m_beamBuffer.erase(m_beamBuffer.begin() + i);
        } else {
            ++i;
        }
    }
}

void RenderWidgetsCollection::updateImageData(std::shared_ptr<DataContainer> data)
{
    const auto current_data = m_data_type_selector->currentData().toInt();
    m_data_type_selector->clear();
    if (data) {
        auto types = data->getAvailableImages();
        for (auto t : types) {
            auto d = QVariant(static_cast<int>(t));
            auto name = QString::fromStdString(data->getImageAsString(t));
            m_data_type_selector->addItem(name, d);
        }
    }
    for (auto& w : m_slice_widgets)
        w->updateImageData(data);
    m_volume_widget->updateImageData(data);
}

void RenderWidgetsCollection::showData(DataContainer::ImageType type)
{
    for (auto& w : m_slice_widgets)
        w->showData(type);
    m_volume_widget->showData(type);
}

void RenderWidgetsCollection::useFXAA(bool on)
{
    for (auto& w : m_slice_widgets)
        w->useFXAA(on);
}

void RenderWidgetsCollection::setInterpolationType(int type)
{
    for (auto& w : m_slice_widgets)
        w->setInterpolationType(type);
}

void RenderWidgetsCollection::setMultisampleAA(int samples)
{
    for (auto& w : m_slice_widgets)
        w->setMultisampleAA(samples);
}

void RenderWidgetsCollection::setInteractionStyleToSlicing()
{
    // not implemented
}

void RenderWidgetsCollection::setInteractionStyleTo3D()
{
    // not implemented
}

void RenderWidgetsCollection::setImageSmoothing(int pixels)
{
    auto pix = static_cast<double>(pixels) / 4 + .5;
    for (auto& w : m_slice_widgets)
        w->setImageSmoothing(pix);
}

void RenderWidgetsCollection::Render()
{
    m_volume_widget->Render();
    for (auto& w : m_slice_widgets)
        w->Render();
}

VolumerenderSettingsWidget* RenderWidgetsCollection::volumerenderSettingsWidget(QWidget* parent)
{
    if (!parent)
        parent = this;

    auto wid = m_volume_widget->createSettingsWidget(parent);

    return wid;
}

template <typename T>
T* addWidgetAndLabel(const QString& txt, QVBoxLayout* layout, QWidget* parent = nullptr)
{
    auto tlayout = new QHBoxLayout;
    tlayout->setContentsMargins(0, 0, 0, 0);
    tlayout->setSpacing(0);
    auto tlabel = new QLabel(txt, parent);
    tlayout->addWidget(tlabel);

    T* wid = nullptr;
    if constexpr (std::is_same_v<T, QSlider>)
        wid = new QSlider(Qt::Horizontal, parent);
    else
        wid = new T(parent);

    tlayout->addWidget(wid);
    layout->addLayout(tlayout);
    return wid;
}

void RenderWidgetsCollection::setBeamActorsVisible(int state)
{
    m_show_beam_actors = state != 0;
    for (auto& b : m_beamBuffer) {
        if (m_show_beam_actors) {
            m_volume_widget->addActor(b.volumeActor);
            for (std::size_t i = 0; i < b.windowActors.size(); ++i)
                m_slice_widgets[i]->addActor(b.windowActors[i]);
        } else {
            m_volume_widget->removeActor(b.volumeActor);
            for (std::size_t i = 0; i < b.windowActors.size(); ++i)
                m_slice_widgets[i]->removeActor(b.windowActors[i]);
        }
    }
}

void RenderWidgetsCollection::setBackgroundColor(const QColor& c)
{
    const double r = c.redF();
    const double g = c.greenF();
    const double b = c.blueF();
    for (auto& w : m_slice_widgets)
        w->setBackgroundColor(r, g, b);
    m_volume_widget->setBackgroundColor(r, g, b);
}

void RenderWidgetsCollection::setUseCTBackground(bool on)
{
    for (auto& w : m_slice_widgets)
        w->setUseCTDataBackground(on);
}

QWidget* RenderWidgetsCollection::createRendersettingsWidget(QWidget* parent)
{

    auto wid = new QWidget(parent);
    auto layout = new QVBoxLayout;
    wid->setLayout(layout);
    layout->setContentsMargins(0, 0, 0, 0);

    auto vol_select_label = new QLabel(tr("Select volume"), wid);
    vol_select_label->setAlignment(Qt::AlignHCenter);
    layout->addWidget(vol_select_label);
    layout->addWidget(m_data_type_selector);

    auto color_picker = addWidgetAndLabel<QPushButton>(tr("Background color"), layout, wid);
    color_picker->setFlat(true);
    color_picker->setAutoFillBackground(true);
    {
        auto pal = color_picker->palette();
        pal.setColor(QPalette::Button, Qt::black);
        color_picker->setPalette(pal);
    }
    connect(color_picker, &QPushButton::clicked, [this, color_picker]() {
        auto color = QColorDialog::getColor(Qt::white, nullptr, tr("Select background color"));
        this->setBackgroundColor(color);
        auto pal = color_picker->palette();
        pal.setColor(QPalette::Button, color);
        color_picker->setPalette(pal);
    });

    auto show_beams = addWidgetAndLabel<QCheckBox>(tr("Show beam outlines"), layout, wid);
    show_beams->setChecked(true);
    connect(show_beams, &QCheckBox::stateChanged, this, &RenderWidgetsCollection::setBeamActorsVisible);

    auto sliceg = new QGroupBox(tr("Slice render settings"), wid);
    auto sliceg_layout = new QVBoxLayout;
    sliceg_layout->setContentsMargins(0, 0, 0, 0);
    sliceg->setLayout(sliceg_layout);
    layout->addWidget(sliceg);

    auto ctbackground = addWidgetAndLabel<QCheckBox>(tr("Use CT background"), sliceg_layout, parent);
    ctbackground->setChecked(false);
    connect(ctbackground, &QCheckBox::stateChanged, this, &RenderWidgetsCollection::setUseCTBackground);

    auto inter_type = addWidgetAndLabel<QComboBox>(tr("Set interpolation type"), sliceg_layout, parent);
    inter_type->addItem(tr("Nearest"));
    inter_type->addItem(tr("Linear"));
    inter_type->addItem(tr("Cubic"));
    inter_type->addItem(tr("Sinc"));
    inter_type->setCurrentIndex(3);
    connect(inter_type, &QComboBox::currentIndexChanged, this, &RenderWidgetsCollection::setInterpolationType);

    /* auto smoother = addWidgetAndLabel<QDoubleSpinBox>(tr("Image smoothing"), sliceg_layout, parent);
     smoother->setRange(0, 10);
     smoother->setValue(0);
     smoother->setSingleStep(0.2);
     smoother->setDecimals(2);
     connect(smoother, &QDoubleSpinBox::valueChanged, this, &RenderWidgetsCollection::setImageSmoothing);
     */
    auto smoother = addWidgetAndLabel<QSlider>(tr("Image display smoothing "), sliceg_layout, parent);
    smoother->setRange(0, 5);
    smoother->setValue(0);
    smoother->setSingleStep(1);
    smoother->setTracking(false);
    // smoother->setDecimals(2);
    connect(smoother, &QSlider::valueChanged, this, &RenderWidgetsCollection::setImageSmoothing);

    // These settings are unusable per now
    /*
    auto set_use_FXAA = new QCheckBox(tr("Use FXAA"));
    set_use_FXAA->setChecked(false);
    layout->addWidget(set_use_FXAA);
    connect(set_use_FXAA, &QCheckBox::stateChanged, [this](int state) { this->useFXAA(state != 0); });

    auto msaa_slider = addWidgetAndLabel<QSlider>(tr("Use multisamples"), layout, wid);
    msaa_slider->setRange(0, 64);
    msaa_slider->setValue(0);
    connect(msaa_slider, &QSlider::valueChanged, this, &RenderWidgetsCollection::setMultisampleAA);

    auto interaction = addWidgetAndLabel<QComboBox>(tr("Set interaction mode"), layout, parent);
    interaction->addItem(tr("Slicing"));
    interaction->addItem(tr("3D"));
    interaction->setCurrentIndex(0);
    connect(interaction, &QComboBox::currentIndexChanged, [this](int idx) { if (idx ==0) this->setInteractionStyleToSlicing(); else this->setInteractionStyleTo3D(); });
    */

    auto volumeg = new QGroupBox(tr("Volume render settings"), wid);
    auto volumeg_layout = new QVBoxLayout;
    volumeg->setLayout(volumeg_layout);
    layout->addWidget(volumeg);

    auto volumerenderettings = m_volume_widget->createSettingsWidget(wid);
    volumeg_layout->addWidget(volumerenderettings);

    return wid;
}
