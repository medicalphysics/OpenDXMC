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

#pragma once

#include <beamactorcontainer.hpp>
#include <datacontainer.hpp>
#include <dxmc_specialization.hpp>

#include <QMap>
#include <QStandardItemModel>
#include <QString>

#include <memory>

class BeamSettingsModel : public QStandardItemModel {
    Q_OBJECT
public:
    BeamSettingsModel(QObject* parent = nullptr);
    void addDXBeam();
    void addCTSpiralBeam();
    void addCTSpiralDualEnergyBeam();
    void updateImageData(std::shared_ptr<DataContainer>);
    void deleteBeam(int index);

signals:
    void beamActorAdded(std::shared_ptr<BeamActorContainer>);
    void beamActorRemoved(std::shared_ptr<BeamActorContainer>);
    void requestRender();

private:
    std::vector<std::pair<std::shared_ptr<Beam>, std::shared_ptr<BeamActorContainer>>> m_beams;
    std::shared_ptr<DataContainer> m_image = nullptr;
    QMap<QString, BowtieFilter> m_bowtieFilters;
};
