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
#include "opendxmc/volumerenderwidgetsettings.h"
#include "opendxmc/colormap.h"

using namespace QtCharts;

OpacitySeries::OpacitySeries(QObject *parent)
	:QScatterSeries(parent)
{
	connect(this, &OpacitySeries::doubleClicked, this, static_cast<void(OpacitySeries::*)(const QPointF&)>(&OpacitySeries::remove));
	connect(this, &OpacitySeries::pressed, this, &OpacitySeries::setPointIsPressed);
	connect(this, &OpacitySeries::released, this, &OpacitySeries::setPointIsReleased);
}

void OpacitySeries::setPointIsPressed(const QPointF & point)
{
	auto points = this->pointsVector();
	if (points.count() > 0)
		m_pointIsPressedIndex = points.indexOf(point);
	else
		m_pointIsPressedIndex = -1;
}

void OpacitySeries::setPointIsReleased(const QPointF & point)
{
	m_pointIsPressedIndex = -1;
}

OpacityChart::OpacityChart()
{
	m_series = new OpacitySeries(this);
	this->addSeries(m_series);
	this->createDefaultAxes();
	this->setAnimationOptions(QChart::NoAnimation);
	this->legend()->hide();
	auto axesX = axes(Qt::Horizontal);
	auto axesY = axes(Qt::Vertical);
	if (axesX.size() > 0)
	{
		auto axisX = axesX[0];
		axisX->setTitleText(tr("Image intensity"));
		axisX->setMinorGridLineVisible(false);
		axisX->setGridLineVisible(false);
	}
	if (axesY.size() > 0)
	{
		auto axisY = axesY[0];
		axisY->setLabelsVisible(false);
		axisY->setTitleVisible(false);
		axisY->setMinorGridLineVisible(false);
		axisY->setGridLineVisible(false);
	}

	this->setContentsMargins(0, 0, 0, 0);
	this->layout()->setContentsMargins(0, 0, 0, 0);
}


OpacityChartView::OpacityChartView(QWidget *parent, vtkPiecewiseFunction * opacityFunction, OpacityChartView::Color color)
	: QChartView(parent), m_opacityFunction(opacityFunction)
{
	m_color = color;
	m_chart = new OpacityChart();
	this->setChart(m_chart);
	setMouseTracking(true);
	connect(m_chart->getOpacitySeries(), &OpacitySeries::pointRemoved, [=](int dummyIndex) {this->updateOpacityFunction(); });
	connect(m_chart->getOpacitySeries(), &OpacitySeries::pointAdded, [=](int dummyIndex) {this->updateOpacityFunction(); });
	this->setImageDataRange(-1.0, 1.0);

	QVector<QPointF> dummyPoints(1, QPointF(0.0, 0.4));
	this->setPoints(dummyPoints);
	this->setContentsMargins(0, 0, 0, 0);

	this->setRenderHint(QPainter::Antialiasing);
	this->setContentsMargins(0, 0, 0, 0);
}
void OpacityChartView::setPoints(const QVector<QPointF> &points)
{
	if (points.count() > 0)
	{
		OpacitySeries *series = m_chart->getOpacitySeries();
		series->clear();
		for (const QPointF &point : points)
			series->append(point);
		this->updateOpacityFunction();
	}
}

void OpacityChartView::setImageDataRange(double min, double max)
{
	// testing if min > max
	double cmin = min < max ? min : max;
	double cmax = min < max ? max : min;
	if (m_color == Gradient)
	{
		cmax = (cmax - cmin)*0.1;
		cmin = 0;
	}

	this->redistributePointsToRange(cmin, cmax); // we want to redistribute plot points

	m_xrange[0] = cmin;
	m_xrange[1] = cmax;
	auto axesX = m_chart->axes(Qt::Horizontal);
	if (axesX.size() > 0)
	{
		axesX[0]->setMin(cmin);
		axesX[0]->setMax(cmax);
	}
	//delete any points outside range
	int teller = 0;
	auto series = m_chart->getOpacitySeries();
	while (teller < series->count())
	{
		const auto &p = series->at(teller);
		if (p.x() < cmin)
			series->remove(teller);
		else if (p.x() > cmax)
			series->remove(teller);
		else
			++teller;
	}
	
}

