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
	void readIRCUMalePhantom(bool removeArms=false);
	void readIRCUFemalePhantom(bool removeArms = false);
	void readCTDIPhantom(int diameter_mm);
protected:
	
	
private:
	
	
};
