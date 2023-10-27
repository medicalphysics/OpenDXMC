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

#include <QAction>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>

#include <ctdicomimportwidget.hpp>
#include <ctimageimportpipeline.hpp>
#include <slicerenderwidget.hpp>

#include <mainwindow.hpp>

#include <vector>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    setCentralWidget(splitter);
    splitter->setOpaqueResize(false);

    auto menuWidget = new QTabWidget(splitter);
    splitter->addWidget(menuWidget);
    menuWidget->setTabPosition(QTabWidget::West);


    // import widgets share a tabbed widget
    auto importWidgets = new QTabWidget(this);
    menuWidget->addTab(importWidgets, tr("Import data"));
    importWidgets->setTabPosition(QTabWidget::North);

    //vector for QObjects in pipeline
    std::vector<QObject*> pipelineitems;


    // adding ct dicom import widget and pipeline
    auto ctimageimportpipeline = new CTImageImportPipeline();
    ctimageimportpipeline->moveToThread(&m_workerThread);
    pipelineitems.push_back(ctimageimportpipeline);
    auto ctdicomimportwidget = new CTDicomImportWidget(importWidgets);
    importWidgets->addTab(ctdicomimportwidget, tr("CT DiCOM import"));
    //setting up signals for ct image import
    connect(ctdicomimportwidget, &CTDicomImportWidget::dicomSeriesActivated, ctimageimportpipeline, &CTImageImportPipeline::readImages);
    connect(ctdicomimportwidget, &CTDicomImportWidget::blurRadiusChanged, ctimageimportpipeline, &CTImageImportPipeline::setBlurRadius);
    connect(ctdicomimportwidget, &CTDicomImportWidget::outputSpacingChanged, ctimageimportpipeline, &CTImageImportPipeline::setOutputSpacing);
    connect(ctdicomimportwidget, &CTDicomImportWidget::useOutputSpacingChanged, ctimageimportpipeline, &CTImageImportPipeline::setUseOutputSpacing);



    // adding an slice render widget
    // TODO add multiple views
    auto slicerender = new SliceRenderWidget(splitter);
    splitter->addWidget(slicerender);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, slicerender, &SliceRenderWidget::updateImageData);

    // simulation progress
    /* m_progressTimer = new QTimer(this);
    m_progressTimer->setTimerType(Qt::CoarseTimer);
    connect(m_simulationPipeline, &SimulationPipeline::progressBarChanged, this, &MainWindow::setProgressBar);
    connect(m_progressTimer, &QTimer::timeout, this, &MainWindow::updateProgressBar);
    */

    // Viewport
    /* ViewPortWidget* viewPort = new ViewPortWidget(this);
    connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    connect(m_simulationPipeline, &SimulationPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    */

    // setting up source 3d actor connection to viewpoert from sourceeditwidget
    /* auto sourceModel = sourceEditWidget->model();
    connect(sourceModel, &SourceModel::sourceActorAdded, viewPort, &ViewPortWidget::addActorContainer);
    connect(sourceModel, &SourceModel::actorsChanged, viewPort, &ViewPortWidget::render);
    connect(sourceModel, &SourceModel::sourceActorRemoved, viewPort, &ViewPortWidget::removeActorContainer);

    //request to run simulation connection
    connect(sourceEditWidget, &SourceEditWidget::runSimulation, m_simulationPipeline, &SimulationPipeline::runSimulation);
    connect(sourceEditWidget, &SourceEditWidget::lowEnergyCorrectionChanged, m_simulationPipeline, &SimulationPipeline::setLowEnergyCorrection);


    //setting up saveload
    m_saveLoad = new SaveLoad();
    m_saveLoad->moveToThread(&m_workerThread);
    connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, m_saveLoad, &SaveLoad::setImageData);
    connect(m_importPipeline, &ImageImportPipeline::materialDataChanged, m_saveLoad, &SaveLoad::setMaterials);
    connect(m_importPipeline, &ImageImportPipeline::organDataChanged, m_saveLoad, &SaveLoad::setOrganList);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::imageDataChanged, m_saveLoad, &SaveLoad::setImageData);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::materialDataChanged, m_saveLoad, &SaveLoad::setMaterials);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::organDataChanged, m_saveLoad, &SaveLoad::setOrganList);
    connect(m_simulationPipeline, &SimulationPipeline::imageDataChanged, m_saveLoad, &SaveLoad::setImageData);
    connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, m_saveLoad, &SaveLoad::setImageData);
    connect(m_binaryImportPipeline, &BinaryImportPipeline::materialDataChanged, m_saveLoad, &SaveLoad::setMaterials);
    connect(m_binaryImportPipeline, &BinaryImportPipeline::organDataChanged, m_saveLoad, &SaveLoad::setOrganList);

    connect(m_saveLoad, &SaveLoad::imageDataChanged, m_simulationPipeline, &SimulationPipeline::setImageData);
    connect(m_saveLoad, &SaveLoad::imageDataChanged, exportWidget, &ExportWidget::registerImage);
    connect(m_saveLoad, &SaveLoad::imageDataChanged, sourceEditWidget->model(), &SourceModel::setImageData);
    connect(m_saveLoad, &SaveLoad::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    connect(m_saveLoad, &SaveLoad::materialDataChanged, m_simulationPipeline, &SimulationPipeline::setMaterials);
    connect(m_saveLoad, &SaveLoad::organDataChanged, m_simulationPipeline, &SimulationPipeline::setOrganList);
    connect(m_saveLoad, &SaveLoad::doseDataChanged, doseReportWidget, &DoseReportWidget::setDoseData);

    connect(m_saveLoad, &SaveLoad::processingDataStarted, this, &MainWindow::setDisableEditing);
    connect(m_saveLoad, &SaveLoad::processingDataEnded, this, &MainWindow::setEnableEditing);
    connect(m_saveLoad, &SaveLoad::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
    connect(m_saveLoad, &SaveLoad::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);

    connect(sourceModel, &SourceModel::sourceAdded, m_saveLoad, &SaveLoad::addSource);
    connect(sourceModel, &SourceModel::sourceRemoved, m_saveLoad, &SaveLoad::removeSource);
    connect(m_saveLoad, &SaveLoad::sourcesChanged, sourceModel, &SourceModel::setSources);
    connect(m_saveLoad, &SaveLoad::aecFilterChanged, sourceEditDelegate, &SourceDelegate::addAecFilter);
    connect(m_saveLoad, &SaveLoad::bowtieFilterChanged, sourceEditDelegate, &SourceDelegate::addBowtieFilter);

    //dose progress image widget
    m_progressWidget = new ProgressWidget(this);
    */
    // setting up layout

    // setting up window menu
    createMenu();

    // no connections to pipeline after this point
    m_workerThread.start();
}