void OpacityChartView::updateOpacityFunction()
{
	if (m_opacityFunction)
	{
		auto points = m_chart->getOpacitySeries()->pointsVector();
		m_opacityFunction->RemoveAllPoints();
		for (const QPointF &point : points)
		{
			double yValue = point.y();
			if ((m_color != None) & (m_color != Gradient)) // for opacity and gradient we want to make the editing easier by squaring the lookup value
				yValue = yValue * yValue;
			m_opacityFunction->AddPoint(point.x(), yValue);
		}
		emit opacityFunctionChanged();
	}
}

void OpacityChartView::setColorTable(const QVector<double>& colorTable)
{
	if (m_color != None)
	{
		int idx = 0;  // red color
		double step = (m_xrange[1] - m_xrange[0]) / (static_cast<double>(colorTable.count()) - 3.0);
		if (m_color == Green)
			idx = 1;
		else if (m_color == Blue)
			idx = 2;
		auto series = m_chart->getOpacitySeries();
		series->clear();
		for (int i = idx; i < colorTable.count(); i += 3)
			series->append(step * (static_cast<double>(i) - idx) + m_xrange[0], colorTable[i]);
		updateOpacityFunction();
	}
}


void OpacityChartView::mousePressEvent(QMouseEvent * event)
{
	QChartView::mousePressEvent(event);
	m_opacityPointIndexIsMoving = m_chart->getOpacitySeries()->getPointIsPressedIndex();
}

void OpacityChartView::mouseReleaseEvent(QMouseEvent * event)
{
	QChartView::mouseReleaseEvent(event);
	if (m_opacityPointIndexIsMoving > -1)
	{
		this->updateOpacityFunction();
	}
	m_opacityPointIndexIsMoving = m_chart->getOpacitySeries()->getPointIsPressedIndex();
	
}

void OpacityChartView::mouseMoveEvent(QMouseEvent * event)
{
	QChartView::mouseMoveEvent(event);
	if ((m_opacityPointIndexIsMoving > -1) & (event->buttons() == Qt::LeftButton))
	{
		QPointF pos_scene = this->mapToScene(event->pos());
		QPointF pos = m_chart->mapToValue(pos_scene, m_chart->getOpacitySeries());
		double ymin = 0.0;
		double ymax = 1.0;
		if (pos.x() < m_xrange[0])
			pos.setX(m_xrange[0]);
		if (pos.x() > m_xrange[1])
			pos.setX(m_xrange[1]);
		if (pos.y() < ymin)
			pos.setY(ymin);
		if (pos.y() > ymax)
			pos.setY(ymax);
		m_chart->getOpacitySeries()->replace(m_opacityPointIndexIsMoving, pos);
	}
}

void OpacityChartView::mouseDoubleClickEvent(QMouseEvent * event)
{
	QChartView::mouseDoubleClickEvent(event);
	if (!event->isAccepted()) // The event is not processed by something else, then we want to add a point
	{
		QPointF pos_scene = this->mapToScene(event->pos());
		QPointF pos = m_chart->mapToValue(pos_scene, m_chart->getOpacitySeries());
		m_chart->getOpacitySeries()->append(pos);
	}
}

void OpacityChartView::redistributePointsToRange(double newMin, double newMax)
{
	auto series = m_chart->getOpacitySeries();
	double oldMin = m_xrange[0];
	double oldMax = m_xrange[1];
	double oldDist = oldMax - oldMin;
	double newDist = newMax - newMin;
	for (int i = 0; i < series->count(); ++i)
	{
		const auto &p = series->at(i);
		double x = newMin + newDist * ((p.x() - oldMin) / oldDist);
		series->replace(i, x, p.y());
	}
	this->updateOpacityFunction();
}


