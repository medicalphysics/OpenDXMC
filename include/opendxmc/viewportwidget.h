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

#include "volumerenderwidget.h"
#include "slicerenderwidget.h"
#include "imageimportpipeline.h"
#include "imagecontainer.h"
#include "volumeactorcontainer.h"

#include <QWidget>
#include <QComboBox>
#include <QSize>
#include <QMap>
#include <QString>
#include <memory>


//lage viewport og viewport object factory (til å generere grafikk items)
class ViewPortWidget : public QWidget
{
	Q_OBJECT
public:
    ViewPortWidget(QWidget* parent = nullptr);
	~ViewPortWidget();

	void setImageData(std::shared_ptr<ImageContainer> imageData);
	void addActorContainer(SourceActorContainer* actorContainer);
	void removeActorContainer(SourceActorContainer* actorContainer);
	void render();
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
	QString imageDescriptionToolTip(int imageDescription);
};
