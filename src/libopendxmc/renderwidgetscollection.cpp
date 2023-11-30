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
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSLider>
#include <QSizePolicy>

std::shared_ptr<DataContainer> generateSampleData()
{
    auto data = std::make_shared<DataContainer>();
    data->setDimensions({ 8, 8, 8 });
    data->setSpacing({ 1, 1, 1 });
    std::vector<double> im(8 * 8 * 8, 0);
    for (int i = 0; i < im.size(); ++i)
        im[i] = i;
    im[0] = -500;
    im.back() = 500;
    data->setImageArray(DataContainer::ImageType::CT, im);
    return data;
}

RenderWidgetsCollection::RenderWidgetsCollection(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(200);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    for (std::size_t i = 0; i < 3; ++i)
        m_slice_widgets[i] = new SliceRenderWidget(i, this);

    layout->addWidget(m_slice_widgets[0], 0, 0);
    layout->addWidget(m_slice_widgets[1], 1, 0);
    layout->addWidget(m_slice_widgets[2], 1, 1);

    m_volume_widget = new VolumerenderWidget(this);
    layout->addWidget(m_volume_widget, 0, 1);

    m_slice_widgets[0]->sharedViews(m_slice_widgets[1], m_slice_widgets[2]);

    // data type selector
    m_data_type_selector = new QComboBox(this);
    connect(m_data_type_selector, &QComboBox::activated, [this](int idx) {
        auto type = static_cast<DataContainer::ImageType>(m_data_type_selector->currentData().toInt());
        this->showData(type);
    });

    updateImageData(generateSampleData());
    this->setLayout(layout);
}

void RenderWidgetsCollection::addActor(std::shared_ptr<BeamActorContainer> actor)
{
    for (auto& bm : m_beamActorsBuffer) {
        if (bm.beamActorContainer == actor) {
            return;
        }
    }

    m_beamActorsBuffer.push_back({ actor });
    if (m_show_beam_actors) {
        auto& a = m_beamActorsBuffer.back();
        m_volume_widget->addActor(a.getNewActor());
        for (auto& w : m_slice_widgets)
            w->addActor(a.getNewActor());
    }
}

void RenderWidgetsCollection::removeActor(std::shared_ptr<BeamActorContainer> actor)
{
    std::size_t idx = m_beamActorsBuffer.size();
    for (std::size_t i = 0; i < m_beamActorsBuffer.size(); ++i) {
        auto& buf = m_beamActorsBuffer[i];
        auto cact = buf.beamActorContainer;
        if (cact == actor) {
            idx = i;
            for (auto vtkactor : buf.actors) {
                m_volume_widget->removeActor(vtkactor);
                for (auto& w : m_slice_widgets)
                    w->removeActor(vtkactor);
            }
        }
    }

    if (idx != m_beamActorsBuffer.size())
        m_beamActorsBuffer.erase(m_beamActorsBuffer.begin() + idx);
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
    if (current_data != m_data_type_selector->currentData().toInt()) {
        auto type = static_cast<DataContainer::ImageType>(m_data_type_selector->currentData().toInt());
        showData(type);
    }
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
    for (auto& w : m_slice_widgets)
        w->setInteractionStyleToSlicing();
}

void RenderWidgetsCollection::setInteractionStyleTo3D()
{
    for (auto& w : m_slice_widgets)
        w->setInteractionStyleTo3D();
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
    if (state == 0) {
        for (auto& coll : m_beamActorsBuffer) {
            for (auto& vtkactor : coll.actors) {
                m_volume_widget->removeActor(vtkactor);
                for (auto& w : m_slice_widgets)
                    w->removeActor(vtkactor);
            }
            coll.actors.clear();
        }
    } else {
        for (auto& coll : m_beamActorsBuffer) {
            m_volume_widget->addActor(coll.getNewActor());
            for (auto& w : m_slice_widgets)
                w->addActor(coll.getNewActor());
        }
    }
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

    auto show_beams = addWidgetAndLabel<QCheckBox>(tr("Show beam outlines"), layout, wid);
    show_beams->setChecked(true);
    connect(show_beams, &QCheckBox::stateChanged, this, &RenderWidgetsCollection::setBeamActorsVisible);

    auto sliceg = new QGroupBox(tr("Slice render settings"), wid);
    auto sliceg_layout = new QVBoxLayout;
    sliceg_layout->setContentsMargins(0, 0, 0, 0);
    sliceg->setLayout(sliceg_layout);
    layout->addWidget(sliceg);

    auto inter_type = addWidgetAndLabel<QComboBox>(tr("Set interpolation type"), sliceg_layout, parent);
    inter_type->addItem(tr("Nearest"));
    inter_type->addItem(tr("Linear"));
    inter_type->addItem(tr("Cubic"));
    inter_type->setCurrentIndex(1);
    connect(inter_type, &QComboBox::currentIndexChanged, this, &RenderWidgetsCollection::setInterpolationType);

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