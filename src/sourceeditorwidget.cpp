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

#include "opendxmc/sourceeditorwidget.h"
#include "opendxmc/volumeactorcontainer.h"

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemEditorFactory>
#include <QJsonArray>
#include <QJsonDocument>
#include <QKeyEvent>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QStyledItemDelegate>
#include <QVBoxLayout>

BowtieFilterReader::BowtieFilterReader(QWidget* parent)
    : QWidget(parent)
{
}

void BowtieFilterReader::addFilter(std::shared_ptr<BowTieFilter> filter)
{
    m_bowtieFilters.push_back(filter);
}

bool BowtieFilterReader::loadFilters()
{
    QFile loadFile(QStringLiteral("resources/bowtiefilters.json"));
    if (!loadFile.open(QIODevice::ReadOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }
    QByteArray saveData = loadFile.readAll();
    QJsonDocument loadDoc(QJsonDocument::fromJson(saveData));
    readJson(loadDoc.object());
    return true;
}

bool BowtieFilterReader::saveFilters()
{
    QFile saveFile(QStringLiteral("resources/bowtiefilters.json"));

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return false;
    }

    QJsonObject filtersObject;
    writeJson(filtersObject);
    QJsonDocument saveDoc(filtersObject);
    saveFile.write(saveDoc.toJson());
    return true;
}

std::shared_ptr<BowTieFilter> BowtieFilterReader::readFilter(QJsonObject& json) const
{
    std::string name {};
    std::vector<std::pair<floating, floating>> filterData;
    if (json.contains("name") && json["name"].isString())
        name = json["name"].toString().toStdString();

    if (json.contains("filterdata") && json["filterdata"].isArray()) {
        QJsonArray filtersArray = json["filterdata"].toArray();
        for (int dataIndex = 0; dataIndex < filtersArray.size(); ++dataIndex) {
            QJsonObject filterObject = filtersArray[dataIndex].toObject();
            if (filterObject.contains("angle") && filterObject["angle"].isDouble()) {
                floating angle = filterObject["angle"].toDouble();
                if (filterObject.contains("weight") && filterObject["weight"].isDouble()) {
                    floating weight = filterObject["weight"].toDouble();
                    filterData.push_back(std::make_pair(angle, weight));
                }
            }
        }
    }
    if ((name.size() == 0) || (filterData.size() == 0))
        return nullptr;
    auto filter = std::make_shared<BowTieFilter>(filterData);
    filter->setFilterName(name);
    return filter;
}

void BowtieFilterReader::writeFilter(QJsonObject& json, std::shared_ptr<BowTieFilter> filter) const
{
    json["name"] = QString::fromStdString(filter->filterName());
    QJsonArray filtersArray;
    const auto& values = filter->data();
    for (const auto& [angle, weight] : values) {
        QJsonObject filterObject;
        filterObject["angle"] = angle;
        filterObject["weight"] = weight;
        filtersArray.append(filterObject);
    }
    json["filterdata"] = filtersArray;
}

void BowtieFilterReader::readJson(const QJsonObject& json)
{
    if (json.contains("filters") && json["filters"].isArray()) {
        QJsonArray filtersArray = json["filters"].toArray();
        m_bowtieFilters.clear();
        m_bowtieFilters.reserve(filtersArray.size());
        for (int filtersIndex = 0; filtersIndex < filtersArray.size(); ++filtersIndex) {
            QJsonObject filterObject = filtersArray[filtersIndex].toObject();
            auto filter = readFilter(filterObject);
            m_bowtieFilters.push_back(filter);
        }
    }
}

void BowtieFilterReader::writeJson(QJsonObject& json) const
{
    QJsonArray filtersArray;
    for (const auto& filter : m_bowtieFilters) {
        QJsonObject filterObject;
        writeFilter(filterObject, filter);
        filtersArray.append(filterObject);
    }
    json["filters"] = filtersArray;
}

//http://doc.qt.io/qt-5/qtwidgets-itemviews-coloreditorfactory-example.html
SourceDelegate::SourceDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
    addBowtieFilter(nullptr);
    addAecFilter(nullptr);
}

