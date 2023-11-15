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
#include <volumerendersettings.hpp>

#include <vtkIdTypeArray.h>
#include <vtkImageData.h>
#include <vtkImageGradientMagnitude.h>
#include <vtkImageHistogram.h>
#include <vtkVolumeProperty.h>

#include <QBrush>
#include <QChartView>
#include <QCheckBox>
#include <QGraphicsLayout>
#include <QLabel>
#include <QLineSeries>
#include <QLinearGradient>
#include <QPointF>
#include <QScatterSeries>
#include <QVBoxLayout>
#include <QValueAxis>

#include <algorithm>
#include <limits>
#include <vector>

double shiftScale(double old_value, double old_min, double old_max, double min, double max)
{
    const auto t = (old_value - old_min) / (old_max - old_min);
    return min + t * (max - min);
}

class LUTSeries : public QScatterSeries {
public:
    LUTSeries(VolumeRenderSettings* settings, VolumeLUTWidget::LUTType type, QObject* parent = nullptr)
        : m_settings(settings)
        , m_type(type)
        , QScatterSeries(parent)
    {
        if (!m_settings)
            return;

        setMarkerSize(8);
        setPointsVisible(true);
        setPointLabelsFormat("@xPoint");
        // setPointLabelsVisible(true);
        // setPointLabelsClipping(true);

        connect(this, &QXYSeries::pressed, [=](const QPointF& point) {
            m_pIdx_edit = this->closestPointIndex(point);
        });
        connect(this, &QXYSeries::released, [=](const QPointF& point) {
            m_pIdx_edit = -1;
            updateLUTfromPoints();
        });
        connect(this, &QXYSeries::doubleClicked, [=](const QPointF& point) {
            if (this->count() > 2) {
                this->remove(point);
                updateLUTfromPoints();
            }
        });

        if (type == VolumeLUTWidget::LUTType::Opacity) {
            connect(m_settings, &VolumeRenderSettings::imageDataChanged, [=]() { this->imageDataUpdated(); });
            for (const auto& p : m_settings->opacityDataNormalized()) {
                append(p[0], p[1]);
            }
        } else if (type == VolumeLUTWidget::LUTType::Gradient) {
            for (const auto& p : m_settings->gradientDataNormalized()) {
                append(p[0], p[1]);
            }
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

    int currentClickedPoint() const
    {
        return m_pIdx_edit;
    }
    void setCurrentClickedPoint(int d)
    {
        m_pIdx_edit = d;
    }

    void swapPointsX(int idx1, int idx2)
    {
        auto p1 = at(idx1);
        auto p2 = at(idx2);
        p1.setY(p2.y());
        p2.setY(p1.y());
        replace(idx1, p2);
        replace(idx2, p1);
    }

    void insertPoint(QPointF p)
    {
        p.setX(std::clamp(p.x(), 0.0, 1.0));
        p.setY(std::clamp(p.y(), 0.0, 1.0));

        for (int i = 0; i < count(); ++i) {
            const auto& point = at(i);
            if (p.x() < point.x()) {
                insert(i, p);
                updateLUTfromPoints();
                return;
            }
        }
        insert(count(), p);
        updateLUTfromPoints();
    }

    void updateLUTfromPoints()
    {
        std::vector<std::array<double, 2>> d(count());
        for (int i = 0; i < count(); ++i) {
            const auto& p = this->at(i);
            d[i] = { p.x(), p.y() };
        }

        if (m_type == VolumeLUTWidget::LUTType::Opacity) {
            m_settings->setOpacityDataNormalized(d);
        } else if (m_type == VolumeLUTWidget::LUTType::Gradient) {
            m_settings->setGradientDataNormalized(d);
        }
    }

protected:
    void imageDataUpdated()
    {
        auto data = m_settings->currentImageData();
        if (!data)
            return;

        if (m_type == VolumeLUTWidget::LUTType::Opacity) {

            // we add histogram of data
            auto chart_ptr = chart();

            for (auto serie : chart_ptr->series())
                if (serie != this)
                    chart_ptr->removeSeries(serie);

            auto hseries = new QLineSeries(chart_ptr);
            chart_ptr->addSeries(hseries);
            hseries->setOpacity(0.4);
            hseries->setPointsVisible(false);
            for (auto axis : chart_ptr->axes())
                hseries->attachAxis(axis);

            auto hist_pipe = vtkSmartPointer<vtkImageHistogram>::New();
            hist_pipe->SetMaximumNumberOfBins(128);
            hist_pipe->AutomaticBinningOn();
            hist_pipe->SetInputData(data);
            hist_pipe->ReleaseDataFlagOn();
            hist_pipe->Update();

            const auto start = hist_pipe->GetBinOrigin();
            const auto step = hist_pipe->GetBinSpacing();
            const auto N = hist_pipe->GetNumberOfBins();
            const auto range = N * step;
            auto hist = hist_pipe->GetHistogram();
            const auto norm = 10 * static_cast<double>(hist_pipe->GetTotal()) / (hist_pipe->GetNumberOfBins());
            for (int i = 0; i < N; ++i) {
                auto y = hist->GetTuple(i);
                const auto x_orig = start + step * i;
                const auto x = (x_orig - start) / range;
                hseries->append(x, y[0] / norm);
            }
        }
    }

private:
    VolumeLUTWidget::LUTType m_type = VolumeLUTWidget::LUTType::Opacity;
    VolumeRenderSettings* m_settings = nullptr;
    int m_pIdx_edit = -1;
};

class LUTChartView : public QChartView {
public:
    LUTChartView(VolumeRenderSettings* settings, VolumeLUTWidget::LUTType type, QWidget* parent = nullptr)
        : m_settings(settings)
        , QChartView(parent)
    {
        setContentsMargins(0, 0, 0, 0);
        setRenderHint(QPainter::Antialiasing, true);
        setRenderHint(QPainter::TextAntialiasing, true);
        setRenderHint(QPainter::SmoothPixmapTransform, true);
        setMouseTracking(true);

        // chart and axis
        // chart()->setAnimationOptions(QChart::AnimationOption::SeriesAnimations);
        chart()->layout()->setContentsMargins(0, 0, 0, 0);
        // chart->setBackgroundRoundness(0);
        chart()->setTheme(QChart::ChartThemeDark);
        chart()->setBackgroundVisible(false);
        if (type == VolumeLUTWidget::LUTType::Opacity)
            chart()->setPlotAreaBackgroundVisible(true);
        chart()->setMargins(QMargins { 0, 0, 0, 0 });
        chart()->legend()->setVisible(false);

        m_axisx = new QValueAxis(chart());
        m_axisx->setMinorGridLineVisible(false);
        m_axisx->setGridLineVisible(false);
        m_axisx->setTickCount(3);
        m_axisx->setLabelsVisible(false);
        chart()->setAxisX(m_axisx);
        auto axisy = new QValueAxis(chart());
        axisy->setGridLineVisible(false);
        axisy->setRange(-.1, 1.1);
        chart()->setAxisY(axisy);
        axisy->setLabelsVisible(false);
        axisy->setMinorGridLineVisible(false);
        axisy->setTickCount(2);
        axisy->setLabelsVisible(false);
        // axisy->hide();
        connect(m_settings, &VolumeRenderSettings::imageDataChanged, [=](void) { this->updateAxisScalarRange(); });

        m_lut_series = new LUTSeries(settings, type, chart());
        chart()->addSeries(m_lut_series);
        m_lut_series->attachAxis(m_axisx);
        m_lut_series->attachAxis(axisy);
        updateAxisScalarRange();
        connect(m_settings, &VolumeRenderSettings::colorLutChanged, [=](void) { this->updatechartColorBackground(); });
    }

    void updateAxisScalarRange()
    {
        /* const auto& range = m_settings->currentImageDataScalarRange();
        m_axisx->setMin(range[0]);
        m_axisx->setMax(range[1]);
        */
        m_axisx->setMin(-.1);
        m_axisx->setMax(1.1);
    }

    void updatechartColorBackground()
    {
        auto q = chart();
        if (!q->isPlotAreaBackgroundVisible())
            return;
        auto view_rect = chart()->plotArea();
        QLinearGradient g(view_rect.bottomLeft(), view_rect.bottomRight());

        const auto cdata = m_settings->getCropColorToOpacityRange() ? m_settings->colorDataNormalizedCroppedToOpacity() : m_settings->colorDataNormalized();
        for (const auto& node : cdata) {
            auto c = QColor::fromRgbF(node[1], node[2], node[3], 0.5);
            g.setColorAt(node[0], c);
        }
        QBrush brush(g);
        q->setPlotAreaBackgroundBrush(brush);
    }

    void colorDataUpdated()
    {
        updatechartColorBackground();
    }

    void mouseMoveEvent(QMouseEvent* event)
    {
        QChartView::mouseMoveEvent(event);
        // to move a point
        if (const int pIdx = m_lut_series->currentClickedPoint(); (pIdx >= 0) && (event->buttons() == Qt::LeftButton)) {

            QPointF pos_scene = this->mapToScene(event->pos());
            QPointF pos = chart()->mapToValue(pos_scene, m_lut_series);

            pos.setX(std::clamp(pos.x(), 0.0, 1.0));
            pos.setY(std::clamp(pos.y(), 0.0, 1.0));

            m_lut_series->replace(pIdx, pos);

            // are we still sorted?
            const auto n = m_lut_series->count();
            if (pIdx > 0 && pIdx < n - 1) {
                if (const auto lower_x = m_lut_series->at(pIdx - 1).x(); pos.x() < lower_x) {
                    m_lut_series->swapPointsX(pIdx, pIdx - 1);
                    m_lut_series->setCurrentClickedPoint(pIdx - 1);
                } else if (const auto upper_x = m_lut_series->at(pIdx + 1).x(); pos.x() > upper_x) {
                    m_lut_series->swapPointsX(pIdx, pIdx + 1);
                    m_lut_series->setCurrentClickedPoint(pIdx + 1);
                }
            }
        }
    }

    void mouseReleaseEvent(QMouseEvent* event)
    {
        QChartView::mouseReleaseEvent(event);
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

    void resizeEvent(QResizeEvent* event)
    {
        QChartView::resizeEvent(event);
        updatechartColorBackground();
    }

private:
    VolumeRenderSettings* m_settings = nullptr;
    LUTSeries* m_lut_series = nullptr;
    QValueAxis* m_axisx = nullptr;
};

VolumeLUTWidget::VolumeLUTWidget(VolumeRenderSettings* settings, VolumeLUTWidget::LUTType type, QWidget* parent)
    : m_settings(settings)
    , QWidget(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto view = new LUTChartView(m_settings, type, this);

    if (type == LUTType::Opacity) {
        auto view_label = new QLabel("Opacity LUT", this);
        view_label->setAlignment(Qt::AlignHCenter);
        layout->addWidget(view_label);

    } else if (type == LUTType::Gradient) {
        auto checkbox = new QCheckBox("Gradient LUT");
        checkbox->setCheckState(Qt::CheckState::Unchecked);
        this->m_settings->volumeProperty()->SetDisableGradientOpacity(true);
        connect(checkbox, &QCheckBox::stateChanged, [=](int state) {
            if (state == 0) {
                view->setEnabled(false);
                this->m_settings->volumeProperty()->SetDisableGradientOpacity(true);
            } else {
                view->setEnabled(true);
                this->m_settings->volumeProperty()->SetDisableGradientOpacity(false);
            }
            m_settings->render();
        });
        layout->addWidget(checkbox);
        view->setDisabled(true);
    }
    layout->addWidget(view);
    setLayout(layout);
}
