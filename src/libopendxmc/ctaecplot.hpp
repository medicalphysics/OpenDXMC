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

#pragma once

#include <datacontainer.hpp>

#include <QChartView>
#include <QValueAxis>

class CTAECPlot : public QChartView {
    Q_OBJECT
public:
    CTAECPlot(QWidget* parent = nullptr);

    void updateImageData(std::shared_ptr<DataContainer>);

protected:
    void updatePlot();

private:
    std::shared_ptr<DataContainer> m_data = nullptr;
    QValueAxis* m_xaxis = nullptr;
    QValueAxis* m_yaxis = nullptr;
};