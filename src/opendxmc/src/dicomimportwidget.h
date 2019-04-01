#pragma once

#include "material.h"

#include <QWidget>
#include <QString>
#include <QStringList>
#include <QComboBox>
#include <QLineEdit>
#include <vtkSmartPointer.h>
#include <vtkDICOMDirectory.h>

#include <vector>
#include <array>


class DicomImportWidget : public QWidget
{
	Q_OBJECT
public:
	DicomImportWidget(QWidget *parent = nullptr);

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


	QLineEdit *m_browseLineEdit;
	vtkSmartPointer<vtkDICOMDirectory> m_imageDirectorySnooper;
	QComboBox *m_seriesSelector;

	std::array<double, 3> m_outputSpacing = { 2, 2, 2 };
	std::array<double, 3> m_blurRadius = { 1, 1, 1 };
	bool m_useOutputSpacing = false;
};
