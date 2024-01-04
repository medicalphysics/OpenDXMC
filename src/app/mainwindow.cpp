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
#include <QDockWidget>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QTabWidget>

#include <beamsettingswidget.hpp>
#include <ctdicomimportwidget.hpp>
#include <ctimageimportpipeline.hpp>
#include <ctsegmentationpipeline.hpp>
#include <dosetablepipeline.hpp>
#include <dosetablewidget.hpp>
#include <h5io.hpp>
#include <icrpphantomimportpipeline.hpp>
#include <icrpphantomimportwidget.hpp>
#include <renderwidgetscollection.hpp>
#include <simulationpipeline.hpp>
#include <simulationwidget.hpp>
#include <volumerendersettingswidget.hpp>

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
    splitter->setStretchFactor(0, 1);
    menuWidget->setTabPosition(QTabWidget::West);
    menuWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

    // import widgets share a tabbed widget
    auto importWidgets = new QTabWidget(this);
    menuWidget->addTab(importWidgets, tr("Import data"));
    importWidgets->setTabPosition(QTabWidget::North);

    // vector for QObjects in pipeline
    std::vector<QObject*> pipelineitems;

    // adding an slice render widget
    auto slicerender = new RenderWidgetsCollection(splitter);
    splitter->addWidget(slicerender);
    splitter->setStretchFactor(1, 10);

    auto rightDock = new QDockWidget(this);
    rightDock->setAllowedAreas(Qt::DockWidgetArea::RightDockWidgetArea);
    auto dockWidget = new QWidget(rightDock);
    dockWidget->setContentsMargins(0, 0, 0, 0);
    auto dockWidget_layout = new QVBoxLayout;
    dockWidget->setLayout(dockWidget_layout);
    dockWidget_layout->addWidget(slicerender->createRendersettingsWidget(dockWidget));
    rightDock->setWidget(dockWidget);
    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, rightDock);

    // adding ct dicom import widget and pipeline
    auto ctimageimportpipeline = new CTImageImportPipeline;
    ctimageimportpipeline->moveToThread(&m_workerThread);
    pipelineitems.push_back(ctimageimportpipeline);
    auto ctdicomimportwidget = new CTDicomImportWidget(importWidgets);
    importWidgets->addTab(ctdicomimportwidget, tr("CT DiCOM import"));
    // setting up signals for ct image import
    connect(ctdicomimportwidget, &CTDicomImportWidget::dicomSeriesActivated, ctimageimportpipeline, &CTImageImportPipeline::readImages);
    connect(ctdicomimportwidget, &CTDicomImportWidget::blurRadiusChanged, ctimageimportpipeline, &CTImageImportPipeline::setBlurRadius);
    connect(ctdicomimportwidget, &CTDicomImportWidget::outputSpacingChanged, ctimageimportpipeline, &CTImageImportPipeline::setOutputSpacing);
    connect(ctdicomimportwidget, &CTDicomImportWidget::useOutputSpacingChanged, ctimageimportpipeline, &CTImageImportPipeline::setUseOutputSpacing);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);

    // adding ct segmentation pipeline
    auto ctsegmentationpipelie = new CTSegmentationPipeline;
    ctsegmentationpipelie->moveToThread(&m_workerThread);
    pipelineitems.push_back(ctsegmentationpipelie);
    connect(ctdicomimportwidget, &CTDicomImportWidget::aqusitionAlFiltrationChanged, ctsegmentationpipelie, &CTSegmentationPipeline::setAlFiltration);
    connect(ctdicomimportwidget, &CTDicomImportWidget::aqusitionSnFiltrationChanged, ctsegmentationpipelie, &CTSegmentationPipeline::setSnFiltration);
    connect(ctdicomimportwidget, &CTDicomImportWidget::aqusitionVoltageChanged, ctsegmentationpipelie, &CTSegmentationPipeline::setAqusitionVoltage);
    connect(ctsegmentationpipelie, &CTSegmentationPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, ctsegmentationpipelie, &CTSegmentationPipeline::updateImageData);

    // Adding icrp phantom import widget
    auto icrpimportwidget = new ICRPPhantomImportWidget(importWidgets);
    importWidgets->addTab(icrpimportwidget, tr("ICRP phantom import"));
    auto icrppipeline = new ICRPPhantomImportPipeline;
    icrppipeline->moveToThread(&m_workerThread);
    connect(icrpimportwidget, &ICRPPhantomImportWidget::setRemoveArms, icrppipeline, &ICRPPhantomImportPipeline::setRemoveArms);
    connect(icrpimportwidget, &ICRPPhantomImportWidget::requestImportPhantom, icrppipeline, &ICRPPhantomImportPipeline::importPhantom);
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);

    // beam settings widget
    auto beamsettingswidget = new BeamSettingsWidget(this);
    menuWidget->addTab(beamsettingswidget, tr("Configure X-ray beams"));
    connect(ctsegmentationpipelie, &CTSegmentationPipeline::imageDataChanged, beamsettingswidget, &BeamSettingsWidget::updateImageData);
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, beamsettingswidget, &BeamSettingsWidget::updateImageData);
    auto beamsettingsmodel = beamsettingswidget->modelView();
    connect(beamsettingsmodel, &BeamSettingsView::beamActorAdded, slicerender, &RenderWidgetsCollection::addActor);
    connect(beamsettingsmodel, &BeamSettingsView::beamActorRemoved, slicerender, &RenderWidgetsCollection::removeActor);
    connect(beamsettingsmodel, &BeamSettingsView::requestRender, slicerender, &RenderWidgetsCollection::Render);

    // simulationwidget
    auto simulationwidget = new SimulationWidget(this);
    menuWidget->addTab(simulationwidget, tr("Simulation"));

    // simulationpipeline
    auto simulationpipeline = new SimulationPipeline;
    simulationpipeline->moveToThread(&m_workerThread);
    connect(ctsegmentationpipelie, &CTSegmentationPipeline::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
    connect(simulationwidget, &SimulationWidget::numberOfThreadsChanged, simulationpipeline, &SimulationPipeline::setNumberOfThreads);
    connect(simulationwidget, &SimulationWidget::ignoreAirChanged, simulationpipeline, &SimulationPipeline::setDeleteAirDose);
    connect(simulationwidget, &SimulationWidget::requestStartSimulation, simulationpipeline, &SimulationPipeline::startSimulation);
    connect(simulationwidget, &SimulationWidget::requestStopSimulation, simulationpipeline, &SimulationPipeline::stopSimulation);
    connect(simulationpipeline, &SimulationPipeline::simulationReady, simulationwidget, &SimulationWidget::setSimulationReady);
    connect(beamsettingsmodel, &BeamSettingsView::beamActorAdded, simulationpipeline, &SimulationPipeline::addBeamActor);
    connect(beamsettingsmodel, &BeamSettingsView::beamActorRemoved, simulationpipeline, &SimulationPipeline::removeBeamActor);
    connect(simulationpipeline, &SimulationPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);
    connect(simulationpipeline, &SimulationPipeline::simulationRunning, beamsettingswidget, &BeamSettingsWidget::setDisabled);
    connect(simulationpipeline, &SimulationPipeline::simulationRunning, ctdicomimportwidget, &CTDicomImportWidget::setDisabled);
    connect(simulationpipeline, &SimulationPipeline::simulationRunning, icrpimportwidget, &ICRPPhantomImportWidget::setDisabled);
    connect(simulationpipeline, &SimulationPipeline::simulationRunning, simulationwidget, &SimulationWidget::setSimulationRunning);
    connect(simulationpipeline, &SimulationPipeline::simulationProgress, simulationwidget, &SimulationWidget::updateSimulationProgress);

    // dosetable
    auto dosetable = new DoseTableWidget(this);
    menuWidget->addTab(dosetable, tr("Organ Doses"));
    auto dosetablepipeline = new DoseTablePipeline;
    dosetablepipeline->moveToThread(&m_workerThread);
    connect(dosetablepipeline, &DoseTablePipeline::clearTable, dosetable, &DoseTableWidget::clearContents);
    connect(dosetablepipeline, &DoseTablePipeline::doseData, dosetable, &DoseTableWidget::setDoseData);
    connect(dosetablepipeline, &DoseTablePipeline::doseDataHeader, dosetable, &DoseTableWidget::setDoseDataHeader);
    connect(simulationpipeline, &SimulationPipeline::imageDataChanged, dosetablepipeline, &DoseTablePipeline::updateImageData);
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, dosetablepipeline, &DoseTablePipeline::updateImageData);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, dosetablepipeline, &DoseTablePipeline::updateImageData);

    // save load
    auto h5io = new H5IO;
    h5io->moveToThread(&m_workerThread);
    connect(ctsegmentationpipelie, &CTSegmentationPipeline::imageDataChanged, h5io, &H5IO::updateImageData);
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, h5io, &H5IO::updateImageData);
    connect(simulationpipeline, &SimulationPipeline::imageDataChanged, h5io, &H5IO::updateImageData);
    connect(beamsettingsmodel, &BeamSettingsView::beamActorAdded, h5io, &H5IO::addBeamActor);
    connect(beamsettingsmodel, &BeamSettingsView::beamActorRemoved, h5io, &H5IO::removeBeamActor);
    connect(this, &MainWindow::saveData, h5io, &H5IO::saveData);
    connect(this, &MainWindow::loadData, h5io, &H5IO::loadData);
    connect(h5io, &H5IO::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
    connect(h5io, &H5IO::imageDataChanged, dosetablepipeline, &DoseTablePipeline::updateImageData);
    connect(h5io, &H5IO::imageDataChanged, beamsettingswidget, &BeamSettingsWidget::updateImageData);
    connect(h5io, &H5IO::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);
    connect(h5io, &H5IO::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
    connect(h5io, &H5IO::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);

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

    // we should cleanup following objects but we let the OS handle this for us now.
    // ctimageimportpipeline
    // ctsegmentationpipelie
    // simulationpipeline
    // ICRPPhantomImportPipeline
}

void MainWindow::createMenu()
{
    auto fileMenu = menuBar()->addMenu(tr("&File"));

    auto saveAction = new QAction(tr("Save as"), this);
    saveAction->setShortcut(QKeySequence::SaveAs);
    saveAction->setStatusTip(tr("Save current simulation"));
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFileAction);
    fileMenu->addAction(saveAction);

    auto openAction = new QAction(tr("Open"), this);
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(tr("Open a previously saved simulation"));
    connect(openAction, &QAction::triggered, this, &MainWindow::loadFileAction);
    fileMenu->addAction(openAction);
}

