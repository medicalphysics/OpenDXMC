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
#include <QValueAxis>

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

    auto axisx = new QValueAxis(chart());
    axisx->setMinorGridLineVisible(false);
    axisx->setGridLineVisible(false);
    axisx->setTickCount(2);
    axisx->setLabelsVisible(false);
    axisx->setRange(-.1, 1.1);
    chart()->setAxisX(axisx);
    auto axisy = new QValueAxis(chart());
    axisy->setGridLineVisible(false);
    axisy->setRange(-.1, 1.1);
    chart()->setAxisY(axisy);
    axisy->setLabelsVisible(false);
    axisy->setMinorGridLineVisible(false);
    axisy->setTickCount(2);
    axisy->setLabelsVisible(false);
}
void CTAECPlot::updatePlot()
{
    if (!m_data)
        return;
    const auto& arr = m_data->hasImage(DataContainer::ImageType::CT) ? m_data->getCTArray() : m_data->getDensityArray();

    const auto& s = m_data->spacing();
    const auto& d = m_data->dimensions();
    if (d[2] < 2)
        return;
    const auto offset = d[0] * d[1];

    QList<QPointF> im_qt(d[2]);

    auto max = std::numeric_limits<double>::lowest();
    for (std::size_t i = 0; i < d[2]; ++i) {
        im_qt[i].setY(std::reduce(arr.cbegin() + offset * i, arr.cbegin() + offset * (i + 1)));
        max = std::max(max, im_qt[i].y());
    }

    const auto step = 1.0 / (d[2] - 1);
    for (std::size_t i = 0; i < d[2]; ++i) {
        auto& p = im_qt[i];
        p.setY(p.y() / max);
        p.setX(step * i);
    }

    auto series = new QLineSeries(this);
    series->append(im_qt);
    for (auto ax : chart()->axes())
        series->attachAxis(ax);

    chart()->removeAllSeries();
    chart()->addSeries(series);
    show();
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