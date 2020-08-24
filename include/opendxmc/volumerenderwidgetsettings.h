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

#include "opendxmc/imagecontainer.h"

#include <QChartView>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QGraphicsLayout>
#include <QGraphicsSceneMouseEvent>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPointF>
#include <QPushButton>
#include <QScatterSeries>
#include <QSlider>
#include <QString>
#include <QVBoxLayout>
#include <QValueAxis>
#include <QVector>
#include <QWidget>

#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkSmartPointer.h>
#include <vtkVolumeProperty.h>

#include <memory>

using namespace QtCharts;

class OpacitySeries : public QScatterSeries {
    Q_OBJECT
public:
    OpacitySeries(QObject* parent = nullptr);
    int getPointIsPressedIndex(void) { return m_pointIsPressedIndex; }

private:
    void setPointIsPressed(const QPointF& point);
    void setPointIsReleased(const QPointF& point);
    int m_pointIsPressedIndex = -1;
};

class OpacityChart : public QChart {
    Q_OBJECT
public:
    OpacityChart();
    OpacitySeries* getOpacitySeries() { return m_series; }

private:
    OpacitySeries* m_series;
};

class OpacityChartView : public QChartView {
    Q_OBJECT
public:
    enum Color { Red,
        Green,
        Blue,
        None,
        Gradient };
    OpacityChartView(QWidget* parent = nullptr, vtkPiecewiseFunction* opacityFunction = nullptr, OpacityChartView::Color color = OpacityChartView::None);
    void setPoints(const QVector<QPointF>& points);
    void setImageDataRange(double min, double max);
    OpacityChart* getOpacityChart() { return m_chart; }
    void updateOpacityFunction();
    void setColorTable(const QVector<double>& colorTable);
    vtkPiecewiseFunction* getOpacityFunction() const { return m_opacityFunction; }

signals:
    void opacityFunctionChanged();

protected:
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mouseDoubleClickEvent(QMouseEvent* event);
    void redistributePointsToRange(double newMin, double newMax);

private:
    OpacityChart* m_chart;
    int m_opacityPointIndexIsMoving = -1;
    vtkPiecewiseFunction* m_opacityFunction;
    double m_xrange[2] = { 0, 1 };
    Color m_color = None;
};

class ColorChartView : public QWidget {
    Q_OBJECT
public:
    ColorChartView(QWidget* parent = nullptr, vtkColorTransferFunction* colorFunction = nullptr);

    void setColorTable(int index);
    void setColorTable(const QVector<double>& colorTable);
    void setImageDataRange(double min, double max);
signals:
    void colorFunctionChanged();
    void colorTableRangeChanged(double min, double max);

private:
    void addColorTable(const QString& name, const QVector<double>& colorTable);
    void updateColorFunction();

    QComboBox* m_comboColorTableSelector;
    QDoubleSpinBox* m_minTableValue;
    QDoubleSpinBox* m_maxTableValue;
    QVector<QVector<double>> m_predefinedColorTables;
    vtkSmartPointer<vtkPiecewiseFunction> m_scalarColorRed;
    vtkSmartPointer<vtkPiecewiseFunction> m_scalarColorGreen;
    vtkSmartPointer<vtkPiecewiseFunction> m_scalarColorBlue;
    OpacityChartView* m_chartViewRed;
    OpacityChartView* m_chartViewGreen;
    OpacityChartView* m_chartViewBlue;
    vtkColorTransferFunction* m_colorFuction;
    bool m_pauseColorFunctionChangedSignal = false;
};

class VolumeCropWidget : public QWidget {
    Q_OBJECT
public:
    VolumeCropWidget(QWidget* parent = nullptr);
    void toggleVisibility(void);
    void setExtent(int extent[6]);
signals:
    void croppingPlanesChanged(int* planes);

private slots:
    void setMinX(int val)
    {
        m_planeValues[0] = val;
        emit croppingPlanesChanged(m_planeValues);
    }
    void setMaxX(int val)
    {
        m_planeValues[1] = val;
        emit croppingPlanesChanged(m_planeValues);
    }
    void setMinY(int val)
    {
        m_planeValues[2] = val;
        emit croppingPlanesChanged(m_planeValues);
    }
    void setMaxY(int val)
    {
        m_planeValues[3] = val;
        emit croppingPlanesChanged(m_planeValues);
    }
    void setMinZ(int val)
    {
        m_planeValues[4] = val;
        emit croppingPlanesChanged(m_planeValues);
    }
    void setMaxZ(int val)
    {
        m_planeValues[5] = val;
        emit croppingPlanesChanged(m_planeValues);
    }

private:
    int m_planeValues[6] = { 0, 1, 0, 1, 0, 1 };

    QSlider* m_sliders[6];
};

class VolumeRenderSettingsWidget : public QWidget {
    Q_OBJECT
public:
    VolumeRenderSettingsWidget(vtkSmartPointer<vtkVolumeProperty> prop, QWidget* parent = nullptr);
    ~VolumeRenderSettingsWidget();
    void toggleVisibility(void);
    void setImage(std::shared_ptr<ImageContainer> image);
    void setColorTable(const QVector<double>& colortable) { m_colorOpacityChart->setColorTable(colortable); }
    vtkVolumeProperty* getVolumeProperty() { return m_property; }

signals:
    void propertyChanged(void);
    void renderModeChanged(int mode);
    void cropPlanesChanged(int planes[6]);

private:
    vtkSmartPointer<vtkVolumeProperty> m_property; // this is the volumeproperty
    OpacityChartView* m_scalarOpacityChart;
    ColorChartView* m_colorOpacityChart;
    OpacityChartView* m_gradientOpacityChart;
    VolumeCropWidget* m_volumeCropWidget;
    ImageContainer::ImageType m_currentImageType;
};
