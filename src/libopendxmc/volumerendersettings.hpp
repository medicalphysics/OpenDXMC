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
#pragma once

#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkVolume.h>

#include <QObject>

#include <array>
#include <string>
#include <vector>

class vtkPiecewiseFunction;
class vtkVolumeProperty;

class VolumeRenderSettings : public QObject {
    Q_OBJECT
public:
    VolumeRenderSettings(vtkSmartPointer<vtkRenderer> renderer,
        vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper> mapper,
        vtkSmartPointer<vtkVolume> volume,
        vtkSmartPointer<vtkDiscretizableColorTransferFunction> colorlut,
        QObject* parent = nullptr);

    vtkRenderer* renderer() { return m_renderer.Get(); }
    vtkOpenGLGPUVolumeRayCastMapper* mapper() { return m_mapper.Get(); }
    vtkVolume* volume() { return m_volume.Get(); }
    vtkVolumeProperty* volumeProperty();
    vtkRenderWindow* renderWindow() { return m_renderer ? m_renderer->GetRenderWindow() : nullptr; }

    vtkDiscretizableColorTransferFunction* colorLut() { return m_color_lut.Get(); }
    vtkPiecewiseFunction* opacityLut();
    vtkPiecewiseFunction* gradientLut();

    void setCurrentImageData(vtkSmartPointer<vtkImageData> data, bool resetCamera = false);
    vtkImageData* currentImageData();
    const std::array<double, 2>& currentImageDataScalarRange() const { return m_currentImageDataScalarRange; }
    void setColorMap(const std::string& name, bool render = true);

    void render();
    bool valid();

    const std::vector<std::array<double, 2>>& opacityDataNormalized() const { return m_opacityDataNormalizedRange; }
    const std::vector<std::array<double, 2>>& gradientDataNormalized() const { return m_gradientDataNormalizedRange; }
    const std::vector<std::array<double, 4>>& colorDataNormalized() const { return m_colorDataNormalizedRange; }
    std::vector<std::array<double, 4>> colorDataNormalizedCroppedToOpacity() const;
    void setOpacityDataNormalized(const std::vector<std::array<double, 2>>&);
    void setGradientDataNormalized(const std::vector<std::array<double, 2>>&);
    void setColorDataNormalized(const std::vector<std::array<double, 4>>&);
    void setUsePowerOpacityLUT(bool on);
    void setCropColorToOpacityRange(bool);
    bool getCropColorToOpacityRange();

signals:
    void imageDataChanged();
    void opacityLutChanged();
    void gradientLutChanged();
    void colorLutChanged();

protected:
    void updateOpacityLutFromNormalizedRange(bool render = true);
    void updateGradientLutFromNormalizedRange(bool render = true);
    void updateColorLutFromNormalizedRange(bool render = true);

private:
    vtkSmartPointer<vtkRenderer> m_renderer = nullptr;
    vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper> m_mapper = nullptr;
    vtkSmartPointer<vtkDiscretizableColorTransferFunction> m_color_lut = nullptr;
    vtkSmartPointer<vtkVolume> m_volume = nullptr;
    vtkSmartPointer<vtkImageData> m_currentImageData = nullptr;
    std::array<double, 2> m_currentImageDataScalarRange = { -1, 1 };

    std::vector<std::array<double, 2>> m_opacityDataNormalizedRange;
    std::vector<std::array<double, 2>> m_gradientDataNormalizedRange;
    std::vector<std::array<double, 4>> m_colorDataNormalizedRange;
    bool m_cropColorToOpacityRange = true;
    bool m_use_opacity_power_lut = false;
};