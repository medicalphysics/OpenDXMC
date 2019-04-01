#pragma once

#include <QMainWindow>
#include <QThread>
#include <QTabWidget>

#include "imageimportpipeline.h"
#include "simulationpipeline.h"

class MainWindow : public QMainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget* parent = nullptr);
	~MainWindow();
	void setEnableEditing(void);
	void setDisableEditing(void);
private:
	QThread m_workerThread;
	ImageImportPipeline *m_importPipeline;
	SimulationPipeline* m_simulationPipeline;
	QTabWidget *m_menuWidget;
};