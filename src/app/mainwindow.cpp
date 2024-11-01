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
#include <otherphantomimportpipeline.hpp>
#include <otherphantomimportwidget.hpp>
#include <renderwidgetscollection.hpp>
#include <simulationpipeline.hpp>
#include <simulationwidget.hpp>
#include <statusbar.hpp>

#ifdef USECTSEGMENTATOR
#include <ctorgansegmentatorpipeline.hpp>
#endif

#include <mainwindow.hpp>

#include <vector>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    auto statusBar = new StatusBar;
    setStatusBar(statusBar);

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
    rightDock->setFeatures(QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable);
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
    statusBar->registerPipeline(ctimageimportpipeline);
    // setting up signals for ct image import
    connect(ctdicomimportwidget, &CTDicomImportWidget::dicomSeriesActivated, ctimageimportpipeline, &CTImageImportPipeline::readImages);
    connect(ctdicomimportwidget, &CTDicomImportWidget::blurRadiusChanged, ctimageimportpipeline, &CTImageImportPipeline::setBlurRadius);
    connect(ctdicomimportwidget, &CTDicomImportWidget::outputSpacingChanged, ctimageimportpipeline, &CTImageImportPipeline::setOutputSpacing);
    connect(ctdicomimportwidget, &CTDicomImportWidget::useOutputSpacingChanged, ctimageimportpipeline, &CTImageImportPipeline::setUseOutputSpacing);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);

    // adding ct material segmentation pipeline
    auto ctsegmentationpipeline = new CTSegmentationPipeline;
    ctsegmentationpipeline->moveToThread(&m_workerThread);
    pipelineitems.push_back(ctsegmentationpipeline);
    statusBar->registerPipeline(ctsegmentationpipeline);
    connect(ctdicomimportwidget, &CTDicomImportWidget::aqusitionAlFiltrationChanged, ctsegmentationpipeline, &CTSegmentationPipeline::setAlFiltration);
    connect(ctdicomimportwidget, &CTDicomImportWidget::aqusitionSnFiltrationChanged, ctsegmentationpipeline, &CTSegmentationPipeline::setSnFiltration);
    connect(ctdicomimportwidget, &CTDicomImportWidget::aqusitionVoltageChanged, ctsegmentationpipeline, &CTSegmentationPipeline::setAqusitionVoltage);
    connect(ctsegmentationpipeline, &CTSegmentationPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, ctsegmentationpipeline, &CTSegmentationPipeline::updateImageData);

#ifdef USECTSEGMENTATOR
    // adding ct organ segmentation pipeline
    auto ctorgansegmentationpipeline = new CTOrganSegmentatorPipeline;
    ctorgansegmentationpipeline->moveToThread(&m_workerThread);
    pipelineitems.push_back(ctorgansegmentationpipeline);
    statusBar->registerPipeline(ctorgansegmentationpipeline);
    connect(ctdicomimportwidget, &CTDicomImportWidget::useOrganSegmentator, ctorgansegmentationpipeline, &CTOrganSegmentatorPipeline::setUseOrganSegmentator);
    connect(ctdicomimportwidget, &CTDicomImportWidget::requestCancelSegmentation, ctorgansegmentationpipeline,
        &CTOrganSegmentatorPipeline::cancelSegmentation, Qt::DirectConnection); // Not a threadsafe connection, but reciver is threadsafe
    connect(ctorgansegmentationpipeline, &CTOrganSegmentatorPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);
    connect(ctorgansegmentationpipeline, &CTOrganSegmentatorPipeline::importProgressChanged, ctdicomimportwidget, &CTDicomImportWidget::setImportProgress);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, ctorgansegmentationpipeline, &CTOrganSegmentatorPipeline::updateImageData);
#endif
    // Adding icrp phantom import widget
    auto icrpimportwidget = new ICRPPhantomImportWidget(importWidgets);
    importWidgets->addTab(icrpimportwidget, tr("ICRP phantom import"));
    auto icrppipeline = new ICRPPhantomImportPipeline;
    icrppipeline->moveToThread(&m_workerThread);
    statusBar->registerPipeline(icrppipeline);
    connect(icrpimportwidget, &ICRPPhantomImportWidget::setRemoveArms, icrppipeline, &ICRPPhantomImportPipeline::setRemoveArms);
    connect(icrpimportwidget, &ICRPPhantomImportWidget::requestImportPhantom, icrppipeline, &ICRPPhantomImportPipeline::importPhantom);
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);

    // Adding other phantom import widget
    auto otherimportwidget = new OtherPhantomImportWidget(importWidgets);
    importWidgets->addTab(otherimportwidget, tr("Other"));
    auto otherphantompipeline = new OtherPhantomImportPipeline;
    otherphantompipeline->moveToThread(&m_workerThread);
    statusBar->registerPipeline(otherphantompipeline);
    connect(otherimportwidget, &OtherPhantomImportWidget::requestImportPhantom, otherphantompipeline, &OtherPhantomImportPipeline::importPhantom);
    connect(otherphantompipeline, &OtherPhantomImportPipeline::imageDataChanged, slicerender, &RenderWidgetsCollection::updateImageData);

    // beam settings widget
    auto beamsettingswidget = new BeamSettingsWidget(this);
    menuWidget->addTab(beamsettingswidget, tr("Configure X-ray beams"));
    connect(ctsegmentationpipeline, &CTSegmentationPipeline::imageDataChanged, beamsettingswidget, &BeamSettingsWidget::updateImageData);
