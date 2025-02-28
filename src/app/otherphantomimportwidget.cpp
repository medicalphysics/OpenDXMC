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

Copyright 2023 Erlend Andersen
*/

#include <otherphantomimportwidget.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <array>

OtherPhantomImportWidget::OtherPhantomImportWidget(QWidget* parent)
    : QWidget(parent)
{
    auto lay = new QVBoxLayout;
    setLayout(lay);

    // phantom selector
    auto p_box = new QGroupBox(tr("Select phantom to import"), this);
    auto p_lay = new QVBoxLayout;
    p_box->setLayout(p_lay);
    auto p_combo = new QComboBox(p_box);
    p_lay->addWidget(p_combo);
    p_combo->addItem(tr("Select phantom to import"));
    p_combo->addItem(tr("Cylinder"));
    p_combo->addItem(tr("Cube"));

    connect(p_combo, &QComboBox::activated, [this](int index) {
        if (index == 0)
            return;
        else if (index == 1) {
            emit this->requestImportPhantom(0, 0.2, 0.2, 0.2, 160, 160, 500);
        } else if (index == 2) {
            emit this->requestImportPhantom(1, 0.1, 0.1, 0.1, 200, 200, 200);
        }
    });
    lay->addWidget(p_box);

    auto hmg_box = new QGroupBox(tr("Select HMGU phantom to import"), this);
    auto hmg_lay = new QVBoxLayout;
    hmg_box->setLayout(hmg_lay);
    auto hmg_label = new QLabel(tr("HMGU could previously be licensed from Helmholtz-Zentrum. If you have one the phantoms select the raw file here. NOT IMPLEMENTED YET"), hmg_box);
    hmg_label->setWordWrap(true);
    hmg_lay->addWidget(hmg_label);
    auto hmg_button = new QPushButton(tr("Browse"), hmg_box);
    hmg_lay->addWidget(hmg_button);

    connect(hmg_button, &QPushButton::clicked, this, &OtherPhantomImportWidget::queryHMGUPhantom);

    lay->addWidget(hmg_box);
    lay->addStretch(100);
}

void OtherPhantomImportWidget::queryHMGUPhantom()
{
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");

    auto dir = settings.value("hmguimport/browsepath").value<QString>();
    auto path = QFileDialog::getOpenFileName(this, tr("Select HMGU phantom file"), dir);

    if (!path.isEmpty()) {
        settings.setValue("hmguimport/browsepath", path);
        emit requestImportHMGUPhantom(path);        
    }
}
