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
#include <QSpinBox>
#include <QVBoxLayout>

#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

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
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);

    auto blend_mode = getSettingsWidget<QComboBox>(tr("Set blend mode"), this);
    blend_mode.widget->addItem(tr("Composite"));
    blend_mode.widget->addItem(tr("MaxIP"));
    blend_mode.widget->addItem(tr("Additive"));
    blend_mode.widget->addItem(tr("Isosurface"));
    blend_mode.widget->setCurrentIndex(0);
    connect(blend_mode.widget, &QComboBox::currentIndexChanged, this, [this](int val) {
        switch (val) {
        case 0:
            m_settings->mapper()->SetBlendModeToComposite();
            break;
        case 1:
            m_settings->mapper()->SetBlendModeToMaximumIntensity();
            break;
        case 2:
            m_settings->mapper()->SetBlendModeToAdditive();
            break;
        case 3:
            m_settings->mapper()->SetBlendModeToIsoSurface();
            break;
        default:
            break;
        }
        m_settings->render();
    });
    layout->addLayout(blend_mode.layout);

    // interpolation
    auto inter_type = getSettingsWidget<QComboBox>(tr("Set interpolation"), this);
    inter_type.widget->addItem(tr("Nearest"));
    inter_type.widget->addItem(tr("Linear"));
    inter_type.widget->addItem(tr("Cubic"));
    inter_type.widget->setCurrentIndex(1);
    connect(inter_type.widget, &QComboBox::currentIndexChanged, this, [this](int val) {
        m_settings->volumeProperty()->SetInterpolationType(val);
        m_settings->render();
    });
    layout->addLayout(inter_type.layout);

    // jittering
    auto jittering = getSettingsWidget<QCheckBox>(tr("Use jittering"), this);
    jittering.widget->setChecked(m_settings->mapper()->GetUseJittering());
    connect(jittering.widget, &QCheckBox::stateChanged, [this](int state) {
        m_settings->mapper()->SetUseJittering(state != 0);
        m_settings->render();
    });
    layout->addLayout(jittering.layout);

    /* NOT used (no effect?)
    // Multi samples
    auto multisampling = getSettingsWidget<QSpinBox>(tr("Multi sampling"), this);
    multisampling.widget->setValue(m_settings->renderWindow()->GetMultiSamples());
    multisampling.widget->setMinimum(0);
    connect(multisampling.widget, &QSpinBox::valueChanged, [this](int value) {
        m_settings->renderWindow()->SetMultiSamples(value);
        m_settings->render();
    });
    layout->addLayout(multisampling.layout);
    */

    auto vprop = m_settings->volumeProperty();
    // shadebox
    auto shadebox = new QGroupBox(tr("Shading"), this);
    auto shade_layout = new QVBoxLayout;
    shade_layout->setContentsMargins(0, 0, 0, 0);
    shadebox->setLayout(shade_layout);
    shadebox->setCheckable(true);
    shadebox->setChecked(vprop->GetShade() == 1);
    connect(shadebox, &QGroupBox::toggled, [vprop, this](bool toggle) {
        vprop->SetShade(toggle ? 1 : 0);
        m_settings->render();
    });
    layout->addWidget(shadebox);

    // two sided lightning
    auto tsl = getSettingsWidget<QCheckBox>(tr("Two sided lightning"), this);
    tsl.widget->setChecked(m_settings->renderer()->GetTwoSidedLighting());
    connect(tsl.widget, &QCheckBox::stateChanged, [this](int state) {
        m_settings->renderer()->SetTwoSidedLighting(state != 0);
        m_settings->render();
    });
    shade_layout->addLayout(tsl.layout);

    // Global illumination reach
    auto gir = getSettingsWidget<QSlider>(tr("Global illumination reach"), shadebox);
    gir.widget->setValue(static_cast<int>(m_settings->mapper()->GetGlobalIlluminationReach() * 100));
    connect(gir.widget, &QSlider::valueChanged, [this](int value) {
        const float g = value / float { 100 };
        m_settings->mapper()->SetGlobalIlluminationReach(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(gir.layout);

    // Volumetric Scattering blending
    auto vsb = getSettingsWidget<QSlider>(tr("Volumetric scattering blending"), this);
    vsb.widget->setValue(static_cast<int>(m_settings->mapper()->GetGlobalIlluminationReach() * 50));
    connect(vsb.widget, &QSlider::valueChanged, [this](int value) {
        const float g = value / float { 50 };
        m_settings->mapper()->SetVolumetricScatteringBlending(std::clamp(g, 0.f, 2.f));
        m_settings->render();
    });
    shade_layout->addLayout(vsb.layout);

    // Ambient
    auto ambient = getSettingsWidget<QSlider>(tr("Ambient"), this);
    ambient.widget->setValue(static_cast<int>(vprop->GetAmbient() * 100));
    connect(ambient.widget, &QSlider::valueChanged, [vprop, this](int value) {
        const float g = value / float { 100 };
        vprop->SetAmbient(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(ambient.layout);

    // Diffuse
    auto diffuse = getSettingsWidget<QSlider>(tr("Diffuse"), this);
    diffuse.widget->setValue(static_cast<int>(vprop->GetDiffuse() * 100));
    connect(diffuse.widget, &QSlider::valueChanged, [vprop, this](int value) {
        const float g = value / float { 100 };
        vprop->SetDiffuse(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(diffuse.layout);

    // Specular
    auto specular = getSettingsWidget<QSlider>(tr("Specular"), this);
    specular.widget->setValue(static_cast<int>(vprop->GetSpecular() * 100));
    connect(specular.widget, &QSlider::valueChanged, [vprop, this](int value) {
        const float g = value / float { 100 };
        vprop->SetSpecular(std::clamp(g, 0.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(specular.layout);

    // Specular power
    auto specularpower = getSettingsWidget<QSlider>(tr("Specular power"), this);
    specularpower.widget->setValue(static_cast<int>(vprop->GetSpecularPower()));
    connect(specularpower.widget, &QSlider::valueChanged, [vprop, this](int value) {
        const float g = value;
        vprop->SetSpecularPower(g);
        m_settings->render();
    });
    shade_layout->addLayout(specularpower.layout);

    // Scattering anisotropy
    auto sa = getSettingsWidget<QSlider>(tr("Scattering anisotropy"), this);
    sa.widget->setValue(static_cast<int>(vprop->GetScatteringAnisotropy() * 50) + 50);
    connect(sa.widget, &QSlider::valueChanged, [vprop, this](int value) {
        const float g = value / float { 50 } - 1;
        vprop->SetScatteringAnisotropy(std::clamp(g, -1.f, 1.f));
        m_settings->render();
    });
    shade_layout->addLayout(sa.layout);

    // computeNormalFromOpacity
    auto computeNormalFromOpacity = getSettingsWidget<QCheckBox>(tr("Compute normals from opacity"), this);
    computeNormalFromOpacity.widget->setChecked(m_settings->mapper()->GetComputeNormalFromOpacity());
    connect(computeNormalFromOpacity.widget, &QCheckBox::stateChanged, [this](int state) {
        m_settings->mapper()->SetComputeNormalFromOpacity(state != 0);
        m_settings->render();
    });
    shade_layout->addLayout(computeNormalFromOpacity.layout);

    // colortable selector
    auto color = getSettingsWidget<QComboBox>(tr("Color table"), this);
    for (const auto& name : Colormaps::availableColormaps()) {
        color.widget->addItem(QString::fromStdString(name));
    }
    auto color_gray_index = color.widget->findText(QString::fromStdString("CT"));
    color.widget->setCurrentIndex(color_gray_index);
    connect(color.widget, &QComboBox::currentTextChanged, [this](const QString& cname) {
        m_settings->setColorMap(cname.toStdString());
    });

    layout->addLayout(color.layout);

    // option for power opacity transfer;
    auto uselog10 = new QCheckBox(tr("Power opacity"), this);
    uselog10->setChecked(false);
    connect(uselog10, &QCheckBox::stateChanged, [this](int state) { m_settings->setUsePowerOpacityLUT(state != 0); });
    //  option for color crop;
    auto colorcrop = new QCheckBox(tr("Crop colors to opacity"), this);
    colorcrop->setChecked(true);
    connect(colorcrop, &QCheckBox::stateChanged, [this](int state) { m_settings->setCropColorToOpacityRange(state != 0); });
    auto color_opt_layout = new QHBoxLayout;
    color_opt_layout->setContentsMargins(0, 0, 0, 0);
    color_opt_layout->addWidget(colorcrop);
    color_opt_layout->addWidget(uselog10);
    layout->addLayout(color_opt_layout);

    // lutWidget opacity
    m_lut_opacity_widget = new VolumeLUTWidget(m_settings, VolumeLUTWidget::LUTType::Opacity, this);
    layout->addWidget(m_lut_opacity_widget);

    // lutWidget gradient
    m_lut_gradient_widget = new VolumeLUTWidget(m_settings, VolumeLUTWidget::LUTType::Gradient, this);
    layout->addWidget(m_lut_gradient_widget);

    setLayout(layout);
}