QWidget* SourceDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto userType = index.data(Qt::DisplayRole).userType();
    if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>()) {
        QComboBox* cb = new QComboBox(parent);
        for (const auto& [name, filter] : m_bowtieFilters) {
            cb->addItem(name);
        }
        return cb;
    } else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>()) {
        QComboBox* cb = new QComboBox(parent);
        for (const auto& [name, filter] : m_aecFilters) {
            cb->addItem(name);
        }
        return cb;
    } else if (userType == QMetaType::Float || userType == QMetaType::Double) {
        QDoubleSpinBox* cb = new QDoubleSpinBox(parent);
        cb->setMinimum(-1.0e6);
        cb->setMaximum(1.e6);
        return cb;
    }
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void SourceDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    auto userType = index.data(Qt::DisplayRole).userType();
    if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>()) {
        auto currentFilter = qvariant_cast<std::shared_ptr<BowTieFilter>>(index.data(Qt::EditRole));
        auto cb = qobject_cast<QComboBox*>(editor);

        auto it = std::find_if(m_bowtieFilters.cbegin(), m_bowtieFilters.cend(), [=](const auto& e) { return e.second == currentFilter; });
        if (it != m_bowtieFilters.cend()) {
            int idx = std::distance(m_bowtieFilters.cbegin(), it);
            cb->setCurrentIndex(idx);
            return;
        }
    } else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>()) {
        auto currentFilter = qvariant_cast<std::shared_ptr<AECFilter>>(index.data(Qt::EditRole));
        auto cb = qobject_cast<QComboBox*>(editor);

        auto it = std::find_if(m_aecFilters.cbegin(), m_aecFilters.cend(), [=](const auto& e) { return e.second == currentFilter; });
        if (it != m_aecFilters.cend()) {
            int idx = std::distance(m_aecFilters.cbegin(), it);
            cb->setCurrentIndex(idx);
            return;
        }
    } else if (userType == QMetaType::Float || userType == QMetaType::Double) {
        auto spinBox = qobject_cast<QDoubleSpinBox*>(editor);
        const auto value = index.data(Qt::DisplayRole).toDouble();
        spinBox->setValue(value);
        return;
    }
    QStyledItemDelegate::setEditorData(editor, index);
}

void SourceDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    auto userType = index.data(Qt::DisplayRole).userType();
    if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>()) {
        auto cb = qobject_cast<QComboBox*>(editor);
        auto idx = cb->currentIndex();
        if (idx >= 0) {
            auto data = QVariant::fromValue(m_bowtieFilters[idx].second);
            model->setData(index, data, Qt::EditRole);
        } else {
            std::shared_ptr<BowTieFilter> empty = nullptr;
            model->setData(index, QVariant::fromValue(empty), Qt::EditRole);
        }
    } else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>()) {
        auto cb = qobject_cast<QComboBox*>(editor);
        auto idx = cb->currentIndex();
        if (idx >= 0) {
            auto data = QVariant::fromValue(m_aecFilters[idx].second);
            model->setData(index, data, Qt::EditRole);
        } else {
            std::shared_ptr<AECFilter> empty = nullptr;
            model->setData(index, QVariant::fromValue(empty), Qt::EditRole);
        }
    } else if (userType == QMetaType::Float || userType == QMetaType::Double) {
        auto spinBox = qobject_cast<QDoubleSpinBox*>(editor);
        const auto value = spinBox->value();
        QVariant data(value);
        model->setData(index, data, Qt::EditRole);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

QString SourceDelegate::displayText(const QVariant& value, const QLocale& locale) const
{
    auto userType = value.userType();
    if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>()) {
        auto currentFilter = qvariant_cast<std::shared_ptr<BowTieFilter>>(value);
        auto it = std::find_if(m_bowtieFilters.cbegin(), m_bowtieFilters.cend(), [=](const auto& pair) { return pair.second == currentFilter; });
        if (it != m_bowtieFilters.cend()) {
            return it->first;
        }
    } else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>()) {
        auto currentFilter = qvariant_cast<std::shared_ptr<AECFilter>>(value);
        auto it = std::find_if(m_aecFilters.cbegin(), m_aecFilters.cend(), [=](const auto& pair) { return pair.second == currentFilter; });
        if (it != m_aecFilters.cend()) {
            return it->first;
        }
    }
    return QStyledItemDelegate::displayText(value, locale);
}

void SourceDelegate::addBowtieFilter(std::shared_ptr<BowTieFilter> filter)
{
    QString name;
    if (filter) {
        name = QString::fromStdString(filter->filterName());
    } else {
        name = "None";
    }

    //Check for filter name already in buffer
    auto idx = std::find_if(m_bowtieFilters.begin(), m_bowtieFilters.end(), [=](const auto& el) { return el.first == name; });
    if (idx == m_bowtieFilters.cend()) {
        m_bowtieFilters.emplace_back(std::make_pair(name, filter));
        std::sort(m_bowtieFilters.begin(), m_bowtieFilters.end());
    } else {
        *idx = std::make_pair(name, filter);
    }
}
void SourceDelegate::addAecFilter(std::shared_ptr<AECFilter> filter)
{
    QString name;
    if (filter) {
        name = QString::fromStdString(filter->filterName());
    } else {
        name = "None";
    }

    //Check for filter name already in buffer
    auto idx = std::find_if(m_aecFilters.begin(), m_aecFilters.end(), [=](const auto& el) { return el.first == name; });
    if (idx == m_aecFilters.cend()) {
        m_aecFilters.emplace_back(std::make_pair(name, filter));
        std::sort(m_aecFilters.begin(), m_aecFilters.end());
    } else {
        *idx = std::make_pair(name, filter);
    }
}

SourceModelView::SourceModelView(QWidget* parent)
    : QTreeView(parent)
{
}
void SourceModelView::keyPressEvent(QKeyEvent* event)
{
    if ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete)) {
        auto index = selectionModel()->currentIndex();
        if (index.isValid()) {
            auto parent = index.parent(); // if top level item parent is not valid
            if (!parent.isValid()) {
                model()->removeRow(index.row());
                return;
            }
        }
    }
    QTreeView::keyPressEvent(event);
}

