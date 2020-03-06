

#include "opendxmc/exportwidget.h"

#include <QSettings>
#include <QVBoxLayout>
#include <QCompleter>
#include <QFileSystemModel>
#include <QDir>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QCheckBox>

#include <filesystem>
#include <fstream>
#include <sstream>

#include <vtkSmartPointer.h>
#include <vtkXMLImageDataWriter.h>

ExportWidget::ExportWidget(QWidget* parent)
	: QWidget(parent)
{
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
	auto* mainLayout = new QVBoxLayout;

	//export raw binary files
	auto *exportRawBrowseLayout = new QHBoxLayout;
	
	m_exportRawLineEdit = new QLineEdit(this);
	m_exportRawLineEdit->setClearButtonEnabled(true);
	exportRawBrowseLayout->addWidget(m_exportRawLineEdit);
	auto *exportRawBrowseCompleter = new QCompleter(this);
	auto *exportRawBrowseCompleterModel = new QFileSystemModel(this);
	exportRawBrowseCompleterModel->setRootPath("");
	exportRawBrowseCompleterModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
	exportRawBrowseCompleter->setModel(exportRawBrowseCompleterModel);
	exportRawBrowseCompleter->setCompletionMode(QCompleter::InlineCompletion);
	connect(this, &ExportWidget::rawExportFolderSelected, [=](const QString& folderPath) {
		exportRawBrowseCompleter->setCompletionPrefix(folderPath);
		m_exportRawLineEdit->setText(folderPath);
		QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
		settings.setValue("dataexport/rawexportfolder", folderPath);
		settings.sync();
		});
	m_exportRawLineEdit->setCompleter(exportRawBrowseCompleter);
	m_exportRawLineEdit->setText(settings.value("dataexport/rawexportfolder").value<QString>());
	
	auto *exportRawBrowseButton = new QPushButton(tr("Browse"), this);
	connect(exportRawBrowseButton, &QPushButton::clicked, this, &ExportWidget::browseForRawExportFolder);
	exportRawBrowseLayout->addWidget(exportRawBrowseButton);
	exportRawBrowseButton->setFixedHeight(m_exportRawLineEdit->sizeHint().height());

	auto *exportRawLayout = new QVBoxLayout;
	exportRawLayout->addLayout(exportRawBrowseLayout);

	auto *exportRawHeaderLayout = new QHBoxLayout;
	
	auto *exportRawHeaderCheckBox = new QCheckBox(tr("Include header in exported files?"), this);
	exportRawHeaderLayout->addWidget(exportRawHeaderCheckBox);

	if (settings.contains("dataexport/rawexportincludeheader")){
		m_rawExportIncludeHeader = settings.value("dataexport/rawexportincludeheader").value<bool>();
	}
	else {
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
	exportRawLayout->addLayout(exportRawHeaderLayout);

	auto* exportRawButton = new QPushButton(tr("Export all"), this);
	exportRawLayout->addWidget(exportRawButton);
	connect(exportRawButton, &QPushButton::clicked, this, &ExportWidget::exportAllRawData);

	auto* rawExportBox = new QGroupBox(tr("Select folder for raw export of binary data"), this);
	rawExportBox->setLayout(exportRawLayout);

	mainLayout->addWidget(rawExportBox);
	mainLayout->addStretch();
	setLayout(mainLayout);
}

void ExportWidget::browseForRawExportFolder()
{
	//finn mappe og emit 
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
	QString initPath;
	if (settings.contains("dataexport/rawexportfolder"))
		initPath = settings.value("dataexport/rawexportfolder").value<QString>();
	else
	{
		initPath = ".";
	}
	QString dir = QFileDialog::getExistingDirectory(this, tr("Select folder for export"),
		initPath,
		QFileDialog::ShowDirsOnly);
	if (!dir.isEmpty())
		emit rawExportFolderSelected(dir);
}


void writeArrayVtk(std::shared_ptr<ImageContainer> image, const std::string& path)
{
	vtkSmartPointer<vtkXMLImageDataWriter> writer =
		vtkSmartPointer<vtkXMLImageDataWriter>::New();
	writer->SetFileName(path.c_str());
	writer->SetInputData(image->image);
	writer->Write();
}

template<typename U>
void add3array(std::stringstream& stream, U* arr)
{
	for (std::size_t i = 0; i < 3; ++i)
		stream << arr[i] << ", ";
	stream.get();
	stream << std::endl;
}

std::array<char, EXPORT_HEADER_SIZE> ExportWidget::getHeaderData(std::shared_ptr<ImageContainer> image)
{
	std::stringstream header;
	auto dimensions = image->image->GetDimensions();
	auto spacing = image->image->GetSpacing();
	auto cosines = image->directionCosines;

	header << "# HEADER_DATA_BEGIN: " << EXPORT_HEADER_SIZE << std::endl;
	header << "# HEADER_SIZE: " << EXPORT_HEADER_SIZE << std::endl;
	header << "# SCALAR_ARRAY: " << "ORGANDATA" << std::endl;
	header << "# SCALAR_TYPE: " << "unsigned char" << std::endl;
	header << "# SCALAR_SIZE_IN_BYTES: " << sizeof(unsigned char) << std::endl;
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

void ExportWidget::writeArrayBin(std::shared_ptr<ImageContainer> image, const std::string& path, bool includeHeader)
{
	std::streamsize size = image->image->GetScalarSize() * image->image->GetNumberOfCells();
	std::fstream file;
	file.open(path, std::ios::out | std::ios::binary);
	if (file.is_open())
	{
		if (includeHeader)
		{
			auto header = ExportWidget::getHeaderData(image);
			file.write(header.data(), EXPORT_HEADER_SIZE);
		}
		file.write(reinterpret_cast<char*>(image->image->GetScalarPointer()), size);
		file.close();
	}
}

void ExportWidget::exportAllRawData()
{
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
	QString dir;
	if (settings.contains("dataexport/rawexportfolder"))
		dir = settings.value("dataexport/rawexportfolder").value<QString>();
	else
	{
		dir = ".";
	}
	QDir cDir(dir);

	for (const auto& im : m_images)
	{
		std::string filenameBin(im->getImageName() + ".bin");
		auto filepathBin = std::filesystem::path(dir.toStdString()) / filenameBin;
		writeArrayBin(im, filepathBin.string(), m_rawExportIncludeHeader);
		std::string filenameVtk(im->getImageName() + ".vti");
		auto filepathVtk = std::filesystem::path(dir.toStdString()) / filenameVtk;
		writeArrayVtk(im, filepathVtk.string());
	}
}

void ExportWidget::registerImage(std::shared_ptr<ImageContainer> image)
{
	if (!(image->image))
		return;
	auto currentID = image->ID;
	bool newImageID = false;
	for (const auto& im : m_images)
	{
		if (im->ID != currentID)
			newImageID = true;
	}
	if (newImageID)
		m_images.clear();

	bool imagereplaced = false;
	for (auto& im : m_images)
	{
		if (im->imageType == image->imageType)
		{
			imagereplaced = true;
			im = image;
		}
	}
	if (!imagereplaced)
		m_images.push_back(image);
}