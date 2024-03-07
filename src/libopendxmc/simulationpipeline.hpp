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

#include <basepipeline.hpp>
#include <dxmc_specialization.hpp>
#include "dxmc/transportprogress.hpp"


#include <QString>



class BeamActorContainer;
class QTimerEvent;

class SimulationPipeline : public BasePipeline {
    Q_OBJECT
public:
    SimulationPipeline(QObject* parent = nullptr);
    ~SimulationPipeline();
    void updateImageData(std::shared_ptr<DataContainer>) override;
    void addBeamActor(std::shared_ptr<BeamActorContainer> actor);
    void removeBeamActor(std::shared_ptr<BeamActorContainer> actor);    
    void setNumberOfThreads(int nthreads);
    void setDeleteAirDose(bool on) { m_deleteAirDose = on; };
    void timerEvent(QTimerEvent*);
    void setLowEnergyCorrectionLevel(int level) { m_lowenergyCorrection = level; }
    void startSimulation();
    void stopSimulation();

signals:
    void simulationReady(bool on);
    void simulationRunning(bool running);
    void simulationProgress(QString, int);

protected:
    bool testIfReadyForSimulation(bool test_image = true) const;
    void finishingSimulation();


private:
    std::shared_ptr<DataContainer> m_data = nullptr;
    std::vector<std::shared_ptr<Beam>> m_beams;
    int m_threads = 0;
    int m_lowenergyCorrection = 1;
    bool m_deleteAirDose = true;
    int m_timerID = 0;
    dxmc::TransportProgress m_progress;
};