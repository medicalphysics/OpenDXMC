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
#include <QLineSeries>

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
    chart()->setTheme(QChart::ChartThemeDark);
    chart()->setBackgroundVisible(false);
    chart()->setPlotAreaBackgroundVisible(false);
    chart()->setMargins(QMargins { 0, 0, 0, 0 });
    chart()->legend()->setVisible(false);

    m_xaxis = new QValueAxis(chart());
    chart()->setAxisX(m_xaxis);
    m_xaxis->setMinorGridLineVisible(false);
    m_xaxis->setGridLineVisible(false);
    m_xaxis->setTickCount(2);
    m_xaxis->setLabelsVisible(false);
    m_xaxis->setRange(-.1, 1.1);

    m_yaxis = new QValueAxis(chart());
    chart()->setAxisY(m_yaxis);
    m_yaxis->setGridLineVisible(false);
    m_yaxis->setRange(-.1, 1.1);
    m_yaxis->setLabelsVisible(false);
    m_yaxis->setMinorGridLineVisible(false);
    m_yaxis->setTickCount(2);
    m_yaxis->setLabelsVisible(false);
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
    if (!m_data)
        return;

    auto series_aec = new QLineSeries(this);
    const auto& aec = m_data->aecData();
    const auto length = dist(aec.startPosition, aec.stopPosition) / 2;
    const auto step_aec = 2 * length / (aec.weights.size() - 1);
    QList<QPointF> aec_qt(aec.weights.size());
    for (std::size_t i = 0; i < aec.weights.size(); ++i) {
        aec_qt[i].setX(i * step_aec - length);
        aec_qt[i].setY(aec.weights[i]);
    }
    series_aec->append(aec_qt);
    m_xaxis->setRange(-length, length);
    const auto max = *std::max_element(aec.weights.cbegin(), aec.weights.cend());
    m_yaxis->setRange(0.0, max * 1.1);

    chart()->removeAllSeries();
    chart()->addSeries(series_aec);

    series_aec->attachAxis(m_xaxis);
    series_aec->attachAxis(m_yaxis);

    m_xaxis->setLabelsVisible(true);
}
void CTAECPlot::updateImageData(std::shared_ptr<DataContainer> d)
{
    if (m_data && d) {
        if (m_data->ID() == d->ID() && !d->hasImage(DataContainer::ImageType::Density))
            return;
    }
    m_data = d;
    updatePlot();
}