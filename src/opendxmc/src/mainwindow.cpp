
#include <QSplitter>
#include <QStatusbar>

#include "mainwindow.h"
#include "viewportwidget.h"
#include "dicomimportwidget.h"
#include "imageimportpipeline.h"
#include "progressindicator.h"
//#include "materialselectionwidget.h"
#include "sourceeditorwidget.h"
#include "phantomselectionwidget.h"
#include "dosereportwidget.h"

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

	//statusbar and progress widget
	auto statusBar = this->statusBar();
	auto progressIndicator = new ProgressIndicator(this);
	connect(m_importPipeline, &ImageImportPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
	connect(m_importPipeline, &ImageImportPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
	connect(m_simulationPipeline, &SimulationPipeline::processingDataStarted, progressIndicator, &ProgressIndicator::startAnimation);
	connect(m_simulationPipeline, &SimulationPipeline::processingDataEnded, progressIndicator, &ProgressIndicator::stopAnimation);
	statusBar->addPermanentWidget(progressIndicator);



	QSplitter* splitter = new QSplitter(Qt::Horizontal);

	m_menuWidget = new QTabWidget(this);
	m_menuWidget->setTabPosition(QTabWidget::West);
	
	

	//dicom import widget
	DicomImportWidget *dicomImportWidget = new DicomImportWidget(this);
	m_menuWidget->addTab(dicomImportWidget, tr("Import DiCOM CT images"));
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
	m_menuWidget->addTab(phantomWidget, tr("Digital phantoms"));
	connect(phantomWidget, &PhantomSelectionWidget::readIRCUFemalePhantom, m_importPipeline, &ImageImportPipeline::importICRUFemalePhantom);
	connect(phantomWidget, &PhantomSelectionWidget::readIRCUMalePhantom, m_importPipeline, &ImageImportPipeline::importICRUMalePhantom);
	connect(phantomWidget, &PhantomSelectionWidget::readCTDIPhantom, m_importPipeline, &ImageImportPipeline::importCTDIPhantom);


	//source edit widget
	auto sourceEditWidget = new SourceEditWidget(this);
	m_menuWidget->addTab(sourceEditWidget, tr("X-ray sources"));
	auto sourceEditDelegate = sourceEditWidget->delegate();
	connect(m_importPipeline, &ImageImportPipeline::aecFilterChanged, sourceEditDelegate, &SourceDelegate::addAecFilter);

	//dosereportWidget
	auto doseReportWidget = new DoseReportWidget(this);
	connect(m_simulationPipeline, &SimulationPipeline::doseDataChanged, doseReportWidget, &DoseReportWidget::setDoseData);
	m_menuWidget->addTab(doseReportWidget, tr("Dose summary"));

	splitter->addWidget(m_menuWidget);
	
	//Viewport
	ViewPortWidget* viewPort = new ViewPortWidget(this);
	splitter->addWidget(viewPort);
	connect(m_importPipeline, &ImageImportPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
	connect(m_simulationPipeline, &SimulationPipeline::imageDataChanged, viewPort, &ViewPortWidget::setImageData);
	setCentralWidget(splitter);

	//setting up source 3d actor connection to viewpoert from sourceeditwidget
	auto model = sourceEditWidget->model();
	connect(model, &SourceModel::sourceAdded, viewPort, &ViewPortWidget::addActorContainer);
	connect(model, &SourceModel::actorsChanged, viewPort, &ViewPortWidget::render);
	connect(model, &SourceModel::sourceRemoved, viewPort, &ViewPortWidget::removeActorContainer);
	
	//request to run simulation connection
	connect(sourceEditWidget, &SourceEditWidget::runSimulation, m_simulationPipeline, &SimulationPipeline::runSimulation);

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
