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

Copyright 2019 Erlend Andersen
*/

#pragma once


#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkImageData.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkOpenGLRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkVolumeProperty.h>

#include <memory>

#include "opendxmc/imagecontainer.h"
#include "opendxmc/volumeactorcontainer.h"
#include "opendxmc/volumerenderwidgetsettings.h"


#ifndef Q_DECLARE_METATYPE_IMAGECONTAINER
#define Q_DECLARE_METATYPE_IMAGECONTAINER
Q_DECLARE_METATYPE(std::shared_ptr<ImageContainer>)
#endif

class VolumeRenderWidget : public QWidget {
    Q_OBJECT
public:
    VolumeRenderWidget(QWidget* parent = nullptr);
    ~VolumeRenderWidget();

    VolumeRenderSettingsWidget* getSettingsWidget(void) { return m_settingsWidget; }
    void updateRendering(void);
    void setImageData(std::shared_ptr<ImageContainer> volume);
    void setRenderMode(int mode);
    void setCropPlanes(int planes[6]);
    void addActorContainer(SourceActorContainer* actorContainer);
    void removeActorContainer(SourceActorContainer* actorContainer);
    void setActorsVisible(int visible);
signals:
    void imageDataChanged(std::shared_ptr<ImageContainer> image);

private:
    void updateVolumeRendering();
    void updateVolumeProps();

    QVTKOpenGLNativeWidget* m_openGLWidget;
    VolumeRenderSettingsWidget* m_settingsWidget;
    vtkSmartPointer<vtkSmartVolumeMapper> m_volumeMapper;
    vtkSmartPointer<vtkImageGaussianSmooth> m_imageSmoother;
    vtkSmartPointer<vtkOpenGLRenderer> m_renderer;
    vtkSmartPointer<vtkVolume> m_volume;

    std::shared_ptr<ImageContainer> m_imageData;
    int m_renderMode = 0;

    std::vector<VolumeActorContainer*> m_volumeProps;
    std::shared_ptr<OrientationActorContainer> m_orientationProp;
    bool m_actorsVisible = true;
};