QString directoryPath(const QString& path)
{
    QFileInfo info(path);
    if (info.isFile()) {
        auto dirpath = info.absolutePath();
        return dirpath;
    }
    return path;
}
QString filePath(const QString& directory, const QString& filename)
{
    auto dir = directoryPath(directory);
    QDir d(dir);
    auto s = d.absoluteFilePath(filename);
    return s;
}
void MainWindow::saveFileAction()
{
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    auto dirname = directoryPath(settings.value("saveload/path", ".").value<QString>());

    auto path = filePath(dirname, QString("savefile.h5"));
    QWidget* parent = this;
    path = QFileDialog::getSaveFileName(parent, tr("Save simulation"), path, tr("HDF5 (*.h5)"));
    if (path.isNull())
        return;
    dirname = directoryPath(path);
    settings.setValue("saveload/path", dirname);
    emit saveData(path);
}
void MainWindow::loadFileAction()
{
    // getting file
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    auto dirname = directoryPath(settings.value("saveload/path", ".").value<QString>());

    QWidget* parent = this;
    auto path = QFileDialog::getOpenFileName(parent, tr("Open simulation"), dirname, tr("HDF5 (*.h5)"));
    if (path.isNull())
        return;
    dirname = directoryPath(path);
    settings.setValue("saveload/path", dirname);
    emit loadData(path);
}