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

#include <QComboBox>
#include <QLineEdit>
#include <QString>
#include <QStringList>
#include <QWidget>
#include <vtkDICOMDirectory.h>
#include <vtkSmartPointer.h>

#include <array>
#include <vector>

#include "opendxmc/dxmc_specialization.h"

#ifndef Q_DECLARE_METATYPE_MATERIALVECTOR
#define Q_DECLARE_METATYPE_MATERIALVECTOR
Q_DECLARE_METATYPE(std::vector<Material>)
#endif

class DicomImportWidget : public QWidget {
    Q_OBJECT
public:
    DicomImportWidget(QWidget* parent = nullptr);

signals:
    void dicomFolderSelectedForBrowsing(QString folderPath);
    void dicomSeriesActivated(QStringList filePaths);
    void blurRadiusChanged(const double*);
    void outputSpacingChanged(const double*);
    void useOutputSpacingChanged(bool value);
    void aqusitionVoltageChanged(double voltage);
    void aqusitionAlFiltrationChanged(double mm);
    void aqusitionCuFiltrationChanged(double mm);
    void segmentationMaterialsChanged(const std::vector<Material>& materials);

private:
    void browseForFolder(void);
    void lookInFolder(void);
    void lookInFolder(QString folderPath);
    void seriesActivated(int index);

    QLineEdit* m_browseLineEdit;
    vtkSmartPointer<vtkDICOMDirectory> m_imageDirectorySnooper;
    QComboBox* m_seriesSelector;

    std::array<double, 3> m_outputSpacing = { 1, 1, 4 };
    std::array<double, 3> m_blurRadius = { 1, 1, 0 };
    bool m_useOutputSpacing = false;
};