ColorChartView::ColorChartView(QWidget *parent, vtkColorTransferFunction *colorFunction)
	:QWidget(parent)
{
	m_colorFuction = colorFunction;

	m_comboColorTableSelector = new QComboBox(this);
	connect(m_comboColorTableSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, static_cast<void (ColorChartView::*)(int)>(&ColorChartView::setColorTable));

	m_minTableValue = new QDoubleSpinBox(this);
	m_maxTableValue = new QDoubleSpinBox(this);
	m_minTableValue->setDecimals(6);
	m_maxTableValue->setDecimals(6);
	connect(m_minTableValue, &QDoubleSpinBox::editingFinished, [=]() {
		double min = m_minTableValue->value();
		double max = m_maxTableValue->value();
		emit colorTableRangeChanged(min, max); 
	});
	connect(m_maxTableValue, &QDoubleSpinBox::editingFinished, [=]() {
		double min = m_minTableValue->value();
		double max = m_maxTableValue->value();
		emit colorTableRangeChanged(min, max);
	});

	
	m_scalarColorRed = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_scalarColorGreen = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_scalarColorBlue = vtkSmartPointer<vtkPiecewiseFunction>::New();
	m_chartViewRed = new OpacityChartView(this, m_scalarColorRed, OpacityChartView::Red);
	m_chartViewGreen = new OpacityChartView(this, m_scalarColorGreen, OpacityChartView::Green);
	m_chartViewBlue = new OpacityChartView(this, m_scalarColorBlue, OpacityChartView::Blue);
	connect(this, &ColorChartView::colorTableRangeChanged, m_chartViewRed, &OpacityChartView::setImageDataRange);
	connect(this, &ColorChartView::colorTableRangeChanged, m_chartViewGreen, &OpacityChartView::setImageDataRange);
	connect(this, &ColorChartView::colorTableRangeChanged, m_chartViewBlue, &OpacityChartView::setImageDataRange);
	connect(m_chartViewRed, &OpacityChartView::opacityFunctionChanged, this, &ColorChartView::updateColorFunction);
	connect(m_chartViewGreen, &OpacityChartView::opacityFunctionChanged, this, &ColorChartView::updateColorFunction);
	connect(m_chartViewBlue, &OpacityChartView::opacityFunctionChanged, this, &ColorChartView::updateColorFunction);

	addColorTable("CT", CT);
	addColorTable("TURBO", TURBO);
	addColorTable("JET", JET);
	addColorTable("GRAY", GRAY);
	addColorTable("HSV", HSV);
	addColorTable("PET", PET);
	addColorTable("MAGMA", MAGMA);
	addColorTable("SPRING", SPRING);
	addColorTable("SUMMER", SUMMER);
	addColorTable("COOL", COOL);
	addColorTable("TERRAIN", TERRAIN);
	addColorTable("BRG", BRG);
	addColorTable("BONE", BONE);
	addColorTable("SIMPLE", SIMPLE);
	addColorTable("HOT IRON", HOT_IRON);
	
	


	auto menyLayout = new QHBoxLayout;
	menyLayout->setContentsMargins(0, 0, 0, 0);
	menyLayout->addWidget(new QLabel(tr("Color table"), this));
	menyLayout->addWidget(m_comboColorTableSelector);
	menyLayout->addWidget(new QLabel(tr("Color table minimum"), this));
	menyLayout->addWidget(m_minTableValue);
	menyLayout->addWidget(new QLabel(tr("Color table maximum"), this));
	menyLayout->addWidget(m_maxTableValue);
	auto chartLayout = new QVBoxLayout;
	chartLayout->setContentsMargins(0, 0, 0, 0);
	chartLayout->addWidget(m_chartViewRed);
	chartLayout->addWidget(m_chartViewGreen);
	chartLayout->addWidget(m_chartViewBlue);
	auto layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addLayout(menyLayout);
	layout->addLayout(chartLayout);
	this->setLayout(layout);

	this->setColorTable(0);

}

void ColorChartView::setColorTable(int index)
{
	const QVector<double> &colorTable = m_predefinedColorTables[index];
	setColorTable(colorTable);
}

void ColorChartView::setColorTable(const QVector<double>& colorTable)
{
	m_pauseColorFunctionChangedSignal = true;
	m_chartViewRed->setColorTable(colorTable);
	m_chartViewGreen->setColorTable(colorTable);
	m_chartViewBlue->setColorTable(colorTable);
	m_pauseColorFunctionChangedSignal = false;
	updateColorFunction();
}

void ColorChartView::setImageDataRange(double min, double max)
{

	m_chartViewRed->setImageDataRange(min, max);
	m_chartViewGreen->setImageDataRange(min, max);
	m_chartViewBlue->setImageDataRange(min, max);
	m_minTableValue->setRange(min, max);
	m_maxTableValue->setRange(min, max);
	m_minTableValue->setValue(min);
	m_maxTableValue->setValue(max);
}

void ColorChartView::addColorTable(const QString & name, const QVector<double>& colorTable)
{
	m_comboColorTableSelector->addItem(name);
	m_predefinedColorTables.append(colorTable);
}

