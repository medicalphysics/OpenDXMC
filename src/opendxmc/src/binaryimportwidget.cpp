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
	
	auto browseButton = new QPushButton(tr("Browse"), this);
	mainLayout->addWidget(browseButton);
	this->setLayout(mainLayout);
}


BinaryImportWidget::BinaryImportWidget(QWidget* parent)
	:QWidget(parent)
{
	QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");

	auto mainLayout = new QVBoxLayout;

	auto materialBox = new QGroupBox(tr("Material map:"), this);
	auto materialLayout = new QVBoxLayout;
	materialBox->setLayout(materialLayout);
	auto materialFileSelect = new FileSelectWidget(this);
	materialLayout->addWidget(materialFileSelect);
	mainLayout->addWidget(materialBox);
	this->setLayout(mainLayout);
}
