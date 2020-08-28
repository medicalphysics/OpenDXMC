

#include "opendxmc/exportwidget.h"

#include <QCheckBox>
#include <QCompleter>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <filesystem>
#include <fstream>
#include <sstream>

#include <vtkSmartPointer.h>
#include <vtkXMLImageDataWriter.h>

void writeArrayVtk(std::shared_ptr<ImageContainer> image, const std::string& path)
{
    vtkSmartPointer<vtkXMLImageDataWriter> writer = vtkSmartPointer<vtkXMLImageDataWriter>::New();
    writer->SetFileName(path.c_str());
    writer->SetInputData(image->image);
    writer->Write();
}

template <typename U>
void add3array(std::stringstream& stream, U* arr)
{
    for (std::size_t i = 0; i < 3; ++i)
        stream << arr[i] << ", ";
    stream.get();
    stream << std::endl;
}

std::array<char, EXPORT_HEADER_SIZE> getHeaderData(std::shared_ptr<ImageContainer> image)
{
    std::stringstream header;
    const auto& dimensions = image->image->GetDimensions();
    const auto& spacing = image->image->GetSpacing();
    const auto& cosines = image->directionCosines;

    auto array_name = image->getImageName();
    auto image_scalar_type = image->image->GetScalarType();
    std::string image_scalar_name("unknown");
    int image_scalar_size = 0;
    if (image_scalar_type == VTK_FLOAT) {
        image_scalar_name = "float";
        image_scalar_size = sizeof(float);
    }
    if (image_scalar_type == VTK_DOUBLE) {
        image_scalar_name = "double";
        image_scalar_size = sizeof(double);
    }
    if (image_scalar_type == VTK_UNSIGNED_CHAR) {
        image_scalar_name = "unsigned char";
        image_scalar_size = sizeof(unsigned char);
    }
    if (image_scalar_type == VTK_UNSIGNED_INT) {
        image_scalar_name = "unsigned int";
        image_scalar_size = sizeof(unsigned int);
    }

    header << "# HEADER_DATA_BEGIN: " << EXPORT_HEADER_SIZE << std::endl;
    header << "# HEADER_SIZE: " << EXPORT_HEADER_SIZE << std::endl;
    header << "# SCALAR_ARRAY: " << array_name << std::endl;
    header << "# SCALAR_TYPE: " << image_scalar_name << std::endl;
    header << "# SCALAR_SIZE_IN_BYTES: " << image_scalar_size << std::endl;
    header << "# WIDTH: " << dimensions[0] << std::endl;
    header << "# HEIGHT: " << dimensions[1] << std::endl;
    header << "# DEPTH: " << dimensions[2] << std::endl;
    header << "# WIDTH_SPACING: " << spacing[0] << std::endl;
    header << "# HEIGHT_SPACING: " << spacing[1] << std::endl;
    header << "# DEPTH_SPACING: " << spacing[2] << std::endl;
    header << "# COSINES_X1: " << cosines[0] << std::endl;
    header << "# COSINES_X2: " << cosines[1] << std::endl;
    header << "# COSINES_X3: " << cosines[2] << std::endl;
    header << "# COSINES_Y1: " << cosines[3] << std::endl;
    header << "# COSINES_Y2: " << cosines[4] << std::endl;
    header << "# COSINES_Y3: " << cosines[5] << std::endl;
    header << "# DATA_UNITS: " << image->dataUnits << std::endl;
    auto str = header.str();

    std::array<char, EXPORT_HEADER_SIZE> arr;
    arr.fill(' ');
    std::copy(str.begin(), str.end(), arr.begin());
    std::string end_header = "\nHEADER_DATA_END\n";
    auto header_end = arr.begin();
    std::advance(header_end, EXPORT_HEADER_SIZE - end_header.size());
    std::copy(end_header.begin(), end_header.end(), header_end);
    return arr;
}
void writeArrayBin(std::shared_ptr<ImageContainer> image, const std::string& path, bool includeHeader)
{
    std::streamsize size = image->image->GetScalarSize() * image->image->GetNumberOfCells();
    std::fstream file;
    file.open(path, std::ios::out | std::ios::binary);
    if (file.is_open()) {
        if (includeHeader) {
            auto header = getHeaderData(image);
            file.write(header.data(), EXPORT_HEADER_SIZE);
        }
        file.write(reinterpret_cast<char*>(image->image->GetScalarPointer()), size);
        file.close();
    }
}
ExportWorker::ExportWorker(QObject* parent)
    : QObject(parent)
{
}

void ExportWorker::exportRawData(std::vector<std::shared_ptr<ImageContainer>> images, QString dir, bool includeHeader)
{
    for (const auto& im : images) {
        std::string filenameBin(im->getImageName() + ".bin");
        auto filepathBin = std::filesystem::path(dir.toStdString()) / filenameBin;
        writeArrayBin(im, filepathBin.string(), includeHeader);
    }
    emit exportFinished();
}

