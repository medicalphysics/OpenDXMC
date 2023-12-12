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

#include <beamsettingsmodel.hpp>

#include <QThread>
#include <QTreeView>

class BeamSettingsView : public QTreeView {
    Q_OBJECT
public:
    BeamSettingsView(QWidget* parent = nullptr);
    ~BeamSettingsView();
    void updateImageData(std::shared_ptr<DataContainer> data) { emit imageDataChanged(data); };
    void addDXBeam() { emit requestAddDXBeam(); }
    void addCTSpiralBeam() { emit requestAddCTSpiralBeam(); }
    void addCTSpiralDualEnergyBeam() { emit requestAddCTSpiralDualEnergyBeam(); }
    void keyPressEvent(QKeyEvent*) override;
signals:
    void requestAddDXBeam();
    void requestAddCTSpiralBeam();
    void requestAddCTSpiralDualEnergyBeam();
    void beamActorAdded(std::shared_ptr<BeamActorContainer>);
    void beamActorRemoved(std::shared_ptr<BeamActorContainer>);
    void requestRender();
    void imageDataChanged(std::shared_ptr<DataContainer> data);
    void requestDeleteBeamIndex(int);

private:
    QThread m_workerThread;
    BeamSettingsModel* m_model = nullptr;
};