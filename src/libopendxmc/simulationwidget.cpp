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

#include <simulationwidget.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>

#include <thread>
#include <utility>

template <typename T>
std::pair<T*, QGroupBox*> createWidget(const QString& title, const QString& txt, QWidget* parent = nullptr)
{

    auto box = new QGroupBox(title, parent);
    auto layout = new QHBoxLayout;
    box->setLayout(layout);

    auto label = new QLabel(txt, box);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto w = new T;
    w->setParent(box);
    layout->addWidget(w);

    return std::make_pair(w, box);
}

SimulationWidget::SimulationWidget(QWidget* parent)
    : QWidget(parent)
{
    auto layout = new QVBoxLayout;
    this->setLayout(layout);

    const auto n_threads = std::max(1, static_cast<int>(std::thread::hardware_concurrency()));
    auto threads_txt = tr("Select number of threads for simulations, setting this to 0 uses ");
    threads_txt += QString::number(n_threads) + tr(" threads.");
    // threads widget
    auto [threads_spin, threads_box] = createWidget<QSpinBox>(tr("Number of threads"), threads_txt, this);
    threads_spin->setRange(0, n_threads * 2);
    threads_spin->setSuffix(tr(" threads"));
    threads_spin->setValue(0);
    connect(threads_spin, &QSpinBox::valueChanged, this, &SimulationWidget::numberOfThreadsChanged);
    layout->addWidget(threads_box);
    m_items.push_back(threads_box);

    // auto lec_txt = tr("Select bound electron correction method: None treats all electrons as free. Livermore applies atomic form factor and scatterfactor corrections to coherent and incoherent scattering. Impulse Approximation uses Harthree-Fock approximation to sample electron momentum for atomic shells for coherent scattering, in addition fluro photons are emitted for photoelectric effect.");
    auto lec_txt = tr("Select bound electron correction method");
    auto [lec_select, lec_box] = createWidget<QComboBox>(tr("Bound electron correction method"), lec_txt, this);
    lec_select->addItem(tr("None"));
    lec_select->addItem(tr("Livermore"));
    lec_select->addItem(tr("Impulse Approximation"));
    lec_select->setCurrentIndex(1);
    connect(lec_select, &QComboBox::currentIndexChanged, this, &SimulationWidget::lowEnergyCorrectionMethodChanged);
    layout->addWidget(lec_box);
    m_items.push_back(lec_box);

    auto air_txt = tr("Remove dose to air for easier visualization of dose. Photons are still transported through air media.");
    auto air_box = new QGroupBox(tr("Ignore air dose"), parent);
    air_box->setCheckable(true);
    auto air_layout = new QHBoxLayout;
    air_box->setLayout(air_layout);
    auto air_label = new QLabel(air_txt, air_box);
    air_label->setWordWrap(true);
    air_layout->addWidget(air_label);
    connect(air_box, &QGroupBox::toggled, this, &SimulationWidget::ignoreAirChanged);
    layout->addWidget(air_box);
    m_items.push_back(air_box);

    auto start_stop_box = new QGroupBox(tr("Start simulation"), this);
    auto start_stop_layout = new QHBoxLayout;
    start_stop_box->setLayout(start_stop_layout);
    m_start_simulation_button = new QPushButton(tr("Start"), start_stop_box);
    m_stop_simulation_button = new QPushButton(tr("Cancel"), start_stop_box);
    start_stop_layout->addWidget(m_start_simulation_button);
    start_stop_layout->addWidget(m_stop_simulation_button);
    connect(m_start_simulation_button, &QPushButton::clicked, this, &SimulationWidget::requestStartSimulation);
    connect(m_stop_simulation_button, &QPushButton::clicked, this, &SimulationWidget::requestStopSimulation);
    m_start_simulation_button->setEnabled(false);
    m_stop_simulation_button->setEnabled(false);
    layout->addWidget(start_stop_box);

    m_progress_bar = new QProgressBar(this);
    layout->addWidget(m_progress_bar);
    m_progress_bar->hide();

    layout->addStretch(100);
}

void SimulationWidget::setSimulationReady(bool on)
{
    m_simulation_ready = on;
    m_start_simulation_button->setDisabled(!m_simulation_ready);
}

void SimulationWidget::setSimulationRunning(bool on)
{
    for (auto& wid : m_items)
        wid->setDisabled(on);
    m_start_simulation_button->setDisabled(on);
    m_stop_simulation_button->setDisabled(!on);
    m_progress_bar->setVisible(on);
}

void SimulationWidget::updateSimulationProgress(QString message, int percent)
{    
    auto p = m_progress_bar;
    if (p->maximum() == 0) {
        p->setRange(0, 100);
    }
    p->setValue(percent);
    p->setFormat(message);
}