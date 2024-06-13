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

Copyright 2024 Erlend Andersen
*/

#pragma once

#include <QComboBox>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>

#include <vtkDICOMDirectory.h>
#include <vtkSmartPointer.h>

#include <array>

#include <basesettingswidget.hpp>

class CTDicomImportWidget : public BaseSettingsWidget {
    Q_OBJECT
public:
    CTDicomImportWidget(QWidget* parent = nullptr);
    void setImportProgress(int current, int total, QString fmt = "%p%");

signals:
    void dicomFolderSelectedForBrowsing(QString folderPath);
    void dicomSeriesActivated(const QStringList& filePaths);
    void blurRadiusChanged(const double*);
    void outputSpacingChanged(const double*);
    void useOutputSpacingChanged(bool value);
    void useOrganSegmentator(bool value);
    void aqusitionVoltageChanged(double voltage);
    void aqusitionAlFiltrationChanged(double mm);
    void aqusitionSnFiltrationChanged(double mm);
    void segmentationMaterialsChanged();
    void requestCancelSegmentation();

private:
    void browseForFolder(void);
    void lookInFolder(void);
    void lookInFolder(QString folderPath);
    void seriesActivated(int index);

    QLineEdit* m_browseLineEdit;
    vtkSmartPointer<vtkDICOMDirectory> m_imageDirectorySnooper = nullptr;
    QComboBox* m_seriesSelector;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_cancelSegmentationButton = nullptr;
    std::array<double, 3> m_outputSpacing = { 2, 2, 2 };
    const std::array<double, 3> m_outputSpacingSegmentator = { 1.5, 1.5, 1.5 };
    std::array<double, 3> m_blurRadius = { 1, 1, 0 };
    bool m_useOutputSpacing = false;
    bool m_useOrganSegmentator = false;
};
