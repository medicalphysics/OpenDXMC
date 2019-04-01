#pragma once 


#include "volumerenderwidgetsettings.h"
#include "volumeactorcontainer.h"
#include "imagecontainer.h"

#include <QWidget>

#include <QVTKOpenGLWidget.h>
#include <vtkSmartPointer.h>
#include <vtkVolumeProperty.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkOpenGlRenderer.h>

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

	QVTKOpenGLWidget *m_openGLWidget;
	VolumeRenderSettingsWidget *m_settingsWidget;
	vtkSmartPointer<vtkSmartVolumeMapper> m_volumeMapper;
	vtkSmartPointer<vtkOpenGLRenderer> m_renderer;
	vtkSmartPointer<vtkVolume> m_volume;
	std::shared_ptr<ImageContainer> m_imageData;
	int m_renderMode = 0;

	std::vector<VolumeActorContainer*> m_volumeProps;
	std::shared_ptr<OrientationActorContainer> m_orientationProp;
};