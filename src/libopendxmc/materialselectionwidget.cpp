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

#include "opendxmc/materialselectionwidget.h"
#include "opendxmc/colormap.h"

#include <QBrush>
#include <QCompleter>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStringList>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVBoxLayout>

MaterialTableModel::MaterialTableModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    m_header.append(tr("Color"));
    m_header.append(tr("Name"));
    m_header.append(tr("Density [g/cm3]"));
    m_header.append("Remove");

    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    settings.beginGroup("material/materials");
    QStringList keys = settings.allKeys();

    for (auto k : keys) {
        addMaterial(k);
    }
}
bool MaterialTableModel::addMaterial(int atomicNumber)
{
    auto mat = Material(atomicNumber);
    return addMaterial(mat);
}
bool MaterialTableModel::addMaterial(const QString& materialName)
{
    auto stdString = materialName.toStdString();
    auto mat = Material(stdString);
    return addMaterial(mat);
}
bool MaterialTableModel::addMaterial(const Material& material)
{
    if (material.isValid()) {
        int row = this->rowCount();
        beginInsertRows(QModelIndex(), row, row + 1);
        m_materials.push_back(material);
        endInsertRows();

        QList<QPersistentModelIndex> changedList; // this is empty since the whole model is changed
        emit layoutAboutToBeChanged(changedList);
        std::sort(m_materials.begin(), m_materials.end(), [=](const Material& a, const Material& b) { return a.standardDensity() < b.standardDensity(); });
        emit layoutChanged(changedList);
        emit materialsChanged(true);
        return true;
    }
    return false;
}
bool MaterialTableModel::insertRows(int position, int rows, const QModelIndex& parent)
{
    beginInsertRows(QModelIndex(), position, position + rows - 1);
    for (int row = 0; row < rows; ++row) {
        m_materials.emplace_back(Material());
    }
    endInsertRows();
    emit materialsChanged(true);
    return true;
}

bool MaterialTableModel::removeRows(int position, int rows, const QModelIndex& parent)
{
    beginRemoveRows(QModelIndex(), position, position + rows - 1);

    for (int row = 0; row < rows; ++row) {
        auto beg = m_materials.begin() + position;
        auto end = beg + rows;
        m_materials.erase(beg, end);
    }
    endRemoveRows();
    emit materialsChanged(true);
    return true;
}

QVariant MaterialTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();

    if (orientation == Qt::Horizontal)
        return QVariant(m_header[section]);
    return QVariant();
}

int MaterialTableModel::rowCount(const QModelIndex& parent) const
{
    return static_cast<int>(m_materials.size());
}

int MaterialTableModel::columnCount(const QModelIndex& parent) const
{
    return m_header.count();
}

Qt::ItemFlags MaterialTableModel::flags(const QModelIndex& index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;
    if (index.column() == 3)
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    if (index.column() == 2)
        return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;
    return QAbstractItemModel::flags(index);
}

QVariant MaterialTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() >= rowCount())
        return QVariant();
    if (index.column() >= columnCount())
        return QVariant();

    if (role == Qt::DisplayRole) {
        if (auto row = index.row(); index.column() == 1)
            return QVariant(QString::fromStdString(m_materials[row].name()));
        if (auto row = index.row(); index.column() == 2)
            return QVariant(m_materials[row].standardDensity());
        if (index.column() == 3)
            return QVariant(false);
    }
    if (role == Qt::EditRole) {
        if (index.column() == 1)
            return QVariant(QString::fromStdString(m_materials[index.row()].name()));
        if (index.column() == 2)
            return QVariant(m_materials[index.row()].standardDensity());
        if (index.column() == 3)
            return QVariant(false);
    }
    if (role == Qt::BackgroundRole) {
        if (index.column() == 0)
            return QVariant(QBrush(getQColor(index.row())));
        else {
            if (!m_materials[index.row()].isValid())
                return QVariant(QBrush(Qt::red));
        }
    }
    return QVariant();
}

bool MaterialTableModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        if (index.column() == 2) {
            auto dens = value.toDouble();
            if (dens > 0.0) {
                m_materials[index.row()].setStandardDensity(dens);
                emit dataChanged(index, index, { role });
                emit materialsChanged(true);
                return true;
            }
        }
        if (index.column() == 3) {
            bool remove = value.toBool();
            if (remove) {
                removeRows(index.row(), 1);
                emit materialsChanged(true);
                return false;
            }
        }
    }
    return false;
}

