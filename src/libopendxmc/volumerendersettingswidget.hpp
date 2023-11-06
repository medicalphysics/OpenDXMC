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

#include <datacontainer.hpp>

#include <QWidget>

#include <vtkOpenGLGPUVolumeRayCastMapper.h>
#include <vtkVolumeProperty.h>

class VolumerenderSettingsWidget : public QWidget {
    Q_OBJECT
public:
    VolumerenderSettingsWidget(vtkOpenGLGPUVolumeRayCastMapper*, vtkVolumeProperty*, QWidget* parent = nullptr);

    void setMapper(vtkOpenGLGPUVolumeRayCastMapper* mapper);
    void setVolumeProperty(vtkVolumeProperty* prop);
    void dataChanged();

signals:
    void renderSettingsChanged();

private:
    vtkVolumeProperty* m_property = nullptr; // this is the volumeproperty
    vtkOpenGLGPUVolumeRayCastMapper* m_mapper = nullptr;
};
