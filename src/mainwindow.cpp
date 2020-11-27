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

#include <QAction>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>

#include "opendxmc/binaryimportwidget.h"
#include "opendxmc/dicomimportwidget.h"
#include "opendxmc/dosereportwidget.h"
#include "opendxmc/exportwidget.h"
#include "opendxmc/imageimportpipeline.h"
#include "opendxmc/mainwindow.h"
#include "opendxmc/phantomimportpipeline.h"
#include "opendxmc/phantomselectionwidget.h"
#include "opendxmc/progressindicator.h"
#include "opendxmc/qpathmanipulation.h"
#include "opendxmc/sourceeditorwidget.h"
#include "opendxmc/viewportwidget.h"

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{

    //image import pipeline
    m_importPipeline = new ImageImportPipeline();
    m_importPipeline->moveToThread(&m_workerThread);
    connect(m_importPipeline, &ImageImportPipeline::processingDataStarted, this, &MainWindow::setDisableEditing);
    connect(m_importPipeline, &ImageImportPipeline::processingDataEnded, this, &MainWindow::setEnableEditing);
    // phantom import pipeline
    m_phantomImportPipeline = new PhantomImportPipeline();
    m_phantomImportPipeline->moveToThread(&m_workerThread);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::processingDataStarted, this, &MainWindow::setDisableEditing);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::processingDataEnded, this, &MainWindow::setEnableEditing);
    //simulation pipeline
    m_simulationPipeline = new SimulationPipeline();
    m_simulationPipeline->moveToThread(&m_workerThread);
    connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, m_simulationPipeline, &SimulationPipeline::setImageData);
    connect(m_importPipeline, &ImageImportPipeline::materialDataChanged, m_simulationPipeline, &SimulationPipeline::setMaterials);
    connect(m_importPipeline, &ImageImportPipeline::organDataChanged, m_simulationPipeline, &SimulationPipeline::setOrganList);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::imageDataChanged, m_simulationPipeline, &SimulationPipeline::setImageData);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::materialDataChanged, m_simulationPipeline, &SimulationPipeline::setMaterials);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::organDataChanged, m_simulationPipeline, &SimulationPipeline::setOrganList);
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
    connect(m_phantomImportPipeline, &PhantomImportPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
    connect(m_binaryImportPipeline, &BinaryImportPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
    connect(m_binaryImportPipeline, &BinaryImportPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
    connect(m_simulationPipeline, &SimulationPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
    connect(m_simulationPipeline, &SimulationPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
    statusBar->addPermanentWidget(progressIndicator);

    m_menuWidget = new QTabWidget(this);
    m_menuWidget->setTabPosition(QTabWidget::West);

    //import widgets share a tabbed widget
    auto importWidget = new QTabWidget(this);
    importWidget->setTabPosition(QTabWidget::North);

    //dicom import widget
    DicomImportWidget* dicomImportWidget = new DicomImportWidget(this);
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
    connect(phantomWidget, &PhantomSelectionWidget::readIRCUFemalePhantom, m_phantomImportPipeline, &PhantomImportPipeline::importICRUFemalePhantom);
    connect(phantomWidget, &PhantomSelectionWidget::readIRCUMalePhantom, m_phantomImportPipeline, &PhantomImportPipeline::importICRUMalePhantom);
    connect(phantomWidget, &PhantomSelectionWidget::readCTDIPhantom, m_phantomImportPipeline, &PhantomImportPipeline::importCTDIPhantom);
    connect(phantomWidget, &PhantomSelectionWidget::readAWSPhantom, m_phantomImportPipeline, &PhantomImportPipeline::importAWSPhantom);

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
    connect(exportWidget, &ExportWidget::processingDataStarted, this, &MainWindow::setDisableEditing);
    connect(exportWidget, &ExportWidget::processingDataEnded, this, &MainWindow::setEnableEditing);
    connect(exportWidget, &ExportWidget::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
    connect(exportWidget, &ExportWidget::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);

    m_menuWidget->addTab(exportWidget, tr("Export data"));

    //simulation progress
    m_progressTimer = new QTimer(this);
    m_progressTimer->setTimerType(Qt::CoarseTimer);
    connect(m_simulationPipeline, &SimulationPipeline::progressBarChanged, this, &MainWindow::setProgressBar);
    connect(m_progressTimer, &QTimer::timeout, this, &MainWindow::updateProgressBar);

    //Viewport
    ViewPortWidget* viewPort = new ViewPortWidget(this);
    connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    connect(m_phantomImportPipeline, &PhantomImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    connect(m_simulationPipeline, &SimulationPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
    connect(m_binaryImportPipeline, &BinaryImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);

    //setting up source 3d actor connection to viewpoert from sourceeditwidget
    auto sourceModel = sourceEditWidget->model();
    connect(sourceModel, &SourceModel::sourceActorAdded, viewPort, &ViewPortWidget::addActorContainer);
    connect(sourceModel, &SourceModel::actorsChanged, viewPort, &ViewPortWidget::render);
    connect(sourceModel, &SourceModel::sourceActorRemoved, viewPort, &ViewPortWidget::removeActorContainer);

    //request to run simulation connection
    connect(sourceEditWidget, &SourceEditWidget::runSimulation, m_simulationPipeline, &SimulationPipeline::runSimulation);

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

    // setting up layout
    QSplitter* splitter = new QSplitter(Qt::Horizontal);
    QWidget* menuHolder = new QWidget(this);
    QVBoxLayout* holderLayout = new QVBoxLayout;
    holderLayout->setContentsMargins(0, 0, 0, 0);
    holderLayout->addWidget(m_menuWidget);
    holderLayout->addWidget(m_progressWidget);
    menuHolder->setLayout(holderLayout);

    splitter->addWidget(menuHolder);
    splitter->addWidget(viewPort);
    splitter->setStretchFactor(0, 1.0);
    splitter->setStretchFactor(1, 10.0);
    splitter->setOpaqueResize(false);
    setCentralWidget(splitter);

    //setting up window menu
    createMenu();

    //no connections to pipeline after this point
    m_workerThread.start();
}

MainWindow::~MainWindow()
{
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
}

void MainWindow::createMenu()
{
    auto fileMenu = menuBar()->addMenu(tr("&File"));

    auto saveAction = new QAction(tr("Save as"), this);
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
    emit requestSaveToFile(path);
}

void MainWindow::loadFileAction()
{
    //getting file
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    auto dirname = directoryPath(settings.value("saveload/path", ".").value<QString>());

    QWidget* parent = this;
    auto path = QFileDialog::getOpenFileName(parent, tr("Open simulation"), dirname, tr("HDF5 (*.h5)"));
    if (path.isNull())
        return;
    dirname = directoryPath(path);
    settings.setValue("saveload/path", dirname);
    emit requestOpenSaveFile(path);
}

void MainWindow::setEnableEditing(void)
{
    for (int i = 0; i < m_menuWidget->count(); ++i) {
        auto wid = m_menuWidget->widget(i);
        wid->setEnabled(true);
    }
}

void MainWindow::setDisableEditing(void)
{
    for (int i = 0; i < m_menuWidget->count(); ++i) {
        auto wid = m_menuWidget->widget(i);
        wid->setDisabled(true);
    }
}

void MainWindow::setProgressBar(ProgressBar* progressBar)
{
    m_progressBar = progressBar;
    m_progressTimer->start(5000);
}

void MainWindow::updateProgressBar()
{
    m_progressWidget->show();
    if (m_progressBar) {
        const auto msg = m_progressBar->getETA();
        this->statusBar()->showMessage(QString::fromStdString(msg), 6000);
        if (m_progressWidget->showProgress())
            m_progressWidget->setImageData(m_progressBar->computeDoseProgressImage());
        if (m_progressWidget->cancelRun())
            m_progressBar->setCancel(true);
    } else {
        m_progressWidget->hide();
        m_progressTimer->stop();
    }
}
