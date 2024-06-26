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

#include <ctaecplot.hpp>

#include <QChart>
#include <QGraphicsLayout>
#include <QGuiApplication>
#include <QLineSeries>
#include <QStyleHints>

CTAECPlot::CTAECPlot(QWidget* parent)
    : QChartView(parent)
{
    setContentsMargins(0, 0, 0, 0);
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::TextAntialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setMouseTracking(true);

    // chart and axis
    chart()->layout()->setContentsMargins(0, 0, 0, 0);
    if (QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark) {
        chart()->setTheme(QChart::ChartThemeDark);
    }
    chart()->setBackgroundVisible(false);
    chart()->setPlotAreaBackgroundVisible(false);
    chart()->setMargins(QMargins { 0, 0, 0, 0 });
    chart()->legend()->setVisible(false);

    m_xaxis = new QValueAxis(chart());
    chart()->addAxis(m_xaxis, Qt::AlignBottom);
    m_xaxis->setMinorGridLineVisible(false);
    m_xaxis->setGridLineVisible(false);
    m_xaxis->setTickCount(2);
    m_xaxis->setLabelsVisible(false);
    m_xaxis->setRange(-.1, 1.1);

    m_yaxis = new QValueAxis(chart());
    chart()->addAxis(m_yaxis, Qt::AlignLeft);
    m_yaxis->setGridLineVisible(false);
    m_yaxis->setRange(-.1, 1.1);
    m_yaxis->setMinorGridLineVisible(false);
    m_yaxis->setTickCount(2);
}

double dist(const std::array<double, 3>& v1, const std::array<double, 3>& v2)
{
    double l = 0;
    for (int i = 0; i < 3; ++i) {
        const auto s = v1[i] - v2[i];
        l += s * s;
    }
    return std::sqrt(l);
}

void CTAECPlot::updatePlot()
{
    if (!m_data) {
        chart()->setTitle("");
        chart()->removeAllSeries();
        return;
    }

    CTAECFilter aec = m_data->aecData();
    chart()->setTitle(tr("AEC from DiCOM exposure"));
    if (aec.isEmpty()) {
        aec = m_data->calculateAECfilterFromWaterEquivalentDiameter();
        chart()->setTitle(tr("AEC from water equiv. diameter"));
    }
    if (aec.isEmpty()) {
        chart()->removeAllSeries();
        chart()->setTitle("");
        return;
    }

    chart()->removeAllSeries();

    auto series_aec = new QLineSeries(this);
    const auto length = aec.length();
    const auto& weights = aec.weights();
    const auto step_aec = length / (weights.size() - 1);
    QList<QPointF> aec_qt(weights.size());
    for (std::size_t i = 0; i < weights.size(); ++i) {
        aec_qt[i].setX(i * step_aec - length / 2);
        aec_qt[i].setY(weights[i]);
    }
    series_aec->append(aec_qt);
    m_xaxis->setRange(-length / 2, length / 2);
    const auto max = *std::max_element(weights.cbegin(), weights.cend());
    m_yaxis->setRange(0.0, max * 1.1);
    chart()->addSeries(series_aec);
    series_aec->attachAxis(m_xaxis);
    series_aec->attachAxis(m_yaxis);

    auto series_one = new QLineSeries(this);
    QList<QPointF> one_qt(2);
    one_qt[0].setX(-length / 2);
    one_qt[0].setY(1);
    one_qt[1].setX(length / 2);
    one_qt[1].setY(1);
    series_one->append(one_qt);    
    chart()->addSeries(series_one);
    auto pen = series_one->pen();
    pen.setStyle(Qt::PenStyle::DotLine);
    pen.setWidthF(pen.widthF() / 2);
    series_one->setPen(pen);
    series_one->attachAxis(m_xaxis);
    series_one->attachAxis(m_yaxis);

    m_xaxis->setLabelsVisible(true);
}

void CTAECPlot::updateImageData(std::shared_ptr<DataContainer> d)
{
    if (m_data && d) {
        if (m_data->ID() == d->ID())
            return;
    }
    m_data = d;
    updatePlot();
}