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
#include <QComboBox>
#include <QGroupBox>

PhantomSelectionWidget::PhantomSelectionWidget(QWidget *parent)
: QWidget(parent)
{
	auto mainLayout = new QVBoxLayout();


	auto selectLayout = new QHBoxLayout(this);
	auto selectionBox = new QGroupBox(tr("Select voxel phantom"), this);
	auto phantomSelector = new QComboBox(this);
	selectLayout->addWidget(phantomSelector);
	selectionBox->setLayout(selectLayout);

	phantomSelector->addItem(tr(""));
	phantomSelector->addItem(tr("CTDI Phantom 320 mm"));
	phantomSelector->addItem(tr("CTDI Phantom 160 mm"));
	phantomSelector->addItem(tr("Female ICRU reference phantom"));
	phantomSelector->addItem(tr("Female ICRU reference phantom without arms"));
	phantomSelector->addItem(tr("Male ICRU reference phantom"));
	phantomSelector->addItem(tr("Male ICRU reference phantom without arms"));
	phantomSelector->addItem(tr("Pregnant female"));
	phantomSelector->addItem(tr("Baby"));
	phantomSelector->addItem(tr("Child"));

	//connecting slots
	connect(phantomSelector, QOverload<int>::of(&QComboBox::activated),
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
		else if (index == 7)
			emit this->readAWSPhantom("Katja");
		else if (index == 8)
			emit this->readAWSPhantom("Babynew");
		else if (index == 9)
			emit this->readAWSPhantom("Child");
	});


	mainLayout->addWidget(selectionBox);

}