#ifdef USECTSEGMENTATOR
    connect(ctorgansegmentationpipeline, &CTOrganSegmentatorPipeline::imageDataChanged, beamsettingswidget, &BeamSettingsWidget::updateImageData);
#endif
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, beamsettingswidget, &BeamSettingsWidget::updateImageData);
    connect(otherphantompipeline, &OtherPhantomImportPipeline::imageDataChanged, beamsettingswidget, &BeamSettingsWidget::updateImageData);
    auto beamsettingsmodel = beamsettingswidget->modelView();
    connect(beamsettingsmodel, &BeamSettingsView::beamActorAdded, slicerender, &RenderWidgetsCollection::addBeam);
    connect(beamsettingsmodel, &BeamSettingsView::beamActorRemoved, slicerender, &RenderWidgetsCollection::removeBeam);
    connect(beamsettingsmodel, &BeamSettingsView::requestRender, slicerender, &RenderWidgetsCollection::Render);

    // simulationwidget
    auto simulationwidget = new SimulationWidget(this);
    menuWidget->addTab(simulationwidget, tr("Simulation"));

    // simulationpipeline
    auto simulationpipeline = new SimulationPipeline;
    simulationpipeline->moveToThread(&m_workerThread);
    statusBar->registerPipeline(simulationpipeline);
    connect(ctsegmentationpipeline, &CTSegmentationPipeline::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
#ifdef USECTSEGMENTATOR
    connect(ctorgansegmentationpipeline, &CTOrganSegmentatorPipeline::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
#endif
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
    connect(otherphantompipeline, &OtherPhantomImportPipeline::imageDataChanged, simulationpipeline, &SimulationPipeline::updateImageData);
    connect(simulationwidget, &SimulationWidget::numberOfThreadsChanged, simulationpipeline, &SimulationPipeline::setNumberOfThreads);
    connect(simulationwidget, &SimulationWidget::ignoreAirChanged, simulationpipeline, &SimulationPipeline::setDeleteAirDose);
    connect(simulationwidget, &SimulationWidget::requestStartSimulation, simulationpipeline, &SimulationPipeline::startSimulation);
    connect(simulationwidget, &SimulationWidget::requestStopSimulation, simulationpipeline, &SimulationPipeline::stopSimulation);
    connect(simulationwidget, &SimulationWidget::lowEnergyCorrectionMethodChanged, simulationpipeline, &SimulationPipeline::setLowEnergyCorrectionLevel);
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
    connect(dosetablepipeline, &DoseTablePipeline::clearTable, dosetable, &DoseTableWidget::clear);
    connect(dosetablepipeline, &DoseTablePipeline::enableSorting, dosetable, &DoseTableWidget::enableSorting);
    connect(dosetablepipeline, &DoseTablePipeline::doseData, dosetable, &DoseTableWidget::setDoseData);
    connect(dosetablepipeline, &DoseTablePipeline::doseDataHeader, dosetable, &DoseTableWidget::setDoseDataHeader);
    connect(simulationpipeline, &SimulationPipeline::imageDataChanged, dosetablepipeline, &DoseTablePipeline::updateImageData);
    connect(simulationpipeline, &SimulationPipeline::simulationRunning, dosetablepipeline, &DoseTablePipeline::clearDoseTable);
    connect(ctimageimportpipeline, &CTImageImportPipeline::imageDataChanged, dosetablepipeline, &DoseTablePipeline::updateImageData);

    // save load
    auto h5io = new H5IO;
    h5io->moveToThread(&m_workerThread);
    statusBar->registerPipeline(h5io);
    connect(ctsegmentationpipeline, &CTSegmentationPipeline::imageDataChanged, h5io, &H5IO::updateImageData);
#ifdef USECTSEGMENTATOR
    connect(ctorgansegmentationpipeline, &CTOrganSegmentatorPipeline::imageDataChanged, h5io, &H5IO::updateImageData);
#endif
    connect(icrppipeline, &ICRPPhantomImportPipeline::imageDataChanged, h5io, &H5IO::updateImageData);
    connect(otherphantompipeline, &OtherPhantomImportPipeline::imageDataChanged, h5io, &H5IO::updateImageData);
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
    connect(h5io, &H5IO::beamDataChanged, beamsettingsmodel, &BeamSettingsView::addBeam);

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
