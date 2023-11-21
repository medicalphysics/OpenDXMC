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
#include <volumerendersettings.hpp>

#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkVolume.h>

#include <memory>

class VolumerenderSettingsWidget;
class BeamActorContainer;

class VolumerenderWidget : public QWidget {
    Q_OBJECT
public:
    VolumerenderWidget(QWidget* parent = nullptr);
    void updateImageData(std::shared_ptr<DataContainer>);
    VolumerenderSettingsWidget* createSettingsWidget(QWidget* parent = nullptr);
    void showData(DataContainer::ImageType type);
    void addActor(std::shared_ptr<BeamActorContainer> actor);
    void removeActor(std::shared_ptr<BeamActorContainer> actor);
    void Render();

protected:
    void setupRenderingPipeline();
    void setNewImageData(vtkSmartPointer<vtkImageData> data, bool rezoom_camera = false);

private:
    std::shared_ptr<DataContainer> m_data = nullptr;
    QVTKOpenGLNativeWidget* openGLWidget = nullptr;
    VolumeRenderSettings* m_settings = nullptr;
};