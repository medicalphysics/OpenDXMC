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

#include <QCompleter>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

#include <vtkDICOMItem.h>
#include <vtkDICOMTag.h>
#include <vtkDICOMValue.h>
#include <vtkMatrix4x4.h>
#include <vtkStringArray.h>

#include <string>

#include <ctdicomimportwidget.hpp>

CTDicomImportWidget::CTDicomImportWidget(QWidget* parent)
    : BaseSettingsWidget(parent)
{
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    auto* mainlayout = new QVBoxLayout;
    auto* browseLayout = new QHBoxLayout;
    // setting ut folder line edit with completer and memory of last folderpath
    m_browseLineEdit = new QLineEdit(this);
    m_browseLineEdit->setClearButtonEnabled(true); // enable clear button in field
    connect(this, &CTDicomImportWidget::dicomFolderSelectedForBrowsing, m_browseLineEdit, &QLineEdit::setText);
    browseLayout->addWidget(m_browseLineEdit);
    auto browseCompleter = new QCompleter(this);
    auto browseCompleterModel = new QFileSystemModel(this);
    browseCompleterModel->setRootPath("");
    browseCompleterModel->setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    browseCompleter->setModel(browseCompleterModel);
    browseCompleter->setCompletionMode(QCompleter::InlineCompletion);
    connect(this, &CTDicomImportWidget::dicomFolderSelectedForBrowsing, browseCompleter, &QCompleter::setCompletionPrefix);
    m_browseLineEdit->setCompleter(browseCompleter);
    m_browseLineEdit->setText(settings.value("dicomimport/browsepath").value<QString>());
    connect(m_browseLineEdit, &QLineEdit::returnPressed, this, static_cast<void (CTDicomImportWidget::*)(void)>(&CTDicomImportWidget::lookInFolder));

    auto* browseLineFolderSelectButton = new QPushButton(tr("Browse"), this);
    connect(browseLineFolderSelectButton, &QPushButton::clicked, this, &CTDicomImportWidget::browseForFolder);
    browseLayout->addWidget(browseLineFolderSelectButton);
    browseLineFolderSelectButton->setFixedHeight(m_browseLineEdit->sizeHint().height());

    auto browseBox = new QGroupBox(tr("Select folder to scan for DICOM series"), this);
    browseBox->setLayout(browseLayout);

    // series picker
    auto* seriesSelectorLayout = new QVBoxLayout;
    m_seriesSelector = new QComboBox(this);
    m_seriesSelector->setDuplicatesEnabled(true);
    seriesSelectorLayout->addWidget(m_seriesSelector);
    connect(m_seriesSelector, static_cast<void (QComboBox::*)(int)>(&QComboBox::activated), this, &CTDicomImportWidget::seriesActivated);
    auto seriesSelectorBox = new QGroupBox(tr("Select CT series to be imported"), this);
    seriesSelectorBox->setLayout(seriesSelectorLayout);

    // image blurfactor selection
    auto outputBlurBox = new QGroupBox(tr("Image smooth factor [XYZ]:"), this);
    auto outputBlurLayoutButtons = new QHBoxLayout;
    for (int i = 0; i < 3; i++) {
        auto spinBox = new QDoubleSpinBox(this);
        spinBox->setMinimum(0.0);
        spinBox->setSuffix(" voxels");
        spinBox->setValue(m_blurRadius[i]);
        connect(spinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this, i](auto value) {
            this->m_blurRadius[i] = value;
            emit blurRadiusChanged(this->m_blurRadius.data());
        });
        outputBlurLayoutButtons->addWidget(spinBox);
    }
    outputBlurBox->setLayout(outputBlurLayoutButtons);

    auto outputSegmentatorBox = new QGroupBox(tr("Use convnet organ segmentation of CT series"), this);
    outputSegmentatorBox->setCheckable(true);
    outputSegmentatorBox->setChecked(false);
    auto outputSegmentatorLayout = new QHBoxLayout;
    auto outputSegmentatorLabel = new QLabel(tr("Attempt to segment CT images into various organs (about 60). Voxel size on imported series is forced to be 1.5 mm isotropic. This is quite computationally demanding, expect at least 10 minute processing time on a decent computer."));
    outputSegmentatorLabel->setWordWrap(true);
    outputSegmentatorLayout->addWidget(outputSegmentatorLabel);
    outputSegmentatorBox->setLayout(outputSegmentatorLayout);

    // voxel resize selection
    auto outputSpacingBox = new QGroupBox(tr("Resize voxels to this spacing for imported series [XYZ]:"), this);
    outputSpacingBox->setCheckable(true);
    outputSpacingBox->setChecked(false);
    connect(outputSpacingBox, &QGroupBox::toggled, [this](bool value) { emit useOutputSpacingChanged(value); });
    auto outputSpacingLayoutButtons = new QHBoxLayout;
    QDoubleSpinBox* ouputSpacingSpinBoxes[3];

    for (int i = 0; i < 3; ++i) {
        auto spinBox = new QDoubleSpinBox(outputSpacingBox);
        ouputSpacingSpinBoxes[i] = spinBox;
        spinBox->setMinimum(0.1);
        spinBox->setSuffix(" mm");
        spinBox->setValue(m_outputSpacing[i]);
        connect(spinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this, i](auto value) {
            this->m_outputSpacing[i] = value;
            emit outputSpacingChanged(this->m_outputSpacing.data());
        });
        outputSpacingLayoutButtons->addWidget(spinBox);
    }
    outputSpacingBox->setLayout(outputSpacingLayoutButtons);

    connect(outputSegmentatorBox, &QGroupBox::toggled, [=, this](bool value) {
        outputSpacingBox->setChecked(value);
        outputSpacingBox->setDisabled(value);
        if (value) {
            for (int i = 0; i < 3; ++i)
                ouputSpacingSpinBoxes[i]->setValue(this->m_outputSpacingSegmentator[i]);
            emit outputSpacingChanged(this->m_outputSpacingSegmentator.data());
        } else {
            for (int i = 0; i < 3; ++i)
                ouputSpacingSpinBoxes[i]->setValue(this->m_outputSpacing[i]);
            emit outputSpacingChanged(this->m_outputSpacing.data());
        }
        emit useOrganSegmentator(value);
    });

    // tube settings
    auto tubeBox = new QGroupBox(tr("Aqusition tube settings: "), this);
    auto tubeLayout = new QHBoxLayout;
    auto tubeVoltageLayout = new QVBoxLayout;
    auto tubeVoltageSpinBox = new QDoubleSpinBox(this);
    tubeVoltageSpinBox->setMinimum(70.0);
    tubeVoltageSpinBox->setMaximum(150.0);
    tubeVoltageSpinBox->setValue(120.0);
    tubeVoltageSpinBox->setSuffix(" kV");
    tubeVoltageSpinBox->setDecimals(0);
    connect(tubeVoltageSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](double val) { emit aqusitionVoltageChanged(val); });
    auto tubeVoltageLabel = new QLabel(tr("Tube voltage"), this);
    tubeVoltageLayout->addWidget(tubeVoltageLabel);
    tubeVoltageLayout->addWidget(tubeVoltageSpinBox);
    tubeLayout->addLayout(tubeVoltageLayout);
    auto tubeAlFiltrationLayout = new QVBoxLayout;
    auto tubeAlFiltrationSpinBox = new QDoubleSpinBox(this);
    tubeAlFiltrationSpinBox->setMinimum(0.0);
    tubeAlFiltrationSpinBox->setMaximum(100.0);
    tubeAlFiltrationSpinBox->setSuffix(" mm");
    tubeAlFiltrationSpinBox->setValue(9.0);
    tubeAlFiltrationSpinBox->setDecimals(1);
    connect(tubeAlFiltrationSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](double val) { emit aqusitionAlFiltrationChanged(val); });
    auto tubeAlFiltrationLabel = new QLabel(tr("Al filtration"), this);
    tubeAlFiltrationLayout->addWidget(tubeAlFiltrationLabel);
    tubeAlFiltrationLayout->addWidget(tubeAlFiltrationSpinBox);
    tubeLayout->addLayout(tubeAlFiltrationLayout);
    auto tubeSnFiltrationLayout = new QVBoxLayout;
    auto tubeSnFiltrationSpinBox = new QDoubleSpinBox(this);
    tubeSnFiltrationSpinBox->setMinimum(0.0);
    tubeSnFiltrationSpinBox->setMaximum(100.0);
    tubeSnFiltrationSpinBox->setValue(0.0);
    tubeSnFiltrationSpinBox->setSuffix(" mm");
    tubeSnFiltrationSpinBox->setDecimals(1);
    connect(tubeSnFiltrationSpinBox, static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [this](double val) { emit aqusitionSnFiltrationChanged(val); });
    auto tubeSnFiltrationLabel = new QLabel(tr("Sn filtration"), this);
    tubeSnFiltrationLayout->addWidget(tubeSnFiltrationLabel);
    tubeSnFiltrationLayout->addWidget(tubeSnFiltrationSpinBox);
    tubeLayout->addLayout(tubeSnFiltrationLayout);
    tubeBox->setLayout(tubeLayout);

    // adding progressbar for image import
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    // adding cancel button for segmentation
    m_cancelSegmentationButton = new QPushButton(tr("Cancel"), this);
    m_cancelSegmentationButton->setVisible(false);
    connect(m_cancelSegmentationButton, &QPushButton::clicked, [this](void) {
        emit requestCancelSegmentation();
        m_cancelSegmentationButton->setText(tr("Cancelling"));
        m_cancelSegmentationButton->setDisabled(true);
    });
    auto progressLayout = new QHBoxLayout;
    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_cancelSegmentationButton);

    // setting up layout
    mainlayout->addWidget(browseBox);
    mainlayout->addWidget(outputBlurBox);
    mainlayout->addWidget(outputSegmentatorBox);
    mainlayout->addWidget(outputSpacingBox);
    mainlayout->addWidget(tubeBox);
    mainlayout->addWidget(seriesSelectorBox);
    mainlayout->addLayout(progressLayout);
    // mainlayout->addWidget(m_progressBar);
    mainlayout->addStretch();
    this->setLayout(mainlayout);

    m_imageDirectorySnooper = vtkSmartPointer<vtkDICOMDirectory>::New();

    connect(this, &CTDicomImportWidget::dicomFolderSelectedForBrowsing, this, static_cast<void (CTDicomImportWidget::*)(QString)>(&CTDicomImportWidget::lookInFolder));

    QDir path = QDir(settings.value("dicomimport/browsepath").value<QString>());
    if (path.exists()) {
        emit dicomFolderSelectedForBrowsing(path.absolutePath());
    }

    // signal for updating blur, spacing and aqusition settings
    QTimer::singleShot(0, [=, this](void) {
        emit this->blurRadiusChanged(this->m_blurRadius.data());
        emit this->outputSpacingChanged(this->m_outputSpacing.data());
        emit useOutputSpacingChanged(outputSpacingBox->isChecked());
        emit aqusitionAlFiltrationChanged(tubeAlFiltrationSpinBox->value());
        emit aqusitionSnFiltrationChanged(tubeSnFiltrationSpinBox->value());
        emit aqusitionVoltageChanged(tubeVoltageSpinBox->value());
    });
}

