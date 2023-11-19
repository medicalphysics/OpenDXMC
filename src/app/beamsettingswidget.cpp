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

Copyright 2024 Erlend Andersen
*/

#include <beamsettingswidget.hpp>

#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>

BeamSettingsWidget::BeamSettingsWidget(QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);

    m_view = new BeamSettingsView(this);

    auto beamselectlayout = new QHBoxLayout;
    beamselectlayout->setContentsMargins(0, 0, 0, 0);
    auto beamselectcombo = new QComboBox(this);
    beamselectlayout->addWidget(beamselectcombo);
    beamselectcombo->addItem(tr("DX Beam"));
    beamselectcombo->addItem(tr("CT Spiral Beam"));
    beamselectcombo->addItem(tr("CT Dual Source Beam"));
    auto beamaddbutton = new QPushButton(tr("Add"), this);
    beamselectlayout->addWidget(beamaddbutton);
    layout->addLayout(beamselectlayout);
    connect(beamaddbutton, &QPushButton::clicked, [=](void) {
        auto idx = beamselectcombo->currentIndex();
        if (idx == 0)
            m_view->addDXBeam();
        else if (idx == 1)
            m_view->addCTSpiralBeam();
        else if (idx == 2)
            m_view->addCTSpiralDualEnergyBeam();
    });

    layout->addWidget(m_view);

    m_aecplot = new CTAECPlot(this);
    layout->addWidget(m_aecplot);
}

void BeamSettingsWidget::updateImageData(std::shared_ptr<DataContainer> data)
{
    m_view->updateImageData(data);
    m_aecplot->updateImageData(data);
}