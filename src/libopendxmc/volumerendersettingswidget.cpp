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

#include <colormaps.hpp>
#include <volumerendersettingswidget.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

#include <vtkVolume.h>

template <typename T>
struct SettingsCollection {
    T* widget = nullptr;
    QHBoxLayout* layout = nullptr;
};

template <typename T>
SettingsCollection<T> getSettingsWidget(const QString& label, QWidget* parent)
{
    auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);

    auto l = new QLabel(label, parent);
    layout->addWidget(l, Qt::AlignLeft);
    T* w = nullptr;
    if constexpr (std::is_same<T, QSlider>::value) {
        w = new T(Qt::Horizontal, parent);
        w->setMinimum(0);
        w->setMaximum(100);
        w->setTracking(false);
        layout->addWidget(w, Qt::AlignRight);
    } else {
        w = new T(parent);
        layout->addWidget(w, Qt::AlignLeft);
    }

    layout->setStretch(1, 10);
    SettingsCollection<T> s;
    s.widget = w;
    s.layout = layout;
    return s;
}

VolumerenderSettingsWidget::VolumerenderSettingsWidget(VolumeRenderSettings* settings, QWidget* parent)
    : m_settings(settings)
    , QWidget(parent)
{
    if (!m_settings)
        return;
    if (!m_settings->valid())
        return;

    // Main layout
    auto layout = new QVBoxLayout(this);

    // jittering
    auto jittering = getSettingsWidget<QCheckBox>(tr("Use jittering"), this);
    jittering.widget->setChecked(m_settings->mapper->GetUseJittering());
    connect(jittering.widget, &QCheckBox::stateChanged, [=](int state) {
        m_settings->mapper->SetUseJittering(state != 0);
        m_settings->render();
    });
    layout->addLayout(jittering.layout);

    auto vprop = m_settings->getVolumeProperty();
    // shadebox
    auto shadebox = new QGroupBox(tr("Shading"), this);
    auto shade_layout = new QVBoxLayout(this);
    shade_layout->setContentsMargins(0, 0, 0, 0);
    shadebox->setLayout(shade_layout);
    shadebox->setCheckable(true);
    shadebox->setChecked(vprop->GetShade() == 1);
    connect(shadebox, &QGroupBox::toggled, [=](bool toggle) {
        vprop->SetShade(toggle ? 1 : 0);
        m_settings->render();
    });
    layout->addWidget(shadebox);

    // Global illumination reach
    auto gir = getSettingsWidget<QSlider>(tr("Global illumination reach"), shadebox);
    gir.widget->setValue(static_cast<int>(m_settings->mapper->GetGlobalIlluminationReach() * 100));
    connect(gir.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        m_settings->mapper->SetGlobalIlluminationReach(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(gir.layout);

    // Volumetric Scattering blending
    auto vsb = getSettingsWidget<QSlider>(tr("Volumetric scattering blending"), this);
    vsb.widget->setValue(static_cast<int>(m_settings->mapper->GetGlobalIlluminationReach() * 50));
    connect(vsb.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 50 };
        m_settings->mapper->SetVolumetricScatteringBlending(std::clamp(g, 0.f, 2.f));
        m_settings->render();
    });
    shade_layout->addLayout(vsb.layout);

    // Ambient
    auto ambient = getSettingsWidget<QSlider>(tr("Ambient"), this);
    ambient.widget->setValue(static_cast<int>(vprop->GetAmbient() * 100));
    connect(ambient.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        vprop->SetAmbient(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(ambient.layout);

    // Diffuse
    auto diffuse = getSettingsWidget<QSlider>(tr("Diffuse"), this);
    diffuse.widget->setValue(static_cast<int>(vprop->GetDiffuse() * 100));
    connect(diffuse.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        vprop->SetDiffuse(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(diffuse.layout);

    // Specular
    auto specular = getSettingsWidget<QSlider>(tr("Specular"), this);
    specular.widget->setValue(static_cast<int>(vprop->GetSpecular() * 100));
    connect(specular.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        vprop->SetSpecular(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(specular.layout);

    // Specular power
    auto specularpower = getSettingsWidget<QSlider>(tr("Specular power"), this);
    specularpower.widget->setValue(static_cast<int>(vprop->GetSpecularPower()));
    connect(specularpower.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value;
        vprop->SetSpecularPower(g);
        m_settings->render();
    });
    shade_layout->addLayout(specularpower.layout);

    // Scattering anisotropy
    auto sa = getSettingsWidget<QSlider>(tr("Scattering anisotropy"), this);
    sa.widget->setValue(static_cast<int>(vprop->GetScatteringAnisotropy() * 50) + 50);
    connect(sa.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 50 } - 1;
        vprop->SetScatteringAnisotropy(std::clamp(g, -1.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(sa.layout);

    // colortable selector
    auto color = getSettingsWidget<QComboBox>(tr("Color table"), this);
    for (const auto& [name, ct] : COLORMAPS) {
        color.widget->addItem(QString::fromStdString(name));
    }
    auto color_gray_index = color.widget->findText(QString::fromStdString("GRAY"));
    color.widget->setCurrentIndex(color_gray_index);
    connect(color.widget, &QComboBox::currentTextChanged, [=](const QString& cname) {
        this->setColorTable(cname.toStdString());
    });
    layout->addLayout(color.layout);

    // lutWidget
    m_lut_opacity_widget = new VolumeLUTWidget(m_settings, VolumeLUTWidget::LUTType::Opacity, this);
    layout->addWidget(m_lut_opacity_widget);
    setColorTable("GRAY");

    // lutWidget
    m_lut_gradient_widget = new VolumeLUTWidget(m_settings, VolumeLUTWidget::LUTType::Gradient, this);
    layout->addWidget(m_lut_gradient_widget);

    layout->addStretch();
    setLayout(layout);
}

void VolumerenderSettingsWidget::setColorTable(const std::string& ct)
{
    if (!COLORMAPS.contains(ct)) {
        return;
    }
    const auto& map = COLORMAPS.at(ct);
    m_lut_opacity_widget->setColorData(map);
}

void VolumerenderSettingsWidget::imageDataUpdated()
{
    auto data = m_settings->currentImageData;
    if (data) {
        m_lut_opacity_widget->imageDataUpdated();
        m_lut_gradient_widget->imageDataUpdated();
    }
}