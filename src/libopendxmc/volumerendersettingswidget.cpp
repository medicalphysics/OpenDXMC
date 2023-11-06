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

#include <volumerendersettingswidget.hpp>

#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QVBoxLayout>

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

VolumerenderSettingsWidget::VolumerenderSettingsWidget(vtkOpenGLGPUVolumeRayCastMapper* mapper, vtkVolumeProperty* prop, QWidget* parent)
    : m_mapper(mapper)
    , m_property(prop)
    , QWidget(parent)
{
    if (!mapper || !prop)
        return;

    // Main layout
    auto layout = new QVBoxLayout(this);

    // jittering
    auto jittering = getSettingsWidget<QCheckBox>(tr("Use jittering"), this);
    jittering.widget->setChecked(m_mapper->GetUseJittering());
    connect(jittering.widget, &QCheckBox::stateChanged, [=](int state) {
        m_mapper->SetUseJittering(state != 0);
        emit this->renderSettingsChanged();
    });
    layout->addLayout(jittering.layout);

    // shadebox
    auto shadebox = new QGroupBox(tr("Shading"), this);
    auto shade_layout = new QVBoxLayout(this);
    shade_layout->setContentsMargins(0, 0, 0, 0);
    shadebox->setLayout(shade_layout);
    shadebox->setCheckable(true);
    shadebox->setChecked(m_property->GetShade() == 1);
    connect(shadebox, &QGroupBox::toggled, [=](bool toggle) {
        m_property->SetShade(toggle ? 1 : 0);
        emit this->renderSettingsChanged();
    });
    layout->addWidget(shadebox);

    // Global illumination reach
    auto gir = getSettingsWidget<QSlider>(tr("Global illumination reach"), shadebox);
    gir.widget->setValue(static_cast<int>(m_mapper->GetGlobalIlluminationReach() * 100));
    connect(gir.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        m_mapper->SetGlobalIlluminationReach(g);
        emit this->renderSettingsChanged();
    });
    shade_layout->addLayout(gir.layout);

    // Volumetric Scattering blending
    auto vsb = getSettingsWidget<QSlider>(tr("Volumetric scattering blending"), this);
    vsb.widget->setValue(static_cast<int>(m_mapper->GetGlobalIlluminationReach() * 50));
    connect(vsb.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 50 };
        m_mapper->SetVolumetricScatteringBlending(g);
        emit this->renderSettingsChanged();
    });
    shade_layout->addLayout(vsb.layout);

    // Ambient
    auto ambient = getSettingsWidget<QSlider>(tr("Ambient"), this);
    ambient.widget->setValue(static_cast<int>(m_property->GetAmbient() * 100));
    connect(ambient.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        m_property->SetAmbient(g);
        emit this->renderSettingsChanged();
    });
    shade_layout->addLayout(ambient.layout);

    // Diffuse
    auto diffuse = getSettingsWidget<QSlider>(tr("Diffuse"), this);
    diffuse.widget->setValue(static_cast<int>(m_property->GetDiffuse() * 100));
    connect(diffuse.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        m_property->SetDiffuse(g);
        emit this->renderSettingsChanged();
    });
    shade_layout->addLayout(diffuse.layout);

    // Specular
    auto specular = getSettingsWidget<QSlider>(tr("Specular"), this);
    specular.widget->setValue(static_cast<int>(m_property->GetSpecular() * 100));
    connect(specular.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 100 };
        m_property->SetSpecular(g);
        emit this->renderSettingsChanged();
    });
    shade_layout->addLayout(specular.layout);

    // Specular power
    auto specularpower = getSettingsWidget<QSlider>(tr("Specular power"), this);
    specularpower.widget->setValue(static_cast<int>(m_property->GetSpecularPower()));
    connect(specularpower.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 1 };
        m_property->SetSpecularPower(g);
        emit this->renderSettingsChanged();
    });
    shade_layout->addLayout(specularpower.layout);

    // Scattering anisotropy  -1
    auto sa = getSettingsWidget<QSlider>(tr("Scattering anisotropy"), this);
    sa.widget->setValue(static_cast<int>(m_property->GetScatteringAnisotropy() * 50) + 50);
    connect(sa.widget, &QSlider::valueChanged, [=](int value) {
        const float g = value / float { 50 } - 1;
        m_property->SetScatteringAnisotropy(g);
        emit this->renderSettingsChanged();
    });
    shade_layout->addLayout(sa.layout);

    auto slider = getSettingsWidget<QSlider>(tr("Slider1"), this);
    layout->addLayout(slider.layout);

    auto slider2 = getSettingsWidget<QSlider>(tr("Slider2"), this);
    layout->addLayout(slider2.layout);

    layout->addStretch();
    setLayout(layout);
}

void VolumerenderSettingsWidget::setMapper(vtkOpenGLGPUVolumeRayCastMapper* mapper)
{
    m_mapper = mapper;
}

void VolumerenderSettingsWidget::setVolumeProperty(vtkVolumeProperty* prop)
{
    m_property = prop;
}

void VolumerenderSettingsWidget::dataChanged()
{
}