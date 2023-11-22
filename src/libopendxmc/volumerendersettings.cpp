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

#include <volumerendersettings.hpp>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>

#include <colormaps.hpp>

VolumeRenderSettings::VolumeRenderSettings(
    vtkSmartPointer<vtkRenderer> renderer,
    vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper> mapper,
    vtkSmartPointer<vtkVolume> volume,
    vtkSmartPointer<vtkDiscretizableColorTransferFunction> colorlut,
    QObject* parent)
    : m_renderer(renderer)
    , m_mapper(mapper)
    , m_volume(volume)
    , m_color_lut(colorlut)
    , QObject(parent)
{

    // Follow new light settings
    m_renderer->SetLightFollowCamera(true);
    m_renderer->GetRenderWindow()->GetInteractor()->SetLightFollowCamera(false);

    auto olut = opacityLut();
    olut->SetClamping(true);
    m_opacityDataNormalizedRange.resize(4);
    m_opacityDataNormalizedRange[0] = { 0.0, 0 };
    m_opacityDataNormalizedRange[1] = { 0.25, 0 };
    m_opacityDataNormalizedRange[2] = { 0.33, 0.8 };
    m_opacityDataNormalizedRange[3] = { 0.66, 0.8 };
    m_gradientDataNormalizedRange.resize(2);
    m_gradientDataNormalizedRange[0] = { 0, 1 };
    m_gradientDataNormalizedRange[1] = { 1, 1 };

    updateGradientLutFromNormalizedRange(false);
    updateOpacityLutFromNormalizedRange(false);
    setColorMap("CT", false);
}

vtkVolumeProperty* VolumeRenderSettings::volumeProperty()
{
    return m_volume ? m_volume->GetProperty() : nullptr;
}

vtkPiecewiseFunction* VolumeRenderSettings::opacityLut()
{
    auto prop = volumeProperty();
    return prop ? prop->GetScalarOpacity() : nullptr;
}

vtkPiecewiseFunction* VolumeRenderSettings::gradientLut()
{
    auto prop = volumeProperty();
    return prop ? prop->GetGradientOpacity() : nullptr;
}

void VolumeRenderSettings::setCurrentImageData(vtkSmartPointer<vtkImageData> data, bool resetCamera)
{
    m_currentImageData = data;
    if (m_currentImageData) {
        m_mapper->SetInputData(m_currentImageData);
        m_currentImageData->GetScalarRange(m_currentImageDataScalarRange.data());
        updateColorLutFromNormalizedRange(false);
        updateGradientLutFromNormalizedRange(false);
        updateOpacityLutFromNormalizedRange(false);
        m_volume->Update();
        if (resetCamera)
            m_renderer->ResetCamera();
        render();
        emit imageDataChanged();
    }
}

vtkImageData* VolumeRenderSettings::currentImageData()
{
    return m_currentImageData.Get();
}

void VolumeRenderSettings::setColorMap(const std::string& name, bool render)
{
    if (!Colormaps::haveColormap(name)) {
        return;
    }
    const auto& map = Colormaps::colormap(name);
    const auto N = map.size();
    m_colorDataNormalizedRange.clear();
    m_colorDataNormalizedRange.reserve(N / 3);
    for (std::size_t i = 0; i < N; i += 3) {
        const auto x = static_cast<double>(i) / (N - 3);
        std::array<double, 4> buf = {
            x,
            map[i],
            map[i + 1],
            map[i + 2]
        };
        m_colorDataNormalizedRange.push_back(std::move(buf));
    }
    updateColorLutFromNormalizedRange(render);
}

bool VolumeRenderSettings::valid()
{
    return m_renderer && m_mapper && m_color_lut && m_volume && volumeProperty();
}

void VolumeRenderSettings::render()
{
    auto renwin = m_renderer ? m_renderer->GetRenderWindow() : nullptr;
    if (renwin && m_currentImageData)
        renwin->Render();
}

double shiftscale(double xnorm, double xmin, double xmax)
{
    return xmin + xnorm * (xmax - xmin);
}

double shiftscale(double xnorm, const std::array<double, 2>& minmax)
{
    return minmax[0] + xnorm * (minmax[1] - minmax[0]);
}

