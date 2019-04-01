
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
	phantomSelector->addItem(tr("Female ICRU reference phantom"));
	phantomSelector->addItem(tr("Male ICRU reference phantom"));
	phantomSelector->addItem(tr("CTDI Phantom 320 mm"));
	phantomSelector->addItem(tr("CTDI Phantom 160 mm"));

	//connecting slots
	connect(phantomSelector, QOverload<int>::of(&QComboBox::activated),
		[=](int index) { 
		if (index == 1) 
			emit this->readIRCUFemalePhantom(); 
		else if (index == 2) 
			emit this->readIRCUMalePhantom(); 
		else if (index == 3)
			emit this->readCTDIPhantom(320);
		else if (index == 4)
			emit this->readCTDIPhantom(160);


	});


	mainLayout->addWidget(selectionBox);

}

