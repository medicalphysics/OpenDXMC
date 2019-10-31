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


#include "binaryimportwidget.h"



#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSettings>
#include <QCompleter>
#include <QFileSystemModel>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QFileDialog>

FileSelectWidget::FileSelectWidget(QWidget* parent)
	:QWidget(parent)
{
	auto* mainLayout = new QHBoxLayout;

	m_lineEdit = new QLineEdit(this);
	mainLayout->addWidget(m_lineEdit);
	
	m_lineEdit->setClearButtonEnabled(true);
	
	auto completer = new QCompleter(this);
	auto completerModel = new QFileSystemModel(this);
	completerModel->setRootPath("");
	completerModel->setFilter(QDir::NoDotAndDotDot | QDir::Files);
	completer->setModel(completerModel);
	completer->setCompletionMode(QCompleter::InlineCompletion);
	m_lineEdit->setCompleter(completer);
	
	connect(m_lineEdit, &QLineEdit::textChanged, this, &FileSelectWidget::pathChanged);

	auto browseButton = new QPushButton(tr("Browse"), this);
	connect(browseButton, &QPushButton::clicked, [=](void) {
		auto path = QFileDialog::getOpenFileName(this, tr("Select binary file"), ".");
		if (!path.isEmpty())
		{
			completerModel->setRootPath(path);
			m_lineEdit->setText(path);
		}
		});
	mainLayout->addWidget(browseButton);
	this->setLayout(mainLayout);
}


DimensionSpacingWidget::DimensionSpacingWidget(QWidget* parent, const std::array<double, 3>& spacing, const std::array<std::size_t, 3> dimensions)
	:QWidget(parent), m_dimension(dimensions), m_spacing(spacing)
{
	auto mainLayout = new QVBoxLayout;
	mainLayout->setContentsMargins(0, 0, 0, 0);
	auto sLayout = new QHBoxLayout;
	auto dLayout = new QHBoxLayout;

	for (int i = 0; i < 3; ++i)
	{
		auto dim = new QSpinBox(this);
		dim->setSuffix(" px");
		dim->setMinimum(1);
		dim->setMaximum(2048);
		dim->setValue(static_cast<int>(m_dimension[i]));
		dLayout->addWidget(dim);
		connect(dim, QOverload<int>::of(&QSpinBox::valueChanged), [=](int value) {
			this->m_dimension[i] = static_cast<std::size_t>(value);
			emit dimensionChanged(m_dimension);
			});
	}
	for (int i = 0; i < 3; ++i)
	{
		auto sp = new QDoubleSpinBox(this);
		sp->setSuffix(" mm");
		sp->setMinimum(0.0001);
		sp->setValue(m_spacing[i]);
		sLayout->addWidget(sp);
		connect(sp, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value) {
			this->m_spacing[i] = value; 
			emit spacingChanged(m_spacing);
			});
	}
	mainLayout->addWidget(new QLabel(tr("Dimensions (X Y Z):"), this));
	mainLayout->addLayout(dLayout);
	mainLayout->addWidget(new QLabel(tr("Spacing (X Y Z):"), this));
	mainLayout->addLayout(sLayout);
	mainLayout->addStretch();
	this->setLayout(mainLayout);
}

BinaryImportWidget::BinaryImportWidget(QWidget* parent)
	:QWidget(parent)
{
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");

	auto mainLayout = new QVBoxLayout;

	// dimensions and spacing
	auto dsBox = new QGroupBox(tr("Dimensions and spacing"), this);
	auto dsLayout = new QHBoxLayout;
	dsBox->setLayout(dsLayout);
	m_dsWidget = new DimensionSpacingWidget(this);
	dsLayout->addWidget(m_dsWidget);
	mainLayout->addWidget(dsBox);

	connect(m_dsWidget, &DimensionSpacingWidget::dimensionChanged, this, &BinaryImportWidget::dimensionChanged);
	connect(m_dsWidget, &DimensionSpacingWidget::spacingChanged, this, &BinaryImportWidget::spacingChanged);

	// material array
	auto materialBox = new QGroupBox(tr("Materials array:"), this);
	auto materialLayout = new QVBoxLayout;
	materialBox->setLayout(materialLayout);
	auto materialDescription = new QLabel(tr("Select binary material array. The material array must be a binary file consisting of one 8 bit number per index (type of unsigned char or int8). This supports up to 255 materials, note that 0 is reserved for air. The size of the array must be dimension_x * dimension_y * dimension_z bytes. The array is read in standard C-style, meaning the first index is varying most."), this);
	materialDescription->setWordWrap(true);
	materialLayout->addWidget(materialDescription);
	auto materialFileSelect = new FileSelectWidget(this);
	materialLayout->addWidget(materialFileSelect);
	mainLayout->addWidget(materialBox);
	connect(materialFileSelect->getLineEditWidget(), &QLineEdit::textChanged, this, &BinaryImportWidget::materialArrayPathChanged);

	//material map
	auto materialMapBox = new QGroupBox(tr("Materials map file:"), this);
	auto materialMapLayout = new QVBoxLayout;
	materialMapBox->setLayout(materialMapLayout);
	auto materialMapDescription = new QLabel(tr("Select material map file. The material map file must be a comma (',') separated text file with material ID, name, composition.  ID must match values in the material array. Material composition must be either atomic number or a chemical composition.  Chemical formulas may contain (nested) brackets, followed by an integer number or real number (with a dot) subscript indicating relative number fraction. Examples of accepted formulas are: 'H2O', 'Ca5(PO4)3F', 'Ca5(PO4)F0.33Cl0.33(OH)0.33'. Example of content in a such file is shown below:\n0, Air, N0.75O0.24Ar0.01\n1, Water, H2O\n3, PMMA, C0.3O0.13H0.53"), this);
	materialMapDescription->setWordWrap(true);
	materialMapLayout->addWidget(materialMapDescription);
	auto materialMapFileSelect = new FileSelectWidget(this);
	materialMapLayout->addWidget(materialMapFileSelect);
	mainLayout->addWidget(materialMapBox);
	connect(materialMapFileSelect->getLineEditWidget(), &QLineEdit::textChanged, this, &BinaryImportWidget::materialMapPathChanged);

	// desnity array
	auto densityBox = new QGroupBox(tr("Density array:"), this);
	auto densityLayout = new QVBoxLayout;
	densityBox->setLayout(densityLayout);
	auto densityDescription = new QLabel(tr("Select binary density array. The density array must be a binary file consisting of one 64 bit number per index (type of double). The size of the array must be dimension_x * dimension_y * dimension_z * 8 bytes. The array is read in standard C-style, meaning the first index is varying most."), this);
	densityDescription->setWordWrap(true);
	densityLayout->addWidget(densityDescription);
	auto densityFileSelect = new FileSelectWidget(this);
	densityLayout->addWidget(densityFileSelect);
	mainLayout->addWidget(densityBox);
	connect(densityFileSelect->getLineEditWidget(), &QLineEdit::textChanged, this, &BinaryImportWidget::densityArrayPathChanged);

	m_errorTxt = new QLabel(this);
	m_errorTxt->setWordWrap(true);
	mainLayout->addWidget(m_errorTxt);

	mainLayout->addStretch();
	this->setLayout(mainLayout);
}

void BinaryImportWidget::setErrorMessage(const QString& message)
{
	if (m_errorTxt)
	{
		m_errorTxt->setText(message);
	}
}
