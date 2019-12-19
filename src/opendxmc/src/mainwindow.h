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

Copyright 2019 Erlend Andersen
*/
#pragma once

#include <QMainWindow>
#include <QThread>
#include <QTabWidget>
#include <QTimer>

#include "imageimportpipeline.h"
#include "simulationpipeline.h"
#include "binaryimportpipeline.h"
#include "progressbar.h"
#include "saveload.h"




class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	void setEnableEditing(void);
	void setDisableEditing(void);
	void setProgressBar(ProgressBar* progressBar);
	void updateProgressBar();

protected:
	void createMenu();
	void saveFileAction();
	void loadFileAction();
signals:
	void requestOpenSaveFile(const QString& path);
	void requestSaveToFile(const QString& path);
private:
	QThread m_workerThread;
	ImageImportPipeline *m_importPipeline = nullptr;
	SimulationPipeline* m_simulationPipeline = nullptr;
	BinaryImportPipeline* m_binaryImportPipeline = nullptr;
	QTabWidget *m_menuWidget = nullptr;

	SaveLoad* m_saveLoad = nullptr;

	ProgressBar* m_progressBar = nullptr;
	QTimer* m_progressTimer = nullptr;
};