MainWindow::~MainWindow()
{
    m_workerThread.quit();
    m_workerThread.wait();

    /*
    if (m_progressBar) {
        m_progressBar->setCancel(true);
    }

    m_workerThread.quit();
    m_workerThread.wait();
    delete m_importPipeline;
    m_importPipeline = nullptr;
    delete m_simulationPipeline;
    m_simulationPipeline = nullptr;
    delete m_saveLoad;
    m_saveLoad = nullptr;
    delete m_binaryImportPipeline;
    m_binaryImportPipeline = nullptr;
    delete m_phantomImportPipeline;
    m_phantomImportPipeline = nullptr;
    */
}

void MainWindow::createMenu()
{
    auto fileMenu = menuBar()->addMenu(tr("&File"));

    /* auto saveAction = new QAction(tr("Save as"), this);
    saveAction->setShortcut(QKeySequence::SaveAs);
    saveAction->setStatusTip(tr("Save current simulation as"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFileAction);
    fileMenu->addAction(saveAction);
    connect(this, &MainWindow::requestSaveToFile, m_saveLoad, &SaveLoad::saveToFile);

    auto openAction = new QAction(tr("Open"), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(tr("Open a previously saved simulation"));
    connect(openAction, &QAction::triggered, this, &MainWindow::loadFileAction);
    fileMenu->addAction(openAction);
    connect(this, &MainWindow::requestOpenSaveFile, m_saveLoad, &SaveLoad::loadFromFile);
    */
}