void ColorChartView::updateColorFunction()
{
	if (!m_pauseColorFunctionChangedSignal)
	{
		//interpolating all colors to a vtkcolortable
		QVector<double> points;
		auto seriesRed = m_chartViewRed->getOpacityChart()->getOpacitySeries()->pointsVector();
		auto seriesGreen = m_chartViewRed->getOpacityChart()->getOpacitySeries()->pointsVector();
		auto seriesBlue = m_chartViewRed->getOpacityChart()->getOpacitySeries()->pointsVector();
		for (const auto &p : seriesRed)
			points.append(p.x());
		for (const auto &p : seriesGreen)
			points.append(p.x());
		for (const auto &p : seriesBlue)
			points.append(p.x());
		m_colorFuction->RemoveAllPoints();

		auto opFunRed = m_chartViewRed->getOpacityFunction();
		auto opFunGreen = m_chartViewGreen->getOpacityFunction();
		auto opFunBlue = m_chartViewBlue->getOpacityFunction();

		for (auto x : points)
			m_colorFuction->AddRGBPoint(x, opFunRed->GetValue(x), opFunGreen->GetValue(x), opFunBlue->GetValue(x));

		emit colorFunctionChanged();
	}
}

VolumeCropWidget::VolumeCropWidget(QWidget *parent)
	:QWidget(parent)
{
	this->setWindowFlags(Qt::Tool);
	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QString labels[6] = { "Min X position", "Max X position", "Min Y position", "Max Y position", "Min Z position", "Max Z position" };

	for (int i = 0; i < 6; ++i)
	{
		QHBoxLayout *sublayout = new QHBoxLayout();
		sublayout->setContentsMargins(0, 0, 0, 0);
		m_sliders[i] = new QSlider(Qt::Horizontal, this);
		m_sliders[i]->setTracking(false);
		QLabel *label = new QLabel(labels[i], this);
		sublayout->addWidget(label);
		sublayout->addWidget(m_sliders[i]);
		layout->addLayout(sublayout);

		m_planeValues[i] = 0;
	}
	connect(m_sliders[0], &QSlider::valueChanged, this, &VolumeCropWidget::setMinX);
	connect(m_sliders[1], &QSlider::valueChanged, this, &VolumeCropWidget::setMaxX);
	connect(m_sliders[2], &QSlider::valueChanged, this, &VolumeCropWidget::setMinY);
	connect(m_sliders[3], &QSlider::valueChanged, this, &VolumeCropWidget::setMaxY);
	connect(m_sliders[4], &QSlider::valueChanged, this, &VolumeCropWidget::setMinZ);
	connect(m_sliders[5], &QSlider::valueChanged, this, &VolumeCropWidget::setMaxZ);
	this->setLayout(layout);
}

void VolumeCropWidget::setExtent(int extent[6])
{
	for (int i = 0; i < 6; ++i)
		m_sliders[i]->blockSignals(true);
	m_sliders[0]->setRange(extent[0], extent[1]);
	m_sliders[1]->setRange(extent[0], extent[1]);
	m_sliders[2]->setRange(extent[2], extent[3]);
	m_sliders[3]->setRange(extent[2], extent[3]);
	m_sliders[4]->setRange(extent[4], extent[5]);
	m_sliders[5]->setRange(extent[4], extent[5]);
	for (int i = 0; i < 6; ++i)
		m_sliders[i]->setValue(extent[i]);
	for (int i = 0; i < 6; ++i)
		m_sliders[i]->blockSignals(false);
	for (int i = 0; i < 6; ++i)
		m_planeValues[i] = extent[i];
}
void VolumeCropWidget::toggleVisibility(void)
{
	if (this->isVisible())
		this->hide();
	else
		this->show();
}


VolumeRenderSettingsWidget::VolumeRenderSettingsWidget(vtkSmartPointer<vtkVolumeProperty> prop, QWidget *parent)
	:QWidget(parent, Qt::Window), m_property(prop)
{

	m_currentImageType = ImageContainer::Empty;


	auto layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);

	//render box setting
	auto renderModeSelector = new QComboBox(this);
	renderModeSelector->addItem(tr("CPU ray caster (slooow but is more accurate)"));
	renderModeSelector->addItem(tr("GPU ray caster (fast if your graphics card is up for it)"));
#ifdef ENABLE_OSPRAY
	renderModeSelector->addItem(tr("OSPRay Intel CPU ray tracer"));
