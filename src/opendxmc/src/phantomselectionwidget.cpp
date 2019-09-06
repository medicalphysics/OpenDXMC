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

#include "phantomselectionwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDir>
#include <QTimer>
#include <QDebug>
#include <QFileDialog>
#include <QSettings>
#include <QTextBrowser>
#include <QPushButton>
#include <vector>
#include <string>
#include <array>
#include <fstream>
#include <sstream>
#include <filesystem>

constexpr std::size_t IMPORT_HEADER_SIZE = 4096;

PhantomSelectionWidget::PhantomSelectionWidget(QWidget *parent)
: QWidget(parent)
{
	auto mainLayout = new QVBoxLayout();


	auto selectLayout = new QHBoxLayout(this);
	auto selectionBox = new QGroupBox(tr("Select voxel phantom"), this);
	m_phantomSelector = new QComboBox(this);
	selectLayout->addWidget(m_phantomSelector);
	selectionBox->setLayout(selectLayout);

	m_phantomSelector->addItem(tr(""));
	m_phantomSelector->addItem(tr("CTDI Phantom 320 mm"));
	m_phantomSelector->addItem(tr("CTDI Phantom 160 mm"));
	m_phantomSelector->addItem(tr("Female ICRU reference phantom"));
	m_phantomSelector->addItem(tr("Female ICRU reference phantom without arms"));
	m_phantomSelector->addItem(tr("Male ICRU reference phantom"));
	m_phantomSelector->addItem(tr("Male ICRU reference phantom without arms"));

	//connecting slots
	connect(m_phantomSelector, QOverload<int>::of(&QComboBox::activated),
		[=](int index) { 
		if (index == 1) 
			emit this->readCTDIPhantom(320);
		else if (index == 2) 
			emit this->readCTDIPhantom(160);
		else if (index == 3)
			emit this->readIRCUFemalePhantom(false);
		else if (index == 4)
			emit this->readIRCUFemalePhantom(true);
		else if (index == 5)
			emit this->readIRCUMalePhantom(false);
		else if (index == 6)
			emit this->readIRCUMalePhantom(true);
	});
	connect(m_phantomSelector, QOverload<const QString&>::of(&QComboBox::activated),
		[=](const QString& text) {
			if (text.compare(tr("Katja (pregnant female)")) == 0)
				emit this->readAWSPhantom("Katja");
			else if (text.compare(tr("Baby")) == 0)
				emit this->readAWSPhantom("Baby");
			else if (text.compare(tr("Child")) == 0)
				emit this->readAWSPhantom("Child");
			else if (text.compare(tr("Donna")) == 0)
				emit this->readAWSPhantom("Donna");
			else if (text.compare(tr("Frank")) == 0)
				emit this->readAWSPhantom("Frank");
			else if (text.compare(tr("Golem")) == 0)
				emit this->readAWSPhantom("Golem");
			else if (text.compare(tr("Helga")) == 0)
				emit this->readAWSPhantom("Helga");
			else if (text.compare(tr("Irene")) == 0)
				emit this->readAWSPhantom("Irene");
			else if (text.compare(tr("Jo")) == 0)
				emit this->readAWSPhantom("Jo");
			else if (text.compare(tr("Vishum")) == 0)
				emit this->readAWSPhantom("Vishum");
		});

	mainLayout->addWidget(selectionBox);

	auto importHelmholtzBox = new QGroupBox(tr("Import Helmholtz-Zentrum phantoms"), this);
	auto importHelmholtzLayout = new QHBoxLayout(this);
	auto importHelmholtzLabel = new QTextBrowser( this);
	importHelmholtzLabel->setText(tr("""Import phantoms made by Helmholtz-Zentrum. Phantoms can be obtained from <a href=https://www.helmholtz-muenchen.de/en/irm/service/virtual-human-download-portal/virtual-human-phantoms-download>helmholtz-muenchen.de</a>. Unzip phantoms into a folder before importing. After importing the phantom will be available in the dropdown menu."));
	importHelmholtzLabel->setOpenExternalLinks(true);
	importHelmholtzLabel->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);
	auto importHelmholtzButton = new QPushButton(tr("Select folder"), this);
	importHelmholtzLayout->addWidget(importHelmholtzLabel);
	importHelmholtzLayout->addWidget(importHelmholtzButton);
	importHelmholtzBox->setLayout(importHelmholtzLayout);
	mainLayout->addWidget(importHelmholtzBox);
	connect(importHelmholtzButton, &QPushButton::clicked, [=](void) {this->importHelmholtzPhantoms(); });
	QTimer::singleShot(0, [=]() {
		importHelmholtzLabel->document()->adjustSize();
		importHelmholtzLabel->updateGeometry();
		importHelmholtzLabel->setMaximumHeight(importHelmholtzLabel->document()->size().height());
		qDebug() << importHelmholtzLabel->document()->size();

		});

	mainLayout->addStretch();
	this->setLayout(mainLayout);

	addInstalledPhantoms();



}


std::vector<unsigned char> readArray(const std::string& path, std::size_t size, std::size_t headerSize = 4096)
{
	std::vector<unsigned char> arr(size, 0);
	std::ifstream input(path, std::ios::in | std::ios::binary);
	input.seekg(headerSize + 1);
	input.read(reinterpret_cast<char*>(arr.data()), size);
	return arr;
}

std::array<char, IMPORT_HEADER_SIZE> createHeader(
	const std::array<std::size_t, 3> & dimensions,
	const std::array<double, 3> & spacing,
	const std::array<double, 6> & cosines)
{
	std::stringstream header;
	header << "# HEADER_DATA_BEGIN: " << IMPORT_HEADER_SIZE << std::endl;
	header << "# HEADER_SIZE: " << IMPORT_HEADER_SIZE << std::endl;
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

	std::array<char, IMPORT_HEADER_SIZE> arr;
	arr.fill(' ');
	std::copy(str.begin(), str.end(), arr.begin());
	std::string end_header = "\n# HEADER_DATA_END\n";
	auto header_end = arr.begin();
	std::advance(header_end, IMPORT_HEADER_SIZE - end_header.size());
	std::copy(end_header.begin(), end_header.end(), header_end);
	return arr;
}

std::size_t index(std::size_t x, std::size_t y, std::size_t z, const std::array<std::size_t, 3> & dim)
{
	return x + y * dim[0] + z * dim[0] * dim[1];
}

std::vector<unsigned char> reverseX(const std::vector<unsigned char>& vector, const std::array<std::size_t, 3> & dim)
{
	std::vector<unsigned char> rev(vector.size(), 0);

	for (std::size_t i = 0; i < dim[0]; ++i)
	{
		auto im = dim[0] - i - 1;
		for (std::size_t j = 0; j < dim[1]; ++j)
		{
			for (std::size_t k = 0; k < dim[2]; ++k)
			{
				auto idx = index(i, j, k, dim);
				auto idxm = index(im, j, k, dim);
				rev[idx] = vector[idxm];
			}
		}
	}
	return rev;
}

std::vector<unsigned char> reverseY(const std::vector<unsigned char>& vector, const std::array<std::size_t, 3> & dim)
{
	std::vector<unsigned char> rev(vector.size(), 0);

	for (std::size_t i = 0; i < dim[0]; ++i)
		for (std::size_t j = 0; j < dim[1]; ++j)
		{
			auto jm = dim[1] - j - 1;
			for (std::size_t k = 0; k < dim[2]; ++k)
			{
				auto idx = index(i, j, k, dim);
				auto idxm = index(i, jm, k, dim);
				rev[idx] = vector[idxm];
			}
		}
	return rev;
}

std::vector<unsigned char> reverseZ(const std::vector<unsigned char>& vector, const std::array<std::size_t, 3> & dim)
{
	std::vector<unsigned char> rev(vector.size(), 0);

	for (std::size_t i = 0; i < dim[0]; ++i)
		for (std::size_t j = 0; j < dim[1]; ++j)
		{
			for (std::size_t k = 0; k < dim[2]; ++k)
			{
				auto km = dim[2] - k - 1;
				auto idx = index(i, j, k, dim);
				auto idxm = index(i, j, km, dim);
				rev[idx] = vector[idxm];
			}
		}
	return rev;
}

void writeArray(
	const std::array<char, IMPORT_HEADER_SIZE>& header,
	const std::vector<unsigned char>& array,
	const std::string& path)
{
	std::ofstream file;
	file.open(path, std::ios::out | std::ios::binary);

	file.write(header.data(), IMPORT_HEADER_SIZE);

	file.write(reinterpret_cast<const char*>(array.data()), array.size());
	file.close();
}

struct phantom
{
	std::array<double, 3> s;
	std::array<std::size_t, 3> d;
	std::array<double, 6> cosines;
	std::string read;
	std::string save;
	bool reverseX = false;
	bool reverseY = false;
	bool reverseZ = false;
};

void PhantomSelectionWidget::importHelmholtzPhantoms()
{
	std::vector<phantom> phantoms;
	phantoms.reserve(10);
	phantoms.push_back({ {0.85, 0.85, 4.0}, {267, 138, 142}, {1,0,0,0,1,0}, "babynew_May2003", "Baby", false, true, true });
	phantoms.push_back({ {1.54, 1.54, 8.0}, {256, 256, 144}, {1,0,0,0,1,0}, "segm_child", "Child" ,false, false, true });
	phantoms.push_back({ {1.875, 1.875, 10.0}, {256, 256, 179}, {1,0,0,0,1,0}, "segm_donna", "Donna", false, true, false });
	phantoms.push_back({ {0.742, 0.742, 5.0}, {512, 512, 193}, {1,0,0,0,1,0}, "segm_frank", "Frank",false, true, false });
	phantoms.push_back({ {2.08, 2.08, 8.0}, {256, 256, 220}, {1,0,0,0,1,0}, "segm_golem", "Golem",false, false, true });
	phantoms.push_back({ {0.98, 0.98, 10.0}, {512, 512, 114}, {1,0,0,0,1,0}, "segm_helga", "Helga",false, true, false });
	phantoms.push_back({ {1.875, 1.875, 5.0}, {262, 132, 348}, {1,0,0,0,1,0}, "Irene", "Irene",false, true, false });
	phantoms.push_back({ {1.875, 1.875, 10.0}, {226, 118, 136}, {1,0,0,0,1,0}, "Jo", "Jo",false, true, false });
	phantoms.push_back({ {1.775, 1.775, 4.84}, {299, 150, 348}, {1,0,0,0,1,0}, "Katja", "Katja",false, false, false });
	phantoms.push_back({ {0.91, 0.94, 5.0}, {512, 512, 250}, {1,0,0,0,1,0}, "segm_vishum", "Vishum",false, true, false });

	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
	QString folderPathSaved = settings.value("phantomimport/browsepath").value<QString>();

	QString phantomFilter = "Phantoms (";
	for (const auto & p: phantoms)
		phantomFilter += " " + QString::fromStdString(p.read);
	phantomFilter += ")";
	
	auto filePath = QFileDialog::getOpenFileName(this, tr("Select folder containing Helmoltz-Zentrum phantoms"), folderPathSaved, phantomFilter);

	if (!filePath.isEmpty())
	{
		QFileInfo fileInfo(filePath);
		settings.setValue("phantomimport/browsepath", fileInfo.absolutePath());
		auto filename = fileInfo.fileName().toStdString();
		for (const auto& p : phantoms)
		{
			if (filename.compare(p.read) == 0)
			{
				auto filePathIn = filePath.toStdString();
				QDir outDir(QDir::currentPath() + "/resources/phantoms/other/");
				auto filePathOut = outDir.absoluteFilePath(QString::fromStdString(p.save)).toStdString();
				auto size = p.d[0] * p.d[1] * p.d[2];
				auto a = readArray(filePathIn, size);
				auto h = createHeader(p.d, p.s, p.cosines);
				if (p.reverseX)
					a = reverseX(a, p.d);
				if (p.reverseY)
					a = reverseY(a, p.d);
				if (p.reverseZ)
					a = reverseZ(a, p.d);
				writeArray(h, a, filePathOut);
				addInstalledPhantoms();
				return;
			}
		}
	}
}

void PhantomSelectionWidget::addInstalledPhantoms(void)
{
	QMap<QString, QString> phantoms;
	QDir path(QDir::currentPath() + "/resources/phantoms/other/");
	phantoms[tr("Katja (pregnant female)")] = path.absoluteFilePath("Katja");
	phantoms[tr("Baby")] = path.absoluteFilePath("Baby");
	phantoms[tr("Child")] = path.absoluteFilePath("Child");
	phantoms[tr("Donna")] = path.absoluteFilePath("Donna");
	phantoms[tr("Frank")] = path.absoluteFilePath("Frank");
	phantoms[tr("Golem")] = path.absoluteFilePath("Golem");
	phantoms[tr("Helga")] = path.absoluteFilePath("Helga");
	phantoms[tr("Irene")] = path.absoluteFilePath("Irene");
	phantoms[tr("Jo")] = path.absoluteFilePath("Jo");
	phantoms[tr("Vishum")] = path.absoluteFilePath("Vishum");
	
	auto i = phantoms.begin();
	while (i != phantoms.end())
	{
		if (path.exists(i.value()))
		{
			//finding phantom
			int hasItem = m_phantomSelector->findText(i.key());
			if (hasItem == -1)
				m_phantomSelector->addItem(i.key());
		}
		++i;
	}
}