std::vector<std::array<double, 4>> VolumeRenderSettings::colorDataNormalizedCroppedToOpacity() const
{
    auto data = m_colorDataNormalizedRange;
    const auto& range_tot = m_currentImageDataScalarRange;
    const auto relmin = m_opacityDataNormalizedRange.front()[0];
    const auto relmax = m_opacityDataNormalizedRange.back()[0];

    const std::array<double, 2> range = { relmin, relmax };
    for (auto& d : data) {
        d[0] = shiftscale(d[0], range);
    }
    return data;
}

void VolumeRenderSettings::setOpacityDataNormalized(const std::vector<std::array<double, 2>>& data)
{
    if (data.size() < 2)
        return;
    m_opacityDataNormalizedRange = data;
    updateOpacityLutFromNormalizedRange();
    if (m_cropColorToOpacityRange)
        updateColorLutFromNormalizedRange();
}

void VolumeRenderSettings::setGradientDataNormalized(const std::vector<std::array<double, 2>>& data)
{
    if (data.size() < 2)
        return;
    m_gradientDataNormalizedRange = data;
    updateGradientLutFromNormalizedRange();
}

void VolumeRenderSettings::setColorDataNormalized(const std::vector<std::array<double, 4>>& data)
{
    if (data.size() < 2)
        return;
    m_colorDataNormalizedRange = data;
    updateColorLutFromNormalizedRange();
}

void VolumeRenderSettings::setUsePowerOpacityLUT(bool on)
{
    m_use_opacity_power_lut = on;
    updateOpacityLutFromNormalizedRange(true);
}

void VolumeRenderSettings::setCropColorToOpacityRange(bool on)
{
    m_cropColorToOpacityRange = on;
    updateColorLutFromNormalizedRange(true);
    emit colorLutChanged();
}

bool VolumeRenderSettings::getCropColorToOpacityRange()
{
    return m_cropColorToOpacityRange;
}

void VolumeRenderSettings::updateOpacityLutFromNormalizedRange(bool rnd)
{
    auto lut = opacityLut();

    const auto& range = m_currentImageDataScalarRange;
    lut->RemoveAllPoints();
    for (const auto& [xnorm, y] : m_opacityDataNormalizedRange) {
        const auto x = shiftscale(xnorm, range);
        if (m_use_opacity_power_lut) {
            lut->AddPoint(x, y * y * y);
        } else {
            lut->AddPoint(x, y);
        }
    }
    if (rnd) {
        m_volume->Update();
        render();
    }
    emit opacityLutChanged();
}

void VolumeRenderSettings::updateGradientLutFromNormalizedRange(bool rnd)
{
    auto lut = gradientLut();

    const auto& imagerange = m_currentImageDataScalarRange;
    constexpr double min = 0;
    const double max = (imagerange[1] - imagerange[0]) * 0.1;

    lut->RemoveAllPoints();
    for (const auto& [xnorm, y] : m_gradientDataNormalizedRange) {
        const auto x = shiftscale(xnorm, min, max);
        lut->AddPoint(x, y);
    }
    if (rnd) {
        m_volume->Update();
        render();
    }
    emit gradientLutChanged();
}

void VolumeRenderSettings::updateColorLutFromNormalizedRange(bool rnd)
{
    auto lut = colorLut();

    if (m_cropColorToOpacityRange) {
        const auto& range_tot = m_currentImageDataScalarRange;
        const auto relmin = m_opacityDataNormalizedRange.front()[0];
        const auto relmax = m_opacityDataNormalizedRange.back()[0];
        const std::array<double, 2> range = {
            range_tot[0] + relmin * (range_tot[1] - range_tot[0]),
            range_tot[0] + relmax * (range_tot[1] - range_tot[0]),
        };

        lut->RemoveAllPoints();
        for (const auto& c : m_colorDataNormalizedRange) {
            const auto x = shiftscale(c[0], range);
            lut->AddRGBPoint(x, c[1], c[2], c[3]);
        }
    } else {
        const auto& range = m_currentImageDataScalarRange;
        lut->RemoveAllPoints();
        for (const auto& c : m_colorDataNormalizedRange) {
            const auto x = shiftscale(c[0], range);
            lut->AddRGBPoint(x, c[1], c[2], c[3]);
        }
    }

    if (rnd) {
        m_volume->Update();
        render();
    }
    emit colorLutChanged();
}
