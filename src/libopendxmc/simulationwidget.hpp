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

#include <QPushButton>
#include <QWidget>
#include <QProgressBar>

#include <vector>

class SimulationWidget : public QWidget {
    Q_OBJECT
public:
    SimulationWidget(QWidget* parent = nullptr);
    void setSimulationReady(bool on);
    void setSimulationRunning(bool on);
    void updateSimulationProgress(QString, int, int, int);
signals:
    void numberOfThreadsChanged(int);
    void lowEnergyCorrectionMethodChanged(int);
    void requestStartSimulation();
    void requestStopSimulation();
    void ignoreAirChanged(bool);

private:
    bool m_simulation_ready = false;
    QPushButton* m_start_simulation_button = nullptr;
    QPushButton* m_stop_simulation_button = nullptr;
    std::vector<QWidget*> m_items;
    std::vector<QProgressBar*> m_progress_bars;
};