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

#include <icrpphantomimportwidget.hpp>

#include <QCheckbox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include <array>

struct Phantom {
    std::array<double, 3> spacing = { 1, 1, 1 };
    std::array<std::size_t, 3> dimensions = { 1, 1, 1 };
    QString name;
    QString filePrefix;
    QString folderPath;

    QString organDefinitionPath(QString basepath) const
    {
        QDir dir(basepath);
        if (dir.cd(folderPath)) {
            return dir.absoluteFilePath(filePrefix + QStringLiteral("organs.dat"));
        }
        return QString {};
    }
    QString mediaDefinitionPath(QString basepath) const
    {
        QDir dir(basepath);
        if (dir.cd(folderPath)) {
            return dir.absoluteFilePath(filePrefix + QStringLiteral("media.dat"));
        }
        return QString {};
    }
    QString organArrayPath(QString basepath) const
    {
        QDir dir(basepath);
        if (dir.cd(folderPath)) {
            return dir.absoluteFilePath(filePrefix + QStringLiteral("binary.dat"));
        }
        return QString {};
    }
};

static std::array<Phantom, 12> phantoms()
{
    std::array<Phantom, 12> p;
    p[0] = { .spacing = { 2.137, 2.137, 8 },
        .dimensions = { 254, 127, 222 },
        .name = "ICRP Adult Male",
        .filePrefix = QStringLiteral("AM_"),
        .folderPath = QStringLiteral("AM") };

    p[1] = { .spacing = { 1.775, 1.775, 4.84 },
        .dimensions = { 299, 137, 348 },
        .name = "ICRP Adult Female",
        .filePrefix = QStringLiteral("AF_"),
        .folderPath = QStringLiteral("AF") };

    p[2] = { .spacing = { 1.25, 1.25, 2.832 },
        .dimensions = { 407, 225, 586 },
        .name = "ICRP 15 year Male",
        .filePrefix = QStringLiteral("15M_"),
        .folderPath = QStringLiteral("15M") };

    p[3] = { .spacing = { 1.25, 1.25, 2.832 },
        .dimensions = { 401, 236, 571 },
        .name = "ICRP 15 year Female",
        .filePrefix = QStringLiteral("15F_"),
        .folderPath = QStringLiteral("15F") };

    p[4] = { .spacing = { .99, .99, 2.425 },
        .dimensions = { 419, 226, 576 },
        .name = "ICRP 10 year Male",
        .filePrefix = QStringLiteral("10M_"),
        .folderPath = QStringLiteral("10M") };

    p[5] = { .spacing = { .99, .99, 2.425 },
        .dimensions = { 419, 226, 576 },
        .name = "ICRP 10 year Female",
        .filePrefix = QStringLiteral("10F_"),
        .folderPath = QStringLiteral("10F") };

    p[6] = { .spacing = { .85, .85, 1.928 },
        .dimensions = { 419, 230, 572 },
        .name = "ICRP 5 year Male",
        .filePrefix = QStringLiteral("05M_"),
        .folderPath = QStringLiteral("05M") };

    p[7] = { .spacing = { .85, .85, 1.928 },
        .dimensions = { 419, 230, 572 },
        .name = "ICRP 5 year Female",
        .filePrefix = QStringLiteral("05F_"),
        .folderPath = QStringLiteral("05F") };

    p[8] = { .spacing = { .663, .633, 1.4 },
        .dimensions = { 393, 248, 546 },
        .name = "ICRP 1 year Male",
        .filePrefix = QStringLiteral("01M_"),
        .folderPath = QStringLiteral("01M") };

    p[9] = { .spacing = { .663, .633, 1.4 },
        .dimensions = { 393, 248, 546 },
        .name = "ICRP 1 year Female",
        .filePrefix = QStringLiteral("01F_"),
        .folderPath = QStringLiteral("01F") };

    p[10] = { .spacing = { .663, .663, .663 },
        .dimensions = { 345, 211, 716 },
        .name = "ICRP newborn Male",
        .filePrefix = QStringLiteral("00M_"),
        .folderPath = QStringLiteral("00M") };

    p[11] = { .spacing = { .663, .663, .663 },
        .dimensions = { 345, 211, 716 },
        .name = "ICRP newborn Female",
        .filePrefix = QStringLiteral("00F_"),
        .folderPath = QStringLiteral("00F") };
    return p;
}

ICRPPhantomImportWidget::ICRPPhantomImportWidget(QWidget* parent)
    : QWidget(parent)
{
    auto lay = new QVBoxLayout;
    setLayout(lay);

    const auto exeDirPath = QCoreApplication::applicationDirPath();

    // phantom selector
    auto p_box = new QGroupBox(tr("Select ICRP phantom to import"), this);
    auto p_lay = new QVBoxLayout;
    p_box->setLayout(p_lay);
    auto p_combo = new QComboBox(p_box);
    p_lay->addWidget(p_combo);
    p_combo->addItem(tr("Select phantom to import"));
    for (const auto& p : phantoms()) {
        p_combo->addItem(p.name);
    }
    connect(p_combo, &QComboBox::currentIndexChanged, [this, exeDirPath](int index) {
        if (index == 0)
            return;
        auto pIdx = index - 1;
        const auto& p = phantoms()[pIdx];
        const auto& d = p.dimensions;
        const auto& s = p.spacing;

        QDir phantoms_path(exeDirPath);
        phantoms_path.cd("data");
        phantoms_path.cd("phantoms");
        phantoms_path.cd("icrp");
        auto arr = p.organArrayPath(phantoms_path.absolutePath());
        auto organs = p.organDefinitionPath(phantoms_path.absolutePath());
        auto media = p.mediaDefinitionPath(phantoms_path.absolutePath());
        emit this->requestImportPhantom(arr, organs, media, s[0], s[1], s[2], d[0], d[1], d[2]);
    });
    lay->addWidget(p_box);

    auto arm_box = new QGroupBox(tr("Remove arms"), this);
    arm_box->setCheckable(true);
    arm_box->setChecked(false);
    connect(arm_box, &QGroupBox::toggled, this, &ICRPPhantomImportWidget::setRemoveArms);
    auto arm_lay = new QVBoxLayout;
    arm_box->setLayout(arm_lay);
    auto arm_label = new QLabel(tr("Replace arms on phantoms with air."), arm_box);
    arm_lay->addWidget(arm_label);
    lay->addWidget(arm_box);
    lay->addStretch(100);
}