void ExportWorker::exportVTKData(std::vector<std::shared_ptr<ImageContainer>> images, QString dir)
{
    for (const auto& im : images) {
        std::string filenameVtk(im->getImageName() + ".vti");
        auto filepathVtk = std::filesystem::path(dir.toStdString()) / filenameVtk;
        writeArrayVtk(im, filepathVtk.string());
    }
    emit exportFinished();
}

ExportWidget::ExportWidget(QWidget* parent)
    : QWidget(parent)
{
    qRegisterMetaType<std::vector<std::shared_ptr<ImageContainer>>>();

    auto* mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    setupRawExportWidgets();
    setupVTKExportWidgets();

    mainLayout->addStretch();
    setLayout(mainLayout);

    m_worker = new ExportWorker();
    m_worker->moveToThread(&m_workerThread);
    connect(m_worker, &ExportWorker::exportFinished, [=](void) { emit this->processingDataEnded(); });
    connect(this, &ExportWidget::exportRawData, m_worker, &ExportWorker::exportRawData);
    connect(this, &ExportWidget::exportVTKData, m_worker, &ExportWorker::exportVTKData);
    m_workerThread.start();
}

void ExportWidget::setupRawExportWidgets()
{
    //exposing settings
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    //Setting up path completers
    auto* exportRawBrowseCompleter = new QCompleter(this);
    auto* exportRawBrowseCompleterModel = new QFileSystemModel(this);
    exportRawBrowseCompleterModel->setRootPath("");
    exportRawBrowseCompleterModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    exportRawBrowseCompleter->setModel(exportRawBrowseCompleterModel);
    exportRawBrowseCompleter->setCompletionMode(QCompleter::InlineCompletion);

    //export raw binary files
    // setting up line edit
    m_exportRawLineEdit = new QLineEdit(this);
    m_exportRawLineEdit->setClearButtonEnabled(true);
    m_exportRawLineEdit->setCompleter(exportRawBrowseCompleter);
    m_exportRawLineEdit->setText(settings.value("dataexport/rawexportfolder").value<QString>());
    connect(this, &ExportWidget::rawExportFolderSelected, [=](const QString& folderPath) {
        exportRawBrowseCompleter->setCompletionPrefix(folderPath);
        m_exportRawLineEdit->setText(folderPath);
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        settings.setValue("dataexport/rawexportfolder", folderPath);
        settings.sync();
    });

    // setting up browse button
    auto* exportRawBrowseButton = new QPushButton(tr("Browse"), this);
    exportRawBrowseButton->setFixedHeight(m_exportRawLineEdit->sizeHint().height());
    connect(exportRawBrowseButton, &QPushButton::clicked, this, &ExportWidget::browseForRawExportFolder);

    // adding line edit and button on line
    auto* exportRawBrowseLayout = new QHBoxLayout;
    exportRawBrowseLayout->addWidget(m_exportRawLineEdit);
    exportRawBrowseLayout->addWidget(exportRawBrowseButton);

    // setting up inluce header option
    auto* exportRawHeaderLayout = new QHBoxLayout;
    auto* exportRawHeaderCheckBox = new QCheckBox(tr("Include header in exported files?"), this);
    exportRawHeaderLayout->addWidget(exportRawHeaderCheckBox);
    if (settings.contains("dataexport/rawexportincludeheader")) {
        m_rawExportIncludeHeader = settings.value("dataexport/rawexportincludeheader").value<bool>();
    } else {
        m_rawExportIncludeHeader = true;
    }
    exportRawHeaderCheckBox->setChecked(m_rawExportIncludeHeader ? Qt::Checked : Qt::Unchecked);
    connect(exportRawHeaderCheckBox, &QCheckBox::stateChanged, [=](int state) {
        if (state == Qt::Unchecked)
            this->m_rawExportIncludeHeader = false;
        else
            this->m_rawExportIncludeHeader = true;
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        settings.setValue("dataexport/rawexportincludeheader", this->m_rawExportIncludeHeader);
        settings.sync();
    });

    // large export button
    auto* exportRawButton = new QPushButton(tr("Export all"), this);
    connect(exportRawButton, &QPushButton::clicked, this, &ExportWidget::exportAllRawData);

    // Adding a nice description
    auto rawDescription = new QLabel(tr("Export all image volumes as binary files to a selected folder. If the check box for include headers is checked the binary file will start with a header of 4096 bytes containing some metadata for the volume. These files are intended to easily read by applications or programming languages that support reading of plain bytes from a file."));
    rawDescription->setWordWrap(true);

    // setting up vertical layout
    auto* exportRawLayout = new QVBoxLayout;
    exportRawLayout->addWidget(rawDescription);
    exportRawLayout->addLayout(exportRawBrowseLayout);
    exportRawLayout->addLayout(exportRawHeaderLayout);
    exportRawLayout->addWidget(exportRawButton);

    // Putting binary export functionality in a neat box
    auto* rawExportBox = new QGroupBox(tr("Select folder for raw export of binary data"), this);
    rawExportBox->setLayout(exportRawLayout);
    this->layout()->addWidget(rawExportBox);
}
void ExportWidget::setupVTKExportWidgets()
{
    //exposing settings
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    //Setting up path completers
    auto* exportVTKBrowseCompleter = new QCompleter(this);
    auto* exportVTKBrowseCompleterModel = new QFileSystemModel(this);
    exportVTKBrowseCompleterModel->setRootPath("");
    exportVTKBrowseCompleterModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    exportVTKBrowseCompleter->setModel(exportVTKBrowseCompleterModel);
    exportVTKBrowseCompleter->setCompletionMode(QCompleter::InlineCompletion);

    //export raw binary files
    // setting up line edit
    m_exportVTKLineEdit = new QLineEdit(this);
    m_exportVTKLineEdit->setClearButtonEnabled(true);
    m_exportVTKLineEdit->setCompleter(exportVTKBrowseCompleter);
    m_exportVTKLineEdit->setText(settings.value("dataexport/vtkexportfolder").value<QString>());
    connect(this, &ExportWidget::vtkExportFolderSelected, [=](const QString& folderPath) {
        exportVTKBrowseCompleter->setCompletionPrefix(folderPath);
        m_exportVTKLineEdit->setText(folderPath);
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        settings.setValue("dataexport/vtkexportfolder", folderPath);
        settings.sync();
    });

    // setting up browse button
    auto* exportBrowseButton = new QPushButton(tr("Browse"), this);
    exportBrowseButton->setFixedHeight(m_exportVTKLineEdit->sizeHint().height());
    connect(exportBrowseButton, &QPushButton::clicked, this, &ExportWidget::browseForVTKExportFolder);

    // adding line edit and button on line
    auto* exportBrowseLayout = new QHBoxLayout;
    exportBrowseLayout->addWidget(m_exportVTKLineEdit);
    exportBrowseLayout->addWidget(exportBrowseButton);

    // large export button
    auto* exportButton = new QPushButton(tr("Export all"), this);
    connect(exportButton, &QPushButton::clicked, this, &ExportWidget::exportAllVTKData);

    // Adding a nice description
    auto description = new QLabel(tr("Export all volumes as .vtk files. This is a file format used by the Visualization Toolkit and can be opened by applications such as Paraview."));
    description->setWordWrap(true);

    // setting up vertical layout
    auto* exportLayout = new QVBoxLayout;
    exportLayout->addWidget(description);
    exportLayout->addLayout(exportBrowseLayout);
    exportLayout->addWidget(exportButton);

    // Putting binary export functionality in a neat box
    auto* exportBox = new QGroupBox(tr("Select folder for export of vtk files"), this);
    exportBox->setLayout(exportLayout);
    this->layout()->addWidget(exportBox);
}
void ExportWidget::browseForRawExportFolder()
{
    //find folder and emit signal
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    QString initPath;
    if (settings.contains("dataexport/rawexportfolder"))
        initPath = settings.value("dataexport/rawexportfolder").value<QString>();
    else {
        initPath = ".";
    }
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select folder for export"),
        initPath,
        QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty())
        emit rawExportFolderSelected(dir);
}

