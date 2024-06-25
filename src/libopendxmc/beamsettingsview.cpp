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

#include <beamsettingsdelegate.hpp>
#include <beamsettingsview.hpp>

#include <QKeyEvent>

BeamSettingsView::BeamSettingsView(QWidget* parent)
    : QTreeView(parent)
{
    m_model = new BeamSettingsModel();
    m_model->moveToThread(&m_workerThread);
    setModel(m_model);

    connect(this, &BeamSettingsView::requestAddDXBeam, m_model, &BeamSettingsModel::addDXBeam);
    connect(this, &BeamSettingsView::requestAddCBCTBeam, m_model, &BeamSettingsModel::addCBCTBeam);
    connect(this, &BeamSettingsView::requestAddCTSpiralBeam, m_model, &BeamSettingsModel::addCTSpiralBeam);
    connect(this, &BeamSettingsView::requestAddCTSequentialBeam, m_model, &BeamSettingsModel::addCTSequentialBeam);
    connect(this, &BeamSettingsView::requestAddCTSpiralDualEnergyBeam, m_model, &BeamSettingsModel::addCTSpiralDualEnergyBeam);
    connect(this, &BeamSettingsView::requestAddPencilBeam, m_model, &BeamSettingsModel::addPencilBeam);
    connect(this, &BeamSettingsView::requestAddNewBeam, m_model, &BeamSettingsModel::addBeam);

    connect(m_model, &BeamSettingsModel::beamActorAdded, [this](auto v) { emit this->beamActorAdded(v); });
    connect(m_model, &BeamSettingsModel::beamActorRemoved, [this](auto v) { emit this->beamActorRemoved(v); });
    connect(m_model, &BeamSettingsModel::requestRender, [this](void) { emit this->requestRender(); });

    connect(this, &BeamSettingsView::imageDataChanged, m_model, &BeamSettingsModel::updateImageData);
    connect(this, &BeamSettingsView::requestDeleteBeamIndex, m_model, &BeamSettingsModel::deleteBeam);

    auto itemdelegate = new BeamSettingsDelegate(m_model, this);
    setItemDelegate(itemdelegate);
    m_workerThread.start();
}

BeamSettingsView::~BeamSettingsView()
{
    m_workerThread.quit();
    m_workerThread.wait();
    delete m_model;
    m_model = nullptr;
}

void BeamSettingsView::keyPressEvent(QKeyEvent* ev)
{
    if (ev->key() == Qt::Key::Key_Delete) {
        auto cIdx = currentIndex();
        auto cItem = m_model->itemFromIndex(cIdx);
        if (cItem) {
            if (cItem->parent() == nullptr) {
                // we delete the node since it's top level
                emit requestDeleteBeamIndex(cItem->row());
            }
        }
    }

    QTreeView::keyPressEvent(ev);
}
