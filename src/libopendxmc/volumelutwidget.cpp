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
        if (type == VolumeLUTWidget::LUTType::Opacity)
            m_lut = m_settings->getOpacityLut();
        else if (type == VolumeLUTWidget::LUTType::Gradient)
            m_lut = m_settings->getGradientLut();

        setMarkerSize(8);
        setPointsVisible(true);
        setPointLabelsFormat("@xPoint");
        setPointLabelsVisible(true);
        setPointLabelsClipping(true);

        connect(this, &QXYSeries::pressed, [=](const QPointF& point) {
            m_pIdx_edit = this->closestPointIndex(point);
        });
        connect(this, &QXYSeries::released, [=](const QPointF& point) {
            m_pIdx_edit = -1;
            updateLUTfromPoints();
            m_settings->render();
        });
        connect(this, &QXYSeries::doubleClicked, [=](const QPointF& point) {
            if (this->count() > 2) {
                this->remove(point);
                updateLUTfromPoints();
                m_settings->render();
            }
        });

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

    void imageDataUpdated()
    {
        // New image data, we want to adjust current lut to fit new scalar range
        auto data = m_settings->currentImageData;
        if (!data)
            return;

        if (m_type == VolumeLUTWidget::LUTType::Opacity) {
            newScalarRange();

            // we add histogram of data
            for (auto series : chart()->series())
                if (series != this)
                    chart()->removeSeries(series);

            auto hseries = new QLineSeries(chart());
            chart()->addSeries(hseries);
            hseries->setOpacity(0.4);
            hseries->setPointsVisible(false);
            for (auto axis : chart()->axes())
                hseries->attachAxis(axis);

            auto hist_pipe = vtkSmartPointer<vtkImageHistogram>::New();
            hist_pipe->SetMaximumNumberOfBins(128);
            hist_pipe->AutomaticBinningOn();
            hist_pipe->SetInputData(data);
            hist_pipe->Update();

            const auto start = hist_pipe->GetBinOrigin();
            const auto step = hist_pipe->GetBinSpacing();
            auto hist = hist_pipe->GetHistogram();
            const auto norm = 10 * static_cast<double>(hist_pipe->GetTotal()) / (hist_pipe->GetNumberOfBins());
            for (int i = 0; i < hist_pipe->GetNumberOfBins(); ++i) {
                auto y = hist->GetTuple(i);
                const auto x = start + step * i;
                hseries->append(x, y[0] / norm);
            }

        } else if (m_type == VolumeLUTWidget::LUTType::Gradient) {
            newScalarRange();
        }
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

    void insertPoint(const QPointF& p)
    {
        for (int i = 0; i < count(); ++i) {
            const auto& point = at(i);
            if (p.x() < point.x()) {
                insert(i, p);
                break;
            }
        }
        insert(count(), p);
        updateLUTfromPoints();
        m_settings->render();
    }

    void updateLUTfromPoints()
    {
        m_lut->RemoveAllPoints();
        for (int i = 0; i < count(); ++i) {
            const auto& p = at(i);
            m_lut->AddPoint(p.x(), p.y());
        }

        // updating view range
        const auto start = at(0).x();
        const auto stop = at(count() - 1).x();
        const auto& r = m_settings->currentImageDataScalarRange;
        m_settings->viewScalarRangeFraction[0] = (start - r[0]) / (r[1] - r[0]);
        m_settings->viewScalarRangeFraction[1] = (stop - r[0]) / (r[1] - r[0]);

        updateColorfomPoints();

        m_settings->volume->Update();
    }

protected:
    void newScalarRange()
    {
        // We adjust current lut to new scalar range from images
        if (m_lut->GetSize() != this->count()) {
            // something is off, we reset (should not happend)
            auto valid_adjust = m_lut->AdjustRange(m_settings->currentImageDataScalarRange.data());
            this->clear();
            for (int i = 0; i < m_lut->GetSize(); ++i) {
                std::array<double, 4> buf;
                m_lut->GetNodeValue(i, buf.data());
                append(buf[0], buf[1]);
            }
            m_settings->volume->Update();
            return;
        }
        if (m_type == VolumeLUTWidget::LUTType::Opacity) {
            auto old_pos = m_lut->GetRange();
            auto old_range = (old_pos[0] - old_pos[1]) / (m_settings->viewScalarRangeFraction[0] - m_settings->viewScalarRangeFraction[1]);
            auto old_min = old_pos[0] - m_settings->viewScalarRangeFraction[0] * old_range;
            const auto min = m_settings->currentImageDataScalarRange[0];
            const auto max = m_settings->currentImageDataScalarRange[1];
            const auto range = max - min;

            for (int i = 0; i < count(); ++i) {
                auto point = this->at(i);
                const auto old_x = point.x();
                const auto t = (old_x - old_min) / old_range;
                const auto x = min + t * range;
                point.setX(std::clamp(x, min, max));
                this->replace(i, point);
            }
        } else {
            const auto& r = m_settings->currentImageDataScalarRange;
            std::array<double, 2> range = { 0, (r[1] - r[0]) / 10 };
            m_lut->AdjustRange(range.data());
        }

        updateLUTfromPoints();
    }

    void updateColorfomPoints()
    {
        const auto& vr = m_settings->currentImageDataScalarRange;
        const auto range = vr[1] - vr[0];
        const auto min = vr[0] + m_settings->viewScalarRangeFraction[0] * range;
        const auto max = vr[0] + m_settings->viewScalarRangeFraction[1] * range;

        auto clut = m_settings->color_lut;

        std::array<double, 6> buffer;
        clut->GetNodeValue(0, buffer.data());
        const auto old_min = buffer[0];
        clut->GetNodeValue(clut->GetSize() - 1, buffer.data());
        const auto old_max = buffer[0];
        const auto old_range = old_max - old_min;
        for (int i = 0; i < clut->GetSize(); ++i) {
            clut->GetNodeValue(i, buffer.data());
            auto old_x = buffer[0];
            const auto t = (old_x - old_min) / old_range;
            buffer[0] = min + t * range;
            clut->SetNodeValue(i, buffer.data());
        }
    }

private:
    VolumeLUTWidget::LUTType m_type = VolumeLUTWidget::LUTType::Opacity;
    int m_pIdx_edit = -1;
    VolumeRenderSettings* m_settings = nullptr;
    vtkPiecewiseFunction* m_lut = nullptr;
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
        chart()->setAnimationOptions(QChart::AnimationOption::SeriesAnimations);
        chart()->layout()->setContentsMargins(0, 0, 0, 0);
        // chart->setBackgroundRoundness(0);
        chart()->setTheme(QChart::ChartThemeDark);
        chart()->setBackgroundVisible(false);
        // if (type == VolumeLUTWidget::LUTType::Opacity)
        chart()->setPlotAreaBackgroundVisible(true);
        // chart()->setMargins(QMargins { 0, 0, 0, 0 });
        chart()->legend()->setVisible(false);

        m_axisx = new QValueAxis(chart());
        m_axisx->setMinorGridLineVisible(false);
        m_axisx->setGridLineVisible(false);
        m_axisx->setTickCount(3);
        chart()->setAxisX(m_axisx);
        auto axisy = new QValueAxis(chart());
        axisy->setGridLineVisible(false);
        axisy->setRange(0, 1);
        chart()->setAxisY(axisy);
        axisy->setLabelsVisible(false);
        axisy->setMinorGridLineVisible(false);
        axisy->setTickCount(2);
        // axisy->hide();

        m_lut_series = new LUTSeries(settings, type, chart());
        connect(m_lut_series, &LUTSeries::pressed, [=](const auto& unused) { this->chart()->setAnimationOptions(QChart::AnimationOption::NoAnimation); });
        connect(m_lut_series, &LUTSeries::released, [=](const auto& unused) { this->chart()->setAnimationOptions(QChart::AnimationOption::SeriesAnimations); });
        chart()->addSeries(m_lut_series);
        m_lut_series->attachAxis(m_axisx);
        m_lut_series->attachAxis(axisy);
    }

    void imageDataUpdated()
    {
        m_lut_series->imageDataUpdated();
        const auto& range = m_settings->currentImageDataScalarRange;
        m_axisx->setMin(range[0]);
        m_axisx->setMax(range[1]);
    }

    void updatechartColorBackground()
    {
        auto q = chart();
        if (!q->isPlotAreaBackgroundVisible())
            return;

        auto view_rect = chart()->plotArea();
        QLinearGradient g(view_rect.bottomLeft(), view_rect.bottomRight());
        auto lut = m_settings->color_lut;
        double min, max;
        if (m_settings->currentImageData) {
            const auto& r = m_settings->currentImageDataScalarRange;
            min = r[0] + (r[1] - r[0]) * m_settings->viewScalarRangeFraction[0];
            max = r[0] + (r[1] - r[0]) * m_settings->viewScalarRangeFraction[1];
        } else {
            lut->GetRange(min, max);
        }
        const auto inv_drange = 1.0 / (max - min);
        const auto N = lut->GetSize();

        auto data = lut->GetDataPointer();
        std::vector<double> test;
        for (int i = 0; i < N; ++i) {
            const auto dx = i * 4;
            auto frac = (data[dx] - min) * inv_drange;
            auto c = QColor::fromRgbF(data[dx + 1], data[dx + 2], data[dx + 3], 0.5);
            g.setColorAt(frac, c);
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

            const auto& scalar_range = m_settings->currentImageDataScalarRange;
            pos.setX(std::clamp(pos.x(), scalar_range[0], scalar_range[1]));
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
        if (event->buttons() == Qt::LeftButton)
            updatechartColorBackground();
    }

    void mouseDoubleClickEvent(QMouseEvent* event)
    {
        QChartView::mouseDoubleClickEvent(event);
        if (!event->isAccepted()) // The event is not processed by something else, then we want to add a point
        {
            QPointF pos_scene = this->mapToScene(event->pos());
            QPointF pos = chart()->mapToValue(pos_scene, m_lut_series);
            m_lut_series->insertPoint(pos);
            updatechartColorBackground();
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
    connect(this, &VolumeLUTWidget::imageDataChanged, [=]() { view->imageDataUpdated(); });
    connect(this, &VolumeLUTWidget::colorDataChanged, [=]() { view->colorDataUpdated(); });

    if (type == LUTType::Opacity) {
        auto view_label = new QLabel("Opacity LUT", this);
        view_label->setAlignment(Qt::AlignHCenter);
        layout->addWidget(view_label);

    } else if (type == LUTType::Gradient) {
        auto checkbox = new QCheckBox("Gradient LUT");
        checkbox->setCheckState(Qt::CheckState::Unchecked);
        this->m_settings->getVolumeProperty()->SetDisableGradientOpacity(true);
        connect(checkbox, &QCheckBox::stateChanged, [=](int state) {
            if (state == 0) {
                view->setEnabled(false);
                this->m_settings->getVolumeProperty()->SetDisableGradientOpacity(true);
            } else {
                view->setEnabled(true);
                this->m_settings->getVolumeProperty()->SetDisableGradientOpacity(false);
            }
            m_settings->render();
        });

        layout->addWidget(checkbox);
        view->setDisabled(true);
    }

    layout->addWidget(view);

    setLayout(layout);
}

void VolumeLUTWidget::setColorData(const std::vector<double>& data)
{
    auto lut = m_settings->color_lut;
    const auto& range = m_settings->currentImageDataScalarRange;
    const auto min = range[0] + m_settings->viewScalarRangeFraction[0] * (range[1] - range[0]);
    const auto max = range[0] + m_settings->viewScalarRangeFraction[1] * (range[1] - range[0]);
    const auto N = data.size() / 3;
    const auto step = (max - min) / (N - 1);
    lut->RemoveAllPoints();
    for (int i = 0; i < N; ++i) {
        auto cIdx = i * 3;
        const auto x = min + step * i;
        lut->AddRGBPoint(x, data[cIdx], data[cIdx + 1], data[cIdx + 2]);
    }
    emit colorDataChanged();
    m_settings->volume->Update();
    m_settings->render();
}

void VolumeLUTWidget::imageDataUpdated()
{
    emit imageDataChanged();
}
