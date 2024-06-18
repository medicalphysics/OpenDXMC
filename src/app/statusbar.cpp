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

#include <statusbar.hpp>

StatusBar::StatusBar(QWidget* parent)
    : QStatusBar(parent)
{

    m_bar = new QProgressBar;
    addWidget(m_bar);
    m_bar->setRange(0, 0);
    m_bar->hide();
}

void StatusBar::processingStarted(ProgressWorkType type)
{
    if (!m_currentProcessing.contains(type)) {
        m_currentProcessing[type] = 0;
    }
    m_currentProcessing.at(type)++;
    m_numberOfJobs++;

    updateInfoText();
    if (m_numberOfJobs != 0)
        m_bar->show();
}

void StatusBar::processingFinished(ProgressWorkType type)
{
    if (!m_currentProcessing.contains(type)) {
        m_currentProcessing[type] = 0;
    } else {
        m_currentProcessing.at(type)--;
    }
    m_numberOfJobs--;

    updateInfoText();
    if (m_numberOfJobs == 0)
        m_bar->hide();
}

void StatusBar::registerPipeline(BasePipeline* pipeline)
{
    connect(pipeline, &BasePipeline::dataProcessingStarted, this, &StatusBar::processingStarted);
    connect(pipeline, &BasePipeline::dataProcessingFinished, this, &StatusBar::processingFinished);
}

void StatusBar::updateInfoText()
{
    QString txt(tr("Working"));
    m_bar->setFormat(txt);
}
