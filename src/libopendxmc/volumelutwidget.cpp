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

#include <volumelutwidget.hpp>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <vtkImageData.h>

#include <QChartView>
#include <QGraphicsLayout>
#include <QLabel>
#include <QLineSeries>
#include <QVBoxLayout>
#include <QValueAxis>

class LUTSeries : public QLineSeries {

public:
    LUTSeries(vtkPiecewiseFunction* lut, QObject* parent = nullptr)
        : m_lut(lut)
        , QLineSeries(parent)
    {
        setPointsVisible(true);
        for (int i = 0; i < m_lut->GetSize(); ++i) {
            std::array<double, 4> data;
            m_lut->GetNodeValue(i, data.data());
            append(data[0], data[1]);
        }
    }

    void setScalarRange(double min, double max)
    {
        std::array<double, 2> range = { min, max };
        auto valid_adjust = m_lut->AdjustRange(range.data());
    }

private:
    vtkPiecewiseFunction* m_lut = nullptr;
};

class LUTChartView : public QChartView {
public:
    LUTChartView(vtkPiecewiseFunction* lut, QWidget* parent = nullptr)
        : QChartView(parent)
    {
        setContentsMargins(0, 0, 0, 0);
        setRenderHint(QPainter::Antialiasing, true);
        setRenderHint(QPainter::TextAntialiasing, true);
        setRenderHint(QPainter::SmoothPixmapTransform, true);

        // chart and axis
        chart()->setAnimationOptions(QChart::AnimationOption::SeriesAnimations);
        chart()->layout()->setContentsMargins(0, 0, 0, 0);
        // chart->setBackgroundRoundness(0);
        chart()->setTheme(QChart::ChartThemeDark);
        chart()->setBackgroundVisible(false);
        chart()->setMargins(QMargins { 0, 0, 0, 0 });
        chart()->legend()->setVisible(false);

        m_axisx = new QValueAxis(chart());
        chart()->setAxisX(m_axisx);
        auto axisy = new QValueAxis(chart());
        axisy->setRange(0, 1);
        chart()->setAxisY(axisy);

        m_lut_series = new LUTSeries(lut, chart());
        chart()->addSeries(m_lut_series);
    }

    void setScalarRange(double min, double max)
    {
        m_lut_series->setScalarRange(min, max);
        m_axisx->setRange(min, max);
    }

private:
    LUTSeries* m_lut_series = nullptr;
    QValueAxis* m_axisx = nullptr;
};

VolumeLUTWidget::VolumeLUTWidget(vtkPiecewiseFunction* lut, vtkDiscretizableColorTransferFunction* colorlut, QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto view = new LUTChartView(lut, this);
    connect(this, &VolumeLUTWidget::scalarRangeChanged, [=](double min, double max) { view->setScalarRange(min, max); });
    auto view_label = new QLabel("Opacity LUT", this);
    view_label->setAlignment(Qt::AlignHCenter);
    layout->addWidget(view_label);
    layout->addWidget(view);

    setLayout(layout);
    layout->addStretch();
}

void VolumeLUTWidget::setColorData(const std::vector<double>& data)
{
}

void VolumeLUTWidget::setImageData(vtkImageData* data)
{
    // set data range
    data->GetScalarRange(m_value_range.data());
    emit scalarRangeChanged(m_value_range[0], m_value_range[1]);
}