void ExportWidget::browseForVTKExportFolder()
{
    //find folder and emit signal
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    QString initPath;
    if (settings.contains("dataexport/vtkexportfolder"))
        initPath = settings.value("dataexport/vtkexportfolder").value<QString>();
    else {
        initPath = ".";
    }
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select folder for export"),
        initPath,
        QFileDialog::ShowDirsOnly);
    if (!dir.isEmpty())
        emit vtkExportFolderSelected(dir);
}

void ExportWidget::exportAllRawData()
{
    emit processingDataStarted();
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    QString dir;
    if (settings.contains("dataexport/rawexportfolder"))
        dir = settings.value("dataexport/rawexportfolder").value<QString>();
    else {
        dir = ".";
    }
    emit exportRawData(m_images, dir, m_rawExportIncludeHeader);
}

void ExportWidget::exportAllVTKData()
{
    emit processingDataStarted();
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    QString dir;
    if (settings.contains("dataexport/vtkexportfolder"))
        dir = settings.value("dataexport/vtkexportfolder").value<QString>();
    else {
        dir = ".";
    }
    emit exportVTKData(m_images, dir);
}

void ExportWidget::registerImage(std::shared_ptr<ImageContainer> image)
{
    if (!(image->image))
        return;
    auto currentID = image->ID;
    bool newImageID = false;
    for (const auto& im : m_images) {
        if (im->ID != currentID)
            newImageID = true;
    }
    if (newImageID)
        m_images.clear();

    bool imagereplaced = false;
    for (auto& im : m_images) {
        if (im->imageType == image->imageType) {
            imagereplaced = true;
            im = image;
        }
    }
    if (!imagereplaced)
        m_images.push_back(image);
}
ExportWidget::~ExportWidget()
{
    m_workerThread.quit();
    m_workerThread.wait();
    if (m_worker) {
        delete m_worker;
    }
}