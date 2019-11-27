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

#include <QSplitter>
#include <QStatusBar>
#include <QAction>
#include <QMenuBar>

#include "mainwindow.h"
#include "viewportwidget.h"
#include "dicomimportwidget.h"
#include "imageimportpipeline.h"
#include "progressindicator.h"
#include "exportwidget.h"
#include "sourceeditorwidget.h"
#include "phantomselectionwidget.h"
#include "dosereportwidget.h"
#include "binaryimportwidget.h"

Q_DECLARE_METATYPE(std::vector<std::shared_ptr<Source>>)

MainWindow::MainWindow(QWidget* parent) 
	: QMainWindow(parent)
{
	qRegisterMetaType<std::vector<std::shared_ptr<Source>>>();

	//image import pipeline
	m_importPipeline = new ImageImportPipeline();
	m_importPipeline->moveToThread(&m_workerThread);
	connect(m_importPipeline, &ImageImportPipeline::processingDataStarted, this, &MainWindow::setDisableEditing);
	connect(m_importPipeline, &ImageImportPipeline::processingDataEnded, this, &MainWindow::setEnableEditing);
	//simulation pipeline
	m_simulationPipeline = new SimulationPipeline();
	m_simulationPipeline->moveToThread(&m_workerThread);
	connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, m_simulationPipeline, &SimulationPipeline::setImageData);
	connect(m_importPipeline, &ImageImportPipeline::materialDataChanged, m_simulationPipeline, &SimulationPipeline::setMaterials);
	connect(m_importPipeline, &ImageImportPipeline::organDataChanged, m_simulationPipeline, &SimulationPipeline::setOrganList);
	//connections to disable widgets when simulationpipeline is working
	connect(m_simulationPipeline, &SimulationPipeline::processingDataStarted, this, &MainWindow::setDisableEditing);
	connect(m_simulationPipeline, &SimulationPipeline::processingDataEnded, this, &MainWindow::setEnableEditing);
	//binary import pipeline
	m_binaryImportPipeline = new BinaryImportPipeline();
	m_binaryImportPipeline->moveToThread(&m_workerThread);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::processingDataStarted, this, &MainWindow::setDisableEditing);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::processingDataEnded, this, &MainWindow::setEnableEditing);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, m_simulationPipeline, &SimulationPipeline::setImageData);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::materialDataChanged, m_simulationPipeline, &SimulationPipeline::setMaterials);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::organDataChanged, m_simulationPipeline, &SimulationPipeline::setOrganList);



	//statusbar and progress indicator widget
	auto statusBar = this->statusBar();
	auto progressIndicator = new ProgressIndicator(this);
	connect(m_importPipeline, &ImageImportPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
	connect(m_importPipeline, &ImageImportPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
	connect(m_simulationPipeline, &SimulationPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
	connect(m_simulationPipeline, &SimulationPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
	statusBar->addPermanentWidget(progressIndicator);



	QSplitter* splitter = new QSplitter(Qt::Horizontal);

	m_menuWidget = new QTabWidget(this);
	m_menuWidget->setTabPosition(QTabWidget::West);
	
	
	//import widgets share å tabbed widget
	auto importWidget = new QTabWidget(this);
	importWidget->setTabPosition(QTabWidget::North);

	//dicom import widget
	DicomImportWidget *dicomImportWidget = new DicomImportWidget(this);
	importWidget->addTab(dicomImportWidget, tr("DiCOM CT images"));
	connect(dicomImportWidget, &DicomImportWidget::dicomSeriesActivated, m_importPipeline, &ImageImportPipeline::setDicomData);
	connect(dicomImportWidget, &DicomImportWidget::outputSpacingChanged, m_importPipeline, &ImageImportPipeline::setOutputSpacing);
	connect(dicomImportWidget, &DicomImportWidget::useOutputSpacingChanged, m_importPipeline, &ImageImportPipeline::setUseOutputSpacing);
	connect(dicomImportWidget, &DicomImportWidget::blurRadiusChanged, m_importPipeline, &ImageImportPipeline::setBlurRadius);
	connect(dicomImportWidget, &DicomImportWidget::aqusitionVoltageChanged, m_importPipeline, &ImageImportPipeline::setCTImportAqusitionVoltage);
	connect(dicomImportWidget, &DicomImportWidget::aqusitionAlFiltrationChanged, m_importPipeline, &ImageImportPipeline::setCTImportAqusitionAlFiltration);
	connect(dicomImportWidget, &DicomImportWidget::aqusitionCuFiltrationChanged, m_importPipeline, &ImageImportPipeline::setCTImportAqusitionCuFiltration);
	connect(dicomImportWidget, &DicomImportWidget::segmentationMaterialsChanged, m_importPipeline, &ImageImportPipeline::setCTImportMaterialMap);
	

	//phantom import widget
	PhantomSelectionWidget* phantomWidget = new PhantomSelectionWidget(this);
	importWidget->addTab(phantomWidget, tr("Digital phantoms"));
	connect(phantomWidget, &PhantomSelectionWidget::readIRCUFemalePhantom, m_importPipeline, &ImageImportPipeline::importICRUFemalePhantom);
	connect(phantomWidget, &PhantomSelectionWidget::readIRCUMalePhantom, m_importPipeline, &ImageImportPipeline::importICRUMalePhantom);
	connect(phantomWidget, &PhantomSelectionWidget::readCTDIPhantom, m_importPipeline, &ImageImportPipeline::importCTDIPhantom);
	connect(phantomWidget, &PhantomSelectionWidget::readAWSPhantom, m_importPipeline, &ImageImportPipeline::importAWSPhantom);

	//binary import widget
	BinaryImportWidget* binaryWidget = new BinaryImportWidget(this);
	importWidget->addTab(binaryWidget, tr("Binary files"));
	connect(binaryWidget, &BinaryImportWidget::dimensionChanged, m_binaryImportPipeline, QOverload<int, int>::of(&BinaryImportPipeline::setDimension));
	connect(binaryWidget, &BinaryImportWidget::spacingChanged, m_binaryImportPipeline, QOverload<int, double>::of(&BinaryImportPipeline::setSpacing));
	connect(binaryWidget, &BinaryImportWidget::materialArrayPathChanged, m_binaryImportPipeline, &BinaryImportPipeline::setMaterialArrayPath);
	connect(binaryWidget, &BinaryImportWidget::densityArrayPathChanged, m_binaryImportPipeline, &BinaryImportPipeline::setDensityArrayPath);
	connect(binaryWidget, &BinaryImportWidget::materialMapPathChanged, m_binaryImportPipeline, &BinaryImportPipeline::setMaterialMapPath);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::errorMessage, binaryWidget, &BinaryImportWidget::setErrorMessage);
	m_menuWidget->addTab(importWidget, tr("Import data"));

	//source edit widget
	auto sourceEditWidget = new SourceEditWidget(this);
	m_menuWidget->addTab(sourceEditWidget, tr("X-ray sources"));
	auto sourceEditDelegate = sourceEditWidget->delegate();
	connect(m_importPipeline, &ImageImportPipeline::aecFilterChanged, sourceEditDelegate, &SourceDelegate::addAecFilter);
	connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, sourceEditWidget->model(), &SourceModel::setImageData);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, sourceEditWidget->model(), &SourceModel::setImageData);

	//dosereportWidget
	auto doseReportWidget = new DoseReportWidget(this);
	connect(m_simulationPipeline, &SimulationPipeline::doseDataChanged, doseReportWidget, &DoseReportWidget::setDoseData);
	m_menuWidget->addTab(doseReportWidget, tr("Dose summary"));
	
	//export Widget
	auto exportWidget = new ExportWidget(this);
	connect(m_simulationPipeline, &SimulationPipeline::imageDataChanged, exportWidget, &ExportWidget::registerImage);
	connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, exportWidget, &ExportWidget::registerImage);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, exportWidget, &ExportWidget::registerImage);
	m_menuWidget->addTab(exportWidget, tr("Export data"));
	splitter->addWidget(m_menuWidget);
	
	//simulation progress
	m_progressTimer = new QTimer(this);
	m_progressTimer->setTimerType(Qt::CoarseTimer);
	connect(m_simulationPipeline, &SimulationPipeline::progressBarChanged, this, &MainWindow::setProgressBar);
	connect(m_progressTimer, &QTimer::timeout, this, &MainWindow::updateProgressBar);

	//Viewport
	ViewPortWidget* viewPort = new ViewPortWidget(this);
	splitter->addWidget(viewPort);
	connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
	connect(m_simulationPipeline, &SimulationPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
	setCentralWidget(splitter);

	//setting up source 3d actor connection to viewpoert from sourceeditwidget
	auto model = sourceEditWidget->model();
	connect(model, &SourceModel::sourceAdded, viewPort, &ViewPortWidget::addActorContainer);
	connect(model, &SourceModel::actorsChanged, viewPort, &ViewPortWidget::render);
	connect(model, &SourceModel::sourceRemoved, viewPort, &ViewPortWidget::removeActorContainer);
	
	//request to run simulation connection
	connect(sourceEditWidget, &SourceEditWidget::runSimulation, m_simulationPipeline, &SimulationPipeline::runSimulation);


	//setting up saveload
	m_saveLoad = new SaveLoad(this);
	connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, m_saveLoad, &SaveLoad::setImageData);
	connect(m_simulationPipeline, &SimulationPipeline::imageDataChanged, m_saveLoad, &SaveLoad::setImageData);
	connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, m_saveLoad, &SaveLoad::setImageData);
	connect(m_saveLoad, &SaveLoad::imageDataChanged, m_simulationPipeline, &SimulationPipeline::setImageData);
	connect(m_saveLoad, &SaveLoad::imageDataChanged, exportWidget, &ExportWidget::registerImage);
	connect(m_saveLoad, &SaveLoad::imageDataChanged, sourceEditWidget->model(), &SourceModel::setImageData);

	connect(m_saveLoad, &SaveLoad::::processingDataStarted, this, &MainWindow::setDisableEditing);
	connect(m_saveLoad, &SaveLoad::::processingDataEnded, this, &MainWindow::setEnableEditing);
	connect(m_saveLoad, &SaveLoad::::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
	connect(m_saveLoad, &SaveLoad::::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);

	//no connections to pipeline after this point
	m_workerThread.start();

}

MainWindow::~MainWindow()
{
	m_workerThread.quit();
	m_workerThread.wait();
	delete m_importPipeline;
	m_importPipeline = nullptr;
	delete m_simulationPipeline;
	m_simulationPipeline = nullptr;
}

void MainWindow::setEnableEditing(void)
{
	for (int i = 0; i < m_menuWidget->count(); ++i)
	{
		auto wid = m_menuWidget->widget(i);
		wid->setEnabled(true);
	}
}

void MainWindow::setDisableEditing(void)
{
	for (int i = 0; i < m_menuWidget->count(); ++i)
	{
		auto wid = m_menuWidget->widget(i);
		wid->setDisabled(true);
	}
}

void MainWindow::setProgressBar(std::shared_ptr<ProgressBar> progressBar)
{
	m_progressBar = progressBar;
	m_progressTimer->start(5000);
}

void MainWindow::updateProgressBar()
{
	if (m_progressBar)
	{
		auto msg = m_progressBar->getETA();
		this->statusBar()->showMessage(QString::fromStdString(msg), 5000);
	}
	else {
		m_progressTimer->stop();
	}
}
