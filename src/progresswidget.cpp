
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

#include "opendxmc/progresswidget.h"
#include "opendxmc/colormap.h"
#include <QVBoxLayout>
#include <QCheckBox>
#include <QSettings>
#include <QImage>
#include <QGraphicsScene>
#include <QTransform>
#include <QPixmap>
#include <QColor>
#include <QBrush>
#include <QTransform>


ProgressWidget::ProgressWidget(QWidget* parent) 
	: QWidget(parent)
{
	auto mainLayout = new QVBoxLayout(this);
	auto hLayout = new QHBoxLayout(this);
	auto setVisibleWidget = new QCheckBox(tr("Show simulation progress"), this);
	hLayout->addWidget(setVisibleWidget);
	hLayout->addStretch();

	m_cancelButton = new QPushButton("Cancel simulation", this);
	connect(m_cancelButton, &QPushButton::clicked, [=]() {this->setCancelRun(true); });
	connect(m_cancelButton, &QPushButton::clicked, [=]() {m_cancelButton->setDisabled(m_cancelProgress); });
	hLayout->addWidget(m_cancelButton);

	mainLayout->addLayout(hLayout);

	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
	m_showProgress = settings.value("simulationprogress/show").value<bool>();
	setVisibleWidget->setChecked(m_showProgress);

	connect(setVisibleWidget, &QCheckBox::stateChanged, this, &ProgressWidget::setShowProgress);

	m_view = new QGraphicsView(this);
	auto scene = new QGraphicsScene(m_view);
	m_view->setScene(scene);
	mainLayout->addWidget(m_view);
	m_pixItem = new QGraphicsPixmapItem();
	scene->addItem(m_pixItem);

	m_colormap = generateStandardQTColorTable(HOT_IRON);
	QColor background(m_colormap[0]);
	m_view->setBackgroundBrush(QBrush(background));
	setLayout(mainLayout);
	hide(); // we start hiding
}

void ProgressWidget::setImageData(std::shared_ptr<DoseProgressImageData> data)
{
	if (data) {
		
		const int width = static_cast<int>(data->dimensions[0]);
		const int height = static_cast<int>(data->dimensions[1]);
		
		const double dw = data->spacing[0];
		const double dh = data->spacing[1];

		QImage qim(data->image.data(), width, height, width, QImage::Format_Indexed8);
		qim.setColorTable(m_colormap);
		QTransform transform(dw, .0, .0, -dh, .0, .0); //scale and mirror y axis
		if (height > width)
		{
			QTransform rot;
			rot.rotate(90.0); // flipping image
			transform = transform * rot;
		}
		
		m_pixItem->setPixmap(QPixmap::fromImage(qim));
		m_pixItem->setTransform(transform);
		const auto rect = m_pixItem->sceneBoundingRect();
		m_view->scene()->setSceneRect(rect);
		m_view->setVisible(m_showProgress);
		m_view->fitInView(rect, Qt::KeepAspectRatio);
	}
}

void ProgressWidget::setShowProgress(bool status)
{
	m_showProgress = status;
	m_view->setVisible(m_showProgress);
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
	settings.setValue("simulationprogress/show", m_showProgress);
}

void ProgressWidget::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);
	m_view->fitInView(m_pixItem, Qt::KeepAspectRatio);
}


void ProgressWidget::showEvent(QShowEvent* event)
{
	QWidget::showEvent(event);
	m_view->setVisible(m_showProgress);
	m_cancelButton->setEnabled(true);
	m_cancelProgress = false;
}