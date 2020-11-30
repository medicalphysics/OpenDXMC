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

#include "opendxmc/imagecontainer.h"
#include "opendxmc/phantomimportpipeline.h"
#include <QComboBox>
#include <QString>
#include <QWidget>

#include <vector>

class PhantomSelectionWidget : public QWidget {
    Q_OBJECT
public:
    PhantomSelectionWidget(QWidget* parent = nullptr);
    void importHelmholtzPhantoms();

signals:
    void readIRCUPhantom(PhantomImportPipeline::Phantom phantom, bool removeArms = false);
    void readCTDIPhantom(int diameter_mm, bool force_measurements);
    void readAWSPhantom(const QString& name);

protected:
    void addInstalledPhantoms(void);

private:
    QComboBox* m_phantomSelector = nullptr;
};
