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

#include <beamsettingsview.hpp>

BeamSettingsView::BeamSettingsView(QWidget* parent)
    : QTreeView(parent)
{
    m_model = new BeamSettingsModel();
    m_model->moveToThread(&m_workerThread);
    setModel(m_model);

    connect(this, &BeamSettingsView::requestAddDXBeam, m_model, &BeamSettingsModel::addDXBeam);
    connect(this, &BeamSettingsView::requestAddCTSpiralBeam, m_model, &BeamSettingsModel::addCTSpiralBeam);
    connect(this, &BeamSettingsView::requestAddCTSpiralDualEnergyBeam, m_model, &BeamSettingsModel::addCTSpiralDualEnergyBeam);

    connect(m_model, &BeamSettingsModel::beamActorAdded, [=](auto v) { emit this->beamActorAdded(v); });
    connect(m_model, &BeamSettingsModel::beamActorRemoved, [=](auto v) { emit this->beamActorRemoved(v); });
    connect(m_model, &BeamSettingsModel::requestRender, [=](void) { emit this->requestRender(); });

    connect(this, &BeamSettingsView::imageDataChanged, m_model, &BeamSettingsModel::updateImageData);

    m_workerThread.start();
}

BeamSettingsView::~BeamSettingsView()
{
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_model;
    m_model = nullptr;
}