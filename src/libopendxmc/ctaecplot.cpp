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
    {
        auto series_aec = new QLineSeries(this);
        const auto& aec = m_data->aecData();
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

        chart()->removeAllSeries();
        chart()->addSeries(series_aec);

        series_aec->attachAxis(m_xaxis);
        series_aec->attachAxis(m_yaxis);

        m_xaxis->setLabelsVisible(true);
    }
    std::vector<CTAECFilter> f;
    f.push_back(m_data->calculateAECfilterFromWaterEquivalentDiameter(false));
    f.push_back(m_data->calculateAECfilterFromWaterEquivalentDiameter(true));

    for (const auto& filt : f) {

        auto series_f = new QLineSeries(this);

        const auto length = filt.length();
        const auto& weights = filt.weights();
        const auto step = length / (weights.size() - 1);
        QList<QPointF> filt_qt(weights.size());
        for (std::size_t i = 0; i < weights.size(); ++i) {
            filt_qt[i].setX(i * step - length / 2);
            filt_qt[i].setY(weights[i]);
        }
        series_f->append(filt_qt);
        chart()->addSeries(series_f);
        series_f->attachAxis(m_xaxis);
        series_f->attachAxis(m_yaxis);
    }
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