#endif // ENABLE_OSPRAY
	connect(renderModeSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), [=](int mode) {emit this->renderModeChanged(mode); });
	auto renderModeLayout = new QHBoxLayout;
	renderModeLayout->setContentsMargins(0, 0, 0, 0);
	renderModeLayout->addWidget(new QLabel(tr("Render mode"), this));
	renderModeLayout->addWidget(renderModeSelector);
	renderModeLayout->insertStretch(-1);
	layout->addLayout(renderModeLayout);

	//crop widget
	auto cropWidgetButton = new QPushButton(tr("Crop volume"), this);
	layout->addWidget(cropWidgetButton);
	m_volumeCropWidget = new VolumeCropWidget(this);
	connect(cropWidgetButton, &QPushButton::clicked, m_volumeCropWidget, &VolumeCropWidget::toggleVisibility);
	connect(m_volumeCropWidget, &VolumeCropWidget::croppingPlanesChanged, [=](int *planes) {emit cropPlanesChanged(planes); });

	// sliders
	auto sliderGroupBox = new QGroupBox(tr("Shading"), this);
	auto sliderGroupLayout = new QVBoxLayout;
	sliderGroupBox->setLayout(sliderGroupLayout);
	sliderGroupBox->setCheckable(true);
	sliderGroupBox->setChecked(true);
	connect(sliderGroupBox, &QGroupBox::toggled, [=](bool enabled) {this->getVolumeProperty()->SetShade(enabled); emit propertyChanged(); });
	QVector<QSlider *> sliders;
	sliders.append(new QSlider(Qt::Horizontal, this));
	sliders.append(new QSlider(Qt::Horizontal, this));
	sliders.append(new QSlider(Qt::Horizontal, this));
	QVector<QString> sliderNames;
	sliderNames.append(tr("Ambient setting [%]"));
	sliderNames.append(tr("Diffuse setting [%]"));
	sliderNames.append(tr("Specular setting [%]"));
	for (int i = 0 ; i < 3; i++)
	{
		sliders[i]->setMaximum(100);
		sliders[i]->setMinimum(0);
		sliders[i]->setTickInterval(1);
		auto sliderLayout = new QHBoxLayout;
		sliderLayout->setContentsMargins(0, 0, 0, 0);
		sliderLayout->addWidget(new QLabel(sliderNames[i], this));
		sliderLayout->addWidget(sliders[i]);
		sliderGroupLayout->addLayout(sliderLayout);
		connect(sliderGroupBox, &QGroupBox::toggled, sliders[1], &QSlider::setEnabled);
	}
	layout->addWidget(sliderGroupBox);
	sliders[0]->setValue(static_cast<int>(m_property->GetAmbient() * 100));
	sliders[1]->setValue(static_cast<int>(m_property->GetDiffuse() * 100));
	sliders[2]->setValue(static_cast<int>(m_property->GetSpecular() * 100));
	connect(sliders[0], &QSlider::valueChanged, [=](int value) {this->getVolumeProperty()->SetAmbient(static_cast<double>(value) / 100.0); emit propertyChanged(); });
	connect(sliders[1], &QSlider::valueChanged, [=](int value) {this->getVolumeProperty()->SetDiffuse(static_cast<double>(value) / 100.0); emit propertyChanged(); });
	connect(sliders[2], &QSlider::valueChanged, [=](int value) {this->getVolumeProperty()->SetSpecular(static_cast<double>(value) / 100.0); emit propertyChanged(); });
	

	m_scalarOpacityChart = new OpacityChartView(this, m_property->GetScalarOpacity());
	layout->addWidget(m_scalarOpacityChart, 1);
	connect(m_scalarOpacityChart, &OpacityChartView::opacityFunctionChanged, [=]() {emit propertyChanged(); });

	m_colorOpacityChart = new ColorChartView(this, m_property->GetRGBTransferFunction());
	layout->addWidget(m_colorOpacityChart, 3);
	connect(m_colorOpacityChart, &ColorChartView::colorFunctionChanged, [=]() {emit propertyChanged(); });
	connect(m_colorOpacityChart, &ColorChartView::colorTableRangeChanged, m_scalarOpacityChart, &OpacityChartView::setImageDataRange);
	//gradient opacity
	auto gradientOpacityGroupBox = new QGroupBox(tr("Gradient opacity"), this);
	auto gradientOpacityLayout = new QVBoxLayout;
	gradientOpacityGroupBox->setLayout(gradientOpacityLayout);
	gradientOpacityGroupBox->setCheckable(true);
	gradientOpacityGroupBox->setChecked(false);
	m_property->SetDisableGradientOpacity(true);
	connect(gradientOpacityGroupBox, &QGroupBox::toggled, [=](bool enabled) {this->getVolumeProperty()->SetDisableGradientOpacity(!enabled); emit propertyChanged(); });
	m_gradientOpacityChart = new OpacityChartView(this, m_property->GetStoredGradientOpacity(), OpacityChartView::Gradient);
	m_gradientOpacityChart->setVisible(false);
	gradientOpacityLayout->addWidget(m_gradientOpacityChart);
	connect(m_gradientOpacityChart, &OpacityChartView::opacityFunctionChanged, [=]() {emit propertyChanged(); });
	connect(gradientOpacityGroupBox, &QGroupBox::toggled, m_gradientOpacityChart, &OpacityChartView::setVisible);
	layout->addWidget(gradientOpacityGroupBox, 2);

	this->setLayout(layout);
}