SourceEditWidget::SourceEditWidget(QWidget* parent)
    : QWidget(parent)
{
    qRegisterMetaType<std::vector<std::shared_ptr<Source>>>();

    auto mainLayout = new QVBoxLayout(this);
    //defining available sources
    m_sourceTypes[0] = "DX tube";
    m_sourceTypes[1] = "CT Spiral tube";
    m_sourceTypes[2] = "CT Axial tube";
    m_sourceTypes[3] = "CT Dual tube";
    m_sourceTypes[4] = "CT Topogram";

    //add source button and selector
    auto addSourceBox = new QGroupBox(tr("Add a x-ray source"), this);
    auto addSourceLayout = new QHBoxLayout(addSourceBox);
    addSourceBox->setLayout(addSourceLayout);
    auto addSourceComboBox = new QComboBox(addSourceBox);
    addSourceLayout->addWidget(addSourceComboBox);
    for (auto key : m_sourceTypes.keys())
        addSourceComboBox->addItem(m_sourceTypes[key]);
    m_currentSourceTypeSelected = addSourceComboBox->currentIndex();
    connect(addSourceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, &SourceEditWidget::setCurrentSourceTypeSelected);
    auto addSourceButton = new QPushButton(tr("Add source"), addSourceBox);
    connect(addSourceButton, &QPushButton::clicked, this, &SourceEditWidget::addCurrentSourceType);
    addSourceLayout->addWidget(addSourceButton);

    //modelView
    auto modelView = new SourceModelView(this);
    m_model = new SourceModel(this);
    modelView->setModel(m_model);
    modelView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    modelView->setAnimated(true);
    m_delegate = new SourceDelegate(this);
    modelView->setItemDelegate(m_delegate);

    ///////////// adding bowtie filters
    auto filters = new BowtieFilterReader(this);
    filters->loadFilters();

    for (auto& filter : filters->filters()) {
        m_delegate->addBowtieFilter(filter);
    }

    modelView->showColumn(2);
    mainLayout->addWidget(addSourceBox);
    mainLayout->addWidget(modelView);

    auto runBox = new QGroupBox(tr("Run simulation"), this);
    auto runBoxLayout = new QVBoxLayout(runBox);
    auto lowEnergyCorrectionLabel = new QLabel(tr("Select low energy correction mode"));

    auto lowEnergyComboBox = new QComboBox(runBox);
    lowEnergyComboBox->addItem(tr("None"));
    lowEnergyComboBox->addItem(tr("Livermore correction"));
    lowEnergyComboBox->addItem(tr("Impulse approximation"));

    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    if (settings.contains("sourceEditor/lowEnergyCorrectionMode"))
        m_lowEnergyCorrection = static_cast<std::size_t>(settings.value("sourceEditor/lowEnergyCorrectionMode").value<int>());
    lowEnergyComboBox->setCurrentIndex(m_lowEnergyCorrection);
    connect(lowEnergyComboBox, QOverload<int>::of(&QComboBox::activated), [=](int value) { 
        this->setLowEnergyCorrection(value);
        emit lowEnergyCorrectionChanged(m_lowEnergyCorrection); });
    emit lowEnergyCorrectionChanged(m_lowEnergyCorrection);

    auto labelLayout = new QHBoxLayout(runBox);
    labelLayout->addWidget(lowEnergyCorrectionLabel);
    labelLayout->addWidget(lowEnergyComboBox);
    runBoxLayout->addLayout(labelLayout);
    auto runbutton = new QPushButton(tr("Run simulation"), this);
    runBoxLayout->addWidget(runbutton);
    connect(runbutton, &QPushButton::clicked, this, &SourceEditWidget::requestRunSimulation);

    mainLayout->addWidget(runBox);

    this->setLayout(mainLayout);
}

void SourceEditWidget::setLowEnergyCorrection(int value)
{
    m_lowEnergyCorrection = std::max(std::min(value, 2), 0);
    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    settings.setValue("sourceEditor/lowEnergyCorrectionMode", m_lowEnergyCorrection);
}
void SourceEditWidget::addCurrentSourceType(void)
{
    if (m_currentSourceTypeSelected == 0) {
        // add dx beam
        m_model->addSource(Source::Type::DX);
    } else if (m_currentSourceTypeSelected == 1) {
        //add spiral beam
        m_model->addSource(Source::Type::CTSpiral);
    } else if (m_currentSourceTypeSelected == 2) {
        //add axial beam
        m_model->addSource(Source::Type::CTAxial);
    } else if (m_currentSourceTypeSelected == 3) {
        //add DE beam
        m_model->addSource(Source::Type::CTDual);
    } else if (m_currentSourceTypeSelected == 4) {
        //add topogram beam
        m_model->addSource(Source::Type::CTTopogram);
    }
}

void SourceEditWidget::requestRunSimulation(void)
{
    if (m_model) {
        auto& sources = m_model->sources();
        emit runSimulation(sources);
    }
}