void CTDicomImportWidget::setImportProgress(int current, int total, QString fmt)
{
    if (total >= 0) {
        m_cancelSegmentationButton->setEnabled(true);
        m_cancelSegmentationButton->setText(tr("Cancel"));
        m_cancelSegmentationButton->setVisible(true);
        m_progressBar->setVisible(true);
        m_progressBar->setRange(0, total);
        m_progressBar->setValue(current);
        m_progressBar->setFormat(fmt);
    } else {
        m_cancelSegmentationButton->setVisible(false);
        m_progressBar->setVisible(false);
    }
}

void CTDicomImportWidget::browseForFolder(void)
{
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    QString lastFolderPath = settings.value("dicomimport/browsepath").value<QString>();
    QString path = QFileDialog::getExistingDirectory(this, "Select folder with dicom files", lastFolderPath, QFileDialog::ShowDirsOnly);
    if (!path.isEmpty()) {
        emit dicomFolderSelectedForBrowsing(path);
    }
}
void CTDicomImportWidget::lookInFolder(void)
{
    QString text = m_browseLineEdit->text();
    lookInFolder(text);
}
void CTDicomImportWidget::lookInFolder(const QString folderPath)
{
    m_imageDirectorySnooper->SetScanDepth(8);
    m_imageDirectorySnooper->RequirePixelDataOn();
    m_imageDirectorySnooper->SetQueryFilesToAlways();

    if (!folderPath.isEmpty()) {
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        settings.setValue("dicomimport/browsepath", folderPath);
    }

    auto cleanPath = QDir::toNativeSeparators(QDir::cleanPath(folderPath));
    m_imageDirectorySnooper->SetDirectoryName(cleanPath.toStdString().c_str());

    // resetting series selector
    m_seriesSelector->clear();
    m_seriesSelector->addItem(tr("Select series to import"));
    m_seriesSelector->setEnabled(false);

    // restricts images to axial CT images
    vtkDICOMItem query;
    query.SetAttributeValue(DC::Modality, " CT ");
    query.SetAttributeValue(DC::ImageType, " AXIAL ");
    query.SetAttributeValue(DC::SOPClassUID, " 1.2.840.10008.5.1.4.1.1.2 ");
    m_imageDirectorySnooper->SetFindQuery(query);

    m_imageDirectorySnooper->Update();

    int n_series = m_imageDirectorySnooper->GetNumberOfSeries();

    if (n_series == 0) {
        return;
    }

    vtkDICOMTag seriesDescriptionTag(8, 0x103E);
    vtkDICOMTag studyDescriptionTag(8, 0x1030);
    for (int i = 0; i < n_series; i++) {
        vtkDICOMItem seriesRecord = m_imageDirectorySnooper->GetSeriesRecord(i);
        vtkDICOMValue seriesDescriptionValue = seriesRecord.GetAttributeValue(seriesDescriptionTag);
        vtkDICOMValue studyDescriptionValue = seriesRecord.GetAttributeValue(studyDescriptionTag);

        QString descQ;
        if (studyDescriptionValue.IsValid()) {
            std::string desc = studyDescriptionValue.GetString(0);
            descQ.append(QString::fromStdString(desc));
        }
        if (seriesDescriptionValue.IsValid()) {
            std::string desc = seriesDescriptionValue.GetString(0);
            descQ.append(QString::fromStdString(desc));
        }
        if (descQ.size() == 0) {
            descQ = QString("Series ") + QString::number(i);
        }
        m_seriesSelector->addItem(descQ);
        m_seriesSelector->setEnabled(true);
    }
}

void CTDicomImportWidget::seriesActivated(int index)
{
    int n_series = m_imageDirectorySnooper->GetNumberOfSeries();
    if (index > n_series || index < 1) {
        m_seriesSelector->clear();
        m_seriesSelector->setDisabled(true);
        return;
    }

    // subtract dummy series;
    index--;

    auto fileNameArray = m_imageDirectorySnooper->GetFileNamesForSeries(index);
    int n = fileNameArray->GetNumberOfValues();
    QStringList fileNames;
    for (int i = 0; i < n; ++i) {
        fileNames.append(QString::fromStdString(fileNameArray->GetValue(i)));
    }

    emit dicomSeriesActivated(fileNames);
}
