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
#include <vtkImageData.h>
#include <vtkPiecewiseFunction.h>
#include <vtkVolume.h>

#include <QChartView>
#include <QGraphicsLayout>
#include <QLabel>
#include <QLineSeries>
#include <QPointF>
#include <QVBoxLayout>
#include <QValueAxis>

#include <algorithm>
#include <limits>

class LUTSeries : public QLineSeries {
public:
    LUTSeries(vtkVolume* volume, vtkPiecewiseFunction* lut, QObject* parent = nullptr)
        : m_volume(volume)
        , m_lut(lut)
        , QLineSeries(parent)
    {
        setMarkerSize(5);
        setPointsVisible(true);
        setPointLabelsFormat("@xPoint");
        setPointLabelsVisible(true);
        setPointLabelsClipping(false);

        connect(this, &QLineSeries::pressed, [=](const QPointF& point) {
            m_pIdx_edit = this->closestPointIndex(point);
        });
        connect(this, &QLineSeries::released, [=](const QPointF& point) {
            m_pIdx_edit = -1;
            updateLUT();
        });
        connect(this, &QLineSeries::doubleClicked, [=](const QPointF& point) {
            this->remove(point);
            updateLUT();
        });
        m_lut->GetRange(m_scalar_range.data());
        for (int i = 0; i < m_lut->GetSize(); ++i) {
            std::array<double, 4> data;
            m_lut->GetNodeValue(i, data.data());
            append(data[0], data[1]);
        }
    }

    int closestPointIndex(const QPointF& point) const
    {
        int idx = -1;
        double cost = std::numeric_limits<double>::max();
        for (int i = 0; i < count(); ++i) {
            const auto d = at(i) - point;
            auto ccost = d.x() * d.x() + d.y() * d.y();
            if (ccost < cost) {
                cost = ccost;
                idx = i;
            }
        }
        return idx;
    }

    void setScalarRange(double min, double max)
    {
        m_scalar_range = { min, max };
        auto valid_adjust = m_lut->AdjustRange(m_scalar_range.data());
        clear();
        for (int i = 0; i < m_lut->GetSize(); ++i) {
            std::array<double, 4> buf;
            m_lut->GetNodeValue(i, buf.data());
            append(buf[0], buf[1]);
        }
    }

    int currentClickedPoint() const { return m_pIdx_edit; }
    void setCurrentClickedPoint(int d) { m_pIdx_edit = d; }
    const std::array<double, 2> scalarRange() const { return m_scalar_range; }

    void swapPointsX(int idx1, int idx2)
    {
        auto p1 = at(idx1);
        auto p2 = at(idx2);
        p1.setY(p2.y());
        p2.setY(p1.y());
        replace(idx1, p2);
        replace(idx2, p1);
    }

    void insertPoint(const QPointF& p)
    {
        for (int i = 0; i < count(); ++i) {
            if (p.x() < at(i).x()) {
                insert(i, p);
                updateLUT();
                return;
            }
        }
        insert(count(), p);
        updateLUT();
    }

    void updateLUT()
    {
        m_lut->RemoveAllPoints();
        for (int i = 0; i < count(); ++i) {
            const auto& p = at(i);
            m_lut->AddPoint(p.x(), p.y());
        }
        m_volume->Update();
    }

private:
    int m_pIdx_edit = -1;
    std::array<double, 2> m_scalar_range = { 0, 0 };
    vtkPiecewiseFunction* m_lut = nullptr;
    vtkVolume* m_volume = nullptr;
};

class LUTChartView : public QChartView {
public:
    LUTChartView(vtkVolume* volume, vtkPiecewiseFunction* lut, QWidget* parent = nullptr)
        : QChartView(parent)
    {
        setContentsMargins(0, 0, 0, 0);
        setRenderHint(QPainter::Antialiasing, true);
        setRenderHint(QPainter::TextAntialiasing, true);
        setRenderHint(QPainter::SmoothPixmapTransform, true);
        setMouseTracking(true);

        // chart and axis
        chart()->setAnimationOptions(QChart::AnimationOption::SeriesAnimations);
        chart()->layout()->setContentsMargins(0, 0, 0, 0);
        // chart->setBackgroundRoundness(0);
        chart()->setTheme(QChart::ChartThemeDark);
        chart()->setBackgroundVisible(false);
        chart()->setMargins(QMargins { 0, 0, 0, 0 });
        chart()->legend()->setVisible(false);

        m_axisx = new QValueAxis(chart());
        m_axisx->setMinorGridLineVisible(false);
        m_axisx->setGridLineVisible(false);
        m_axisx->setTickCount(3);
        chart()->setAxisX(m_axisx);
        auto axisy = new QValueAxis(chart());
        axisy->setRange(0, 1.1);
        chart()->setAxisY(axisy);
        axisy->hide();

        m_lut_series = new LUTSeries(volume, lut, chart());
        connect(m_lut_series, &LUTSeries::pressed, [=](const auto& unused) { this->chart()->setAnimationOptions(QChart::AnimationOption::NoAnimation); });
        connect(m_lut_series, &LUTSeries::released, [=](const auto& unused) { this->chart()->setAnimationOptions(QChart::AnimationOption::SeriesAnimations); });
        chart()->addSeries(m_lut_series);
        m_lut_series->attachAxis(m_axisx);
        m_lut_series->attachAxis(axisy);
    }

    void setScalarRange(double min, double max)
    {
        m_lut_series->setScalarRange(min, max);
        m_axisx->setMin(min);
        m_axisx->setMax(max);
    }

    void mouseMoveEvent(QMouseEvent* event)
    {
        QChartView::mouseMoveEvent(event);
        if (const int pIdx = m_lut_series->currentClickedPoint(); (pIdx >= 0) && (event->buttons() == Qt::LeftButton)) {
            QPointF pos_scene = this->mapToScene(event->pos());
            QPointF pos = chart()->mapToValue(pos_scene, m_lut_series);
            const auto& scalar_range = m_lut_series->scalarRange();
            pos.setX(std::clamp(pos.x(), scalar_range[0], scalar_range[1]));
            pos.setY(std::clamp(pos.y(), 0.0, 1.0));
            m_lut_series->replace(pIdx, pos);

            // are we still sorted?
            const auto n = m_lut_series->count();
            if (pIdx > 0 && pIdx < n - 1) {
                if (const auto lower_x = m_lut_series->at(pIdx - 1).x(); pos.x() < lower_x) {
                    m_lut_series->swapPointsX(pIdx, pIdx - 1);
                    m_lut_series->setCurrentClickedPoint(pIdx - 1);
                } else if (const auto lower_x = m_lut_series->at(pIdx + 1).x(); pos.x() > lower_x) {
                    m_lut_series->swapPointsX(pIdx, pIdx + 1);
                    m_lut_series->setCurrentClickedPoint(pIdx + 1);
                }
            }
        }
    }

    void mouseDoubleClickEvent(QMouseEvent* event)
    {
        QChartView::mouseDoubleClickEvent(event);
        if (!event->isAccepted()) // The event is not processed by something else, then we want to add a point
        {
            QPointF pos_scene = this->mapToScene(event->pos());
            QPointF pos = chart()->mapToValue(pos_scene, m_lut_series);
            m_lut_series->insertPoint(pos);
        }
    }

private:
    LUTSeries* m_lut_series = nullptr;
    QValueAxis* m_axisx = nullptr;
};

VolumeLUTWidget::VolumeLUTWidget(vtkVolume* volume, vtkPiecewiseFunction* lut, vtkDiscretizableColorTransferFunction* colorlut, QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto view = new LUTChartView(volume, lut, this);
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
