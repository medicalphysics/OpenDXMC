#pragma once


#include "imagecontainer.h"
#include <QWidget>
#include <vector>
#include <string>

class PhantomSelectionWidget : public QWidget
{
	Q_OBJECT
public:
	PhantomSelectionWidget(QWidget *parent = nullptr);

signals:
	void readIRCUMalePhantom(void);
	void readIRCUFemalePhantom(void);
	void readCTDIPhantom(int mm);
protected:
	
	
private:
	
	
};