VolumeRenderSettingsWidget::~VolumeRenderSettingsWidget()
{
	if (m_volumeCropWidget)
	{
		delete m_volumeCropWidget;
		m_volumeCropWidget = nullptr;
	}
}

void VolumeRenderSettingsWidget::toggleVisibility(void)
{
	if (this->isVisible())
		this->hide();
	else
		this->show();
}

void VolumeRenderSettingsWidget::setImage(std::shared_ptr<ImageContainer> image)
{
	if (!image)
		return;
	if (!image->image)
		return;
	// setting up image range
	m_scalarOpacityChart->setImageDataRange(image->minMax[0], image->minMax[1]);
	m_colorOpacityChart->setImageDataRange(image->minMax[0], image->minMax[1]);
	m_gradientOpacityChart->setImageDataRange(image->minMax[0], image->minMax[1]);
	
	// setting up dimensions
	int extent[6];
	image->image->GetExtent(extent);
	m_volumeCropWidget->setExtent(extent); 

	
	//setting up type and scalar lookuptables
	if (m_currentImageType != image->imageType)
	{
		QVector<QPointF> opacityPoints;
		QVector<QPointF> gradientPoints;
		if (image->imageType == ImageContainer::CTImage)
		{
			opacityPoints.append(QPointF(-500.0, 0.0));
			opacityPoints.append(QPointF( 000.0, 0.5));
			opacityPoints.append(QPointF( 500.0, 0.0));
			gradientPoints.append(QPointF(0.0, 0.75));
			m_scalarOpacityChart->setPoints(opacityPoints);
			m_gradientOpacityChart->setPoints(gradientPoints);
		}
		else if (image->imageType == ImageContainer::MaterialImage)
		{
			
			opacityPoints.append(QPointF(0.0, 0.0));
			double i = (image->minMax[1] - image->minMax[0]) > 1 ? 1 : 0;
			opacityPoints.append(QPointF(i - 0.05, 0.0));
			while (i <= image->minMax[1])
			{
				opacityPoints.append(QPointF(i, 0.2));
				i += 1.0;
			}
			gradientPoints.append(QPointF(0.0, 1.0));
			m_scalarOpacityChart->setPoints(opacityPoints);
			m_gradientOpacityChart->setPoints(gradientPoints);
		}
		else if (image->imageType == ImageContainer::DensityImage)
		{
			opacityPoints.append(QPointF(0.5, 0.0));
			opacityPoints.append(QPointF(1.0, 0.5));
			opacityPoints.append(QPointF(1.5, 0.0));
			gradientPoints.append(QPointF(0.0, 1.0));
			m_scalarOpacityChart->setPoints(opacityPoints);
			m_gradientOpacityChart->setPoints(gradientPoints);
		}
		else if (image->imageType == ImageContainer::DoseImage)
		{
			const double minPosPoint = std::min(0.1, (image->minMax[1] - image->minMax[0]) / 1000.0);
			opacityPoints.append(QPointF(image->minMax[0], 0.0));
			opacityPoints.append(QPointF(minPosPoint, 0.1));
			opacityPoints.append(QPointF(image->minMax[1], 1.0));
			gradientPoints.append(QPointF(0.0, 1.0));
			m_scalarOpacityChart->setPoints(opacityPoints);
			m_gradientOpacityChart->setPoints(gradientPoints);
		}
		m_currentImageType = image->imageType;
	}
	

}

