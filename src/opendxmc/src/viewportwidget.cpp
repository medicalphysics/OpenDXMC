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

#include "viewportwidget.h"
#include "colormap.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QToolBar>
#include <QPushButton>
#include <QVariant>
#include <QCheckBox>

#include <QVTKOpenGLWidget.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>


ViewPortWidget::ViewPortWidget(QWidget* parent)
	:QWidget(parent)
{
	QVBoxLayout* mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);

	auto toolBar = new QToolBar(tr("View settings"), this);
	mainLayout->addWidget(toolBar);

	m_volumeSelectorWidget = new QComboBox(toolBar);
	m_volumeSelectorWidget->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	connect(m_volumeSelectorWidget, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ViewPortWidget::showImageData);
	auto volumeSelectorLabel = new QLabel(tr("Select Volume: "), this);
	toolBar->addWidget(volumeSelectorLabel);
	toolBar->addWidget(m_volumeSelectorWidget);

	QSplitter* vSplitter = new QSplitter(Qt::Vertical);
	QSplitter* upperHSplitter = new QSplitter(Qt::Horizontal);
	QSplitter* lowerHSplitter = new QSplitter(Qt::Horizontal);
	vSplitter->addWidget(upperHSplitter);
	vSplitter->addWidget(lowerHSplitter);
	vSplitter->setContentsMargins(0, 0, 0, 0);
	mainLayout->addWidget(vSplitter);

	
	m_volumeRenderWidget = new VolumeRenderWidget(this);
	m_sliceRenderWidgetAxial = new SliceRenderWidget(this, SliceRenderWidget::Axial);
	m_sliceRenderWidgetCoronal = new SliceRenderWidget(this, SliceRenderWidget::Coronal);
	m_sliceRenderWidgetSagittal = new SliceRenderWidget(this, SliceRenderWidget::Sagittal);

	upperHSplitter->addWidget(m_sliceRenderWidgetAxial);
	upperHSplitter->addWidget(m_volumeRenderWidget);
	
	lowerHSplitter->addWidget(m_sliceRenderWidgetCoronal);
	lowerHSplitter->addWidget(m_sliceRenderWidgetSagittal);
	
	//adding volumerender settings to toolbar
	
	
	this->setLayout(mainLayout);

	
}

ViewPortWidget::~ViewPortWidget()
{

}

void ViewPortWidget::setImageData(std::shared_ptr<ImageContainer> volumeData)
{
	if (!volumeData)
		return;
	int indexKey = volumeData->imageType;
	if (volumeData->image) // valid pointer
	{
		//test for valid ID
		auto it = m_availableVolumes.begin();
		while (it != m_availableVolumes.end())
		{
			if (it.value()->ID == volumeData->ID)
				it++;
			else
				it = m_availableVolumes.erase(it); // deleting values where ID do not match current ID
		}		
		m_availableVolumes[indexKey] = volumeData;
	}
	else 
	{
		//test if key is in m_availableVolumes
		//if its there erase at index key since pointer is not valid
		auto it = m_availableVolumes.find(indexKey);
		if (it != m_availableVolumes.end())
			m_availableVolumes.erase(it);
	}
	updateVolumeSelectorWidget();
	showCurrentImageData();
}

void ViewPortWidget::addActorContainer(VolumeActorContainer* actorContainer)
{
	m_volumeRenderWidget->addActorContainer(actorContainer);
}

void ViewPortWidget::removeActorContainer(VolumeActorContainer * actorContainer)
{
	m_volumeRenderWidget->removeActorContainer(actorContainer);
}

void ViewPortWidget::showCurrentImageData()
{
	int currentIndex = m_volumeSelectorWidget->currentIndex();
	showImageData(currentIndex);
}

void ViewPortWidget::showImageData(int index)
{
	int imageDescription = -1;
	if ((index >= 0) & (index < m_volumeSelectorWidget->count())) 
		imageDescription = m_volumeSelectorWidget->itemData(index).toInt();
	
	if (m_availableVolumes.contains(imageDescription) || imageDescription==ImageContainer::CustomType)
	{
		std::shared_ptr<ImageContainer> volume, background;
		if (imageDescription == ImageContainer::CustomType)
		{
			volume = m_availableVolumes[static_cast<int>(ImageContainer::DoseImage)];
			background = m_availableVolumes[static_cast<int>(ImageContainer::CTImage)];
		}
		else
		{
			volume = m_availableVolumes[imageDescription];
			background = nullptr;
		}
		
		m_sliceRenderWidgetAxial->setImageData(volume, background);
		m_sliceRenderWidgetCoronal->setImageData(volume, background);
		m_sliceRenderWidgetSagittal->setImageData(volume, background);

		m_volumeRenderWidget->setImageData(volume);
		
		if ((imageDescription == static_cast<int>(ImageContainer::MaterialImage)) ||
			(imageDescription == static_cast<int>(ImageContainer::OrganImage)))
		{
			//generate color table if there is not to many different colors (7)
			if (volume->minMax[1] < 7.0)
			{
				QVector<double> colortable;
				for (int i = 0; i <= volume->minMax[1]; ++i)
				{
					auto color = getColor(i);
					for (int j = 0; j < 3; ++j)
						colortable.append(color[j]);
				}
				m_volumeRenderWidget->getSettingsWidget()->setColorTable(colortable);
			}
		}
	}
}

void ViewPortWidget::updateVolumeSelectorWidget()
{
	int currentIndex = m_volumeSelectorWidget->currentIndex();
	m_volumeSelectorWidget->blockSignals(true);
	m_volumeSelectorWidget->clear();
	auto it = m_availableVolumes.keyBegin();
	while (it != m_availableVolumes.keyEnd())
	{
		int indexKey = *it;
		m_volumeSelectorWidget->addItem(imageDescriptionName(indexKey), QVariant(indexKey));
		++it;
	}
	//adding dose overlay if possible
	if (m_availableVolumes.contains(ImageContainer::CTImage) && m_availableVolumes.contains(ImageContainer::DoseImage))
	{
		int indexKey = ImageContainer::CustomType;
		m_volumeSelectorWidget->addItem(imageDescriptionName(indexKey), QVariant(indexKey));
	}

	m_volumeSelectorWidget->blockSignals(false);
	if (m_volumeSelectorWidget->count() > 0)
	{
		if (currentIndex < 0)
			currentIndex = 0;
		else if (currentIndex > m_volumeSelectorWidget->count())
			currentIndex = 0;
		if (m_volumeSelectorWidget->currentIndex() == currentIndex)
			showImageData(currentIndex);
		else
			m_volumeSelectorWidget->setCurrentIndex(currentIndex);
	}
}

QString ViewPortWidget::imageDescriptionName(int imageDescription)
{

	if (imageDescription == static_cast<int>(ImageContainer::CTImage)) {
		return QString("CT data");
	}
	else if (imageDescription == static_cast<int>(ImageContainer::MaterialImage)) {
		return QString("Material data");
	}
	else if (imageDescription == static_cast<int>(ImageContainer::DensityImage)) {
		return QString("Density data");
	}
	else if (imageDescription == static_cast<int>(ImageContainer::OrganImage)) {
		return QString("Organ data");
	}
	else if (imageDescription == static_cast<int>(ImageContainer::DoseImage)) {
		return QString("Dose data");
	}
	else if (imageDescription == static_cast<int>(ImageContainer::CustomType)) {
		return QString("Dose overlay");
	}
	return QString();
}
