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

#include <beamsettingsmodel.hpp>

#include <functional>
#include <array>

class LabelItem : public QStandardItem {
public:
    LabelItem(const QString& txt)
        : QStandardItem(txt)
    {
        setEditable(false);
    }
};

template<typename Getter, typename Setter> 
class VectorItem : public QStandardItem {
public:
    LabelItem(std::function<void(std::array<double, 3>)> setter, std::function<std::array<double, 3>(void)> getter)
        : QStandardItem()
    {
    }
    QVariant data(Qt::DataRole role = Qt::UserRole+1) {
        if (role )
    }

protected:
    QString arrayToString(const std::array<double, 3>& arr) {
        return QString()
    }
};

BeamSettingsModel::BeamSettingsModel(QObject* parent)
    : QStandardItemModel(parent)
{
    QStringList header;
    header.append(tr("Settings"));
    header.append(tr("Value"));
    setHorizontalHeaderLabels(header);
}

void BeamSettingsModel::addDXBeam()
{

    auto root = new LabelItem(tr("DX Beam"));
    appendRow(root);

    QList<QStandardItem*> row(2);
    row[0] = new LabelItem("Test");
    row[1] = new LabelItem("Test2");
    root->appendRow(row);
}