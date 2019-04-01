 
#pragma once

#include "volumerenderwidget.h"
#include "slicerenderwidget.h"
#include "imageimportpipeline.h"
#include "imagecontainer.h"
#include "volumeactorcontainer.h"

#include <QWidget>
#include <QSize>
#include <QMap>
#include <memory>


//lage viewport og viewport object factory (til å generere grafikk items)
class ViewPortWidget : public QWidget
{
	Q_OBJECT
public:
    ViewPortWidget(QWidget* parent = nullptr);
	~ViewPortWidget();

	void setImageData(std::shared_ptr<ImageContainer> imageData);
	void addActorContainer(VolumeActorContainer* actorContainer);
	void removeActorContainer(VolumeActorContainer* actorContainer);
	void render() { m_volumeRenderWidget->updateRendering(); }
	void showCurrentImageData(void);
	void showImageData(int imageDescription);
	QSize minimumSizeHint(void) const { return QSize(200, 200); }


private:
	VolumeRenderWidget *m_volumeRenderWidget;
	SliceRenderWidget *m_sliceRenderWidgetAxial;
	SliceRenderWidget *m_sliceRenderWidgetCoronal;
	SliceRenderWidget *m_sliceRenderWidgetSagittal;
	
	QComboBox* m_volumeSelectorWidget;
	QMap<int, std::shared_ptr<ImageContainer>> m_availableVolumes;

	void updateVolumeSelectorWidget();
	QString imageDescriptionName(int imageDescription);
	
};