MaterialSelectionWidget::MaterialSelectionWidget(QWidget* parent)
    : QWidget(parent)
{
    qRegisterMetaType<std::vector<Material>>();

    m_tableModel = new MaterialTableModel(this);
    auto mainLayout = new QVBoxLayout(this);

    //line edit with completer and OK button and density field
    m_materialNameEdit = new QLineEdit(this);
    auto nistCompoundNames = Material::getNISTCompoundNames();
    QStringList nistCompoundStrings;
    for (const auto& mat_name : nistCompoundNames)
        nistCompoundStrings.append(QString::fromStdString(mat_name));
    // adding elements
    for (int i = 1; i < 104; ++i)
        nistCompoundStrings.append(QString::fromStdString(Material::getSymbolFromAtomicNumber(i)));
    nistCompoundStrings.sort(Qt::CaseSensitive);

    auto nameCompleter = new QCompleter(nistCompoundStrings, this);
    nameCompleter->setCompletionMode(QCompleter::UnfilteredPopupCompletion);
    nameCompleter->setFilterMode(Qt::MatchStartsWith);
    nameCompleter->setCaseSensitivity(Qt::CaseInsensitive);
    nameCompleter->setModelSorting(QCompleter::CaseSensitivelySortedModel);
    m_materialNameEdit->setCompleter(nameCompleter);
    connect(m_materialNameEdit, &QLineEdit::editingFinished, this, &MaterialSelectionWidget::getDensityFromMaterialName);

    m_materialDensityEdit = new QDoubleSpinBox(this);
    m_materialDensityEdit->setMinimum(0.0);
    m_materialDensityEdit->setDecimals(6);
    m_materialDensityEdit->setSuffix(" g/cm3");
    auto materialOKButton = new QPushButton(tr("Add material"));
    connect(materialOKButton, &QPushButton::clicked, this, &MaterialSelectionWidget::tryAddMaterial);

    auto addMaterialLayout = new QHBoxLayout();
    addMaterialLayout->addWidget(new QLabel(tr("Material name:"), this));
    addMaterialLayout->addWidget(m_materialNameEdit);
    auto addMaterialLayout2 = new QHBoxLayout();
    addMaterialLayout2->addWidget(new QLabel(tr("Density:"), this));
    addMaterialLayout2->addWidget(m_materialDensityEdit);
    addMaterialLayout2->addStretch(2);
    addMaterialLayout2->addWidget(materialOKButton);
    mainLayout->addLayout(addMaterialLayout);
    mainLayout->addLayout(addMaterialLayout2);

    m_tableView = new QTableView(this);
    m_tableView->setSortingEnabled(true);
    m_tableView->setModel(m_tableModel);
    m_tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    connect(m_tableModel, &MaterialTableModel::layoutChanged, [=](const auto& parents, auto hint) { m_tableView->resizeColumnsToContents(); });

    auto delegate = new QStyledItemDelegate(m_tableView);
    auto factory = new QItemEditorFactory();
    delegate->setItemEditorFactory(factory);
    m_tableView->setItemDelegateForColumn(3, delegate);

    mainLayout->addWidget(m_tableView);
    connect(m_tableModel, &MaterialTableModel::materialsChanged, this, &MaterialSelectionWidget::applyMaterials);

    this->setLayout(mainLayout);

    std::vector<QString> mats { "Air, Dry (near sea level)", "Bone, Compact (ICRU)", "Adipose Tissue (ICRP)", "Muscle, Skeletal",
        "Tissue, Soft (ICRP)" };
    for (auto m : mats) {
        Material mat(m.toStdString());
        if (m.compare("Bone, Compact (ICRU)") == 0) // adding more sane density for bone
            mat.setStandardDensity(1.1);
        m_tableModel->addMaterial(mat);
    }

    //trigger to send materials to simulationpipeline on startup
    QTimer::singleShot(0, this, &MaterialSelectionWidget::applyMaterials);
}

void MaterialSelectionWidget::getDensityFromMaterialName(void)
{
    auto materialName = QString(m_materialNameEdit->text()).toStdString();
    int z = Material::getAtomicNumberFromSymbol(materialName);
    Material mat;
    if (z > 0)
        mat = Material(z);
    else
        mat = Material(materialName);

    if (mat.isValid())
        m_materialDensityEdit->setValue(mat.standardDensity());
    else
        m_materialDensityEdit->setValue(0.0);
}

void MaterialSelectionWidget::tryAddMaterial(void)
{
    auto materialName = QString(m_materialNameEdit->text()).toStdString();
    int z = Material::getAtomicNumberFromSymbol(materialName);
    Material mat;
    if (z > 0)
        mat = Material(z);
    else
        mat = Material(materialName);

    double density = m_materialDensityEdit->value();
    mat.setStandardDensity(density);
    bool added = m_tableModel->addMaterial(mat);
    if (!added) {
        auto message = QString("Material ") + QString::fromStdString(mat.name()) + QString(" with density %1 is not a valid material.").arg(density);
        emit statusMessage(message, 10000);
    }
}

void MaterialSelectionWidget::applyMaterials()
{
    auto materials = m_tableModel->getMaterials();
    emit materialsChanged(materials);
}