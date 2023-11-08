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
#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkOpenGLRenderer.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindow.h>
#include <vtkSmartPointer.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>

struct VolumeRenderSettings {
    vtkSmartPointer<vtkOpenGLRenderer> renderer = nullptr;
    vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper> mapper = nullptr;
    vtkSmartPointer<vtkDiscretizableColorTransferFunction> color_lut = nullptr;
    vtkSmartPointer<vtkVolume> volume = nullptr;

    vtkVolumeProperty* getVolumeProperty() { return volume ? volume->GetProperty() : nullptr; }

    vtkPiecewiseFunction* getOpacityLut()
    {
        auto prop = getVolumeProperty();
        return prop ? prop->GetScalarOpacity() : nullptr;
    }

    vtkPiecewiseFunction* getGradientLut()
    {
        auto prop = getVolumeProperty();
        return prop ? prop->GetGradientOpacity() : nullptr;
    }

    bool valid()
    {
        return renderer && mapper && color_lut && volume && getVolumeProperty();
    }

    void render()
    {
        auto renwin = renderer ? renderer->GetRenderWindow() : nullptr;
        if (renwin)
            renwin->Render();
    }
};