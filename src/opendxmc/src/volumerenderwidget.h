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


#include "volumerenderwidgetsettings.h"
#include "volumeactorcontainer.h"
#include "imagecontainer.h"

#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkSmartPointer.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkOpenGLRenderer.h>
#include <vtkImageGaussianSmooth.h>

#include <memory>

class VolumeRenderWidget : public QWidget
{
	Q_OBJECT
public:
	VolumeRenderWidget(QWidget *parent = nullptr);
	~VolumeRenderWidget();
	
	VolumeRenderSettingsWidget *getSettingsWidget(void) { return m_settingsWidget; }
	void updateRendering(void);
	void setImageData(std::shared_ptr<ImageContainer> volume);
	void setRenderMode(int mode);
	void setCropPlanes(int planes[6]);
	void addActorContainer(VolumeActorContainer* actorContainer);
	void removeActorContainer(VolumeActorContainer* actorContainer);
	void setActorsVisible(int visible);
signals:
	void imageDataChanged(std::shared_ptr<ImageContainer> image);
private:
	void updateVolumeRendering();
	void updateVolumeProps();

	QVTKOpenGLNativeWidget *m_openGLWidget;
	VolumeRenderSettingsWidget *m_settingsWidget;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volumeMapper;
	vtkSmartPointer<vtkImageGaussianSmooth> m_imageSmoother;
	vtkSmartPointer<vtkOpenGLRenderer> m_renderer;
	vtkSmartPointer<vtkVolume> m_volume;

	std::shared_ptr<ImageContainer> m_imageData;
	int m_renderMode = 0;

	std::vector<VolumeActorContainer*> m_volumeProps;
	std::shared_ptr<OrientationActorContainer> m_orientationProp;
};