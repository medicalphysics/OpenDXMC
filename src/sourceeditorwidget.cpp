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

#include <QStyledItemDelegate>
#include <QItemEditorFactory>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QKeyEvent>

#include <QHeaderView>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>

BowtieFilterReader::BowtieFilterReader(QWidget *parent)
	:QWidget(parent)
{

}


void BowtieFilterReader::addFilter(const QString & name, std::shared_ptr<BowTieFilter>& filter)
{
	m_bowtieFilters.push_back(std::make_pair(name, filter));
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

std::pair<QString, std::shared_ptr<BowTieFilter>> BowtieFilterReader::readFilter(QJsonObject & json) const
{
	QString name;
	std::vector<std::pair<double, double>> filterData;
	if (json.contains("name") && json["name"].isString())
		name = json["name"].toString();
		
	if (json.contains("filterdata") && json["filterdata"].isArray()) {
		QJsonArray filtersArray = json["filterdata"].toArray();
		for (int dataIndex = 0; dataIndex < filtersArray.size(); ++dataIndex) {
			QJsonObject filterObject = filtersArray[dataIndex].toObject();
			if (filterObject.contains("angle") && filterObject["angle"].isDouble()) {
				double angle = filterObject["angle"].toDouble();
				if (filterObject.contains("weight") && filterObject["weight"].isDouble()) {
					double weight = filterObject["weight"].toDouble();
					filterData.push_back(std::make_pair(angle, weight));
				}
			}
		}
	}
	if ((name.size() == 0) || (filterData.size() == 0))
		return std::pair<QString, std::shared_ptr<BowTieFilter>>();
	auto filter = std::make_shared<BowTieFilter>(filterData);
	return std::make_pair(name, filter);
}

void BowtieFilterReader::writeFilter(QJsonObject & json, const std::pair<QString, std::shared_ptr<BowTieFilter>>& filter) const
{
	json["name"] = filter.first;
	QJsonArray filtersArray;
	auto values = filter.second->data();
	for (auto[angle, weight] : values)
	{
		QJsonObject filterObject;
		filterObject["angle"] = angle;
		filterObject["weight"] = weight;
		filtersArray.append(filterObject);
	}
	json["filterdata"] = filtersArray;
}

void BowtieFilterReader::readJson(const QJsonObject & json)
{
	if (json.contains("filters") && json["filters"].isArray()) {
		QJsonArray filtersArray = json["filters"].toArray();
		m_bowtieFilters.clear();
		m_bowtieFilters.reserve(filtersArray.size());
		for (int filtersIndex = 0; filtersIndex < filtersArray.size(); ++filtersIndex) {
			QJsonObject filterObject = filtersArray[filtersIndex].toObject();
			m_bowtieFilters.push_back(readFilter(filterObject));
		}
	}
}

void BowtieFilterReader::writeJson(QJsonObject & json) const
{
	QJsonArray filtersArray;
	for(const auto &filter : m_bowtieFilters) {
		QJsonObject filterObject;
		writeFilter(filterObject, filter);
		filtersArray.append(filterObject);
	}
	json["filters"] = filtersArray;
}

//http://doc.qt.io/qt-5/qtwidgets-itemviews-coloreditorfactory-example.html
SourceDelegate::SourceDelegate(QObject *parent)
	: QStyledItemDelegate(parent)
{
	addBowtieFilter("None", nullptr);
	addAecFilter("None", nullptr);
}



QWidget *SourceDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	auto userType = index.data(Qt::DisplayRole).userType();
	if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>())
	{
		QComboBox *cb = new QComboBox(parent);
		for (auto &[name, filter] : m_bowtieFilters)
		{
			cb->addItem(name);
		}
		return cb;
	}
	else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>())
	{
		QComboBox *cb = new QComboBox(parent);
		for (const auto &[name, filter] : m_aecFilters)
		{
			cb->addItem(name);
		}
		return cb;
	}
	return QStyledItemDelegate::createEditor(parent, option, index);
}


void SourceDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	auto userType = index.data(Qt::DisplayRole).userType();
	if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>())
	{
		auto currentFilter = qvariant_cast<std::shared_ptr<BowTieFilter>>(index.data(Qt::EditRole));
		auto cb = qobject_cast<QComboBox*>(editor);
		int teller = 0;
		for (auto &[name, filter] : m_bowtieFilters)
		{
			if (filter == currentFilter)
			{
				cb->setCurrentIndex(teller);
				return;
			}
			teller++;
		}
	}
	else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>())
	{
		auto currentFilter = qvariant_cast<std::shared_ptr<AECFilter>>(index.data(Qt::EditRole));
		auto cb = qobject_cast<QComboBox*>(editor);
		int teller = 0;
		for (const auto &[name, filter] : m_aecFilters)
		{
			if (filter == currentFilter)
			{
				cb->setCurrentIndex(teller);
				return;
			}
			teller++;
		}
	}
	QStyledItemDelegate::setEditorData(editor, index);
}


void SourceDelegate::setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const
{
	auto userType = index.data(Qt::DisplayRole).userType();
	if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>())
	{
		auto cb = qobject_cast<QComboBox *>(editor);
		auto idx = cb->currentIndex();
		if (idx < 0)
			idx = 0;
		auto data = QVariant::fromValue(m_bowtieFilters[idx].second);
		model->setData(index, data, Qt::EditRole);
	}
	else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>())
	{
		auto cb = qobject_cast<QComboBox *>(editor);
		auto idx = cb->currentText();
		if (idx.size() == 0)
			idx = "None";
		auto data = QVariant::fromValue(m_aecFilters.at(idx));
		model->setData(index, data, Qt::EditRole);
	}
	else
	{
		QStyledItemDelegate::setModelData(editor, model, index);
	}	
}

QString SourceDelegate::displayText(const QVariant & value, const QLocale & locale) const
{
	auto userType = value.userType();
	if (userType == qMetaTypeId<std::shared_ptr<BowTieFilter>>())
	{
		auto currentFilter = qvariant_cast<std::shared_ptr<BowTieFilter>>(value);
		int teller = 0;
		for (auto &[name, filter] : m_bowtieFilters)
		{
			if (filter == currentFilter)
			{
				return m_bowtieFilters[teller].first;
			}
			teller++;
		}
	}
	else if (userType == qMetaTypeId<std::shared_ptr<AECFilter>>())
	{
		auto currentFilter = qvariant_cast<std::shared_ptr<AECFilter>>(value);
		int teller = 0;
		for (auto &[name, filter] : m_aecFilters)
		{
			if (filter == currentFilter)
			{
				return name;
			}
			teller++;
		}
	}
	return QStyledItemDelegate::displayText(value, locale);
}

void SourceDelegate::addBowtieFilter(const QString& name, std::shared_ptr<BowTieFilter> filter) {
	m_bowtieFilters.push_back(std::make_pair(name, filter));
	std::sort(m_bowtieFilters.begin(), m_bowtieFilters.end());
}
void SourceDelegate::addAecFilter(const QString& name, std::shared_ptr<AECFilter> filter) { 
	m_aecFilters[name] = filter; 
}

SourceModelView::SourceModelView(QWidget* parent)
	:QTreeView(parent)
{
}
void SourceModelView::keyPressEvent(QKeyEvent* event)
{	
	if ((event->key() == Qt::Key_Backspace) || (event->key() == Qt::Key_Delete))
	{
		auto index = selectionModel()->currentIndex();
		if (index.isValid())
		{
			auto parent = index.parent(); // if top level item parent is not valid
			if (!parent.isValid())
			{

				model()->removeRow(index.row());
				return;
			}
		}
	}
	QTreeView::keyPressEvent(event);
}

SourceEditWidget::SourceEditWidget(QWidget *parent)
	:QWidget(parent)
{
	qRegisterMetaType<std::vector<std::shared_ptr<Source>>>();

	auto mainLayout = new QVBoxLayout(this);
	//defining available sources
	m_sourceTypes[0] = "DX tube";
	m_sourceTypes[1] = "CT Spiral tube";
	m_sourceTypes[2] = "CT Axial tube";
	m_sourceTypes[3] = "CT Dual tube";


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
	auto test = new BowtieFilterReader(this);
	test->loadFilters();
	
	for (auto[name, filter] : test->filters())
	{
		m_delegate->addBowtieFilter(name, filter);
	}

	
	//Perhaps make an interface to start and stop simulations//
	auto runbutton = new QPushButton(tr("Run simulation"), this);
	mainLayout->addWidget(runbutton);
	connect(runbutton, &QPushButton::clicked, this, &SourceEditWidget::requestRunSimulation);

	modelView->showColumn(2);
	mainLayout->addWidget(addSourceBox);
	mainLayout->addWidget(modelView);
	this->setLayout(mainLayout);
}

void SourceEditWidget::addCurrentSourceType(void)
{
	if (m_currentSourceTypeSelected == 0)
	{
		// add dx beam
		m_model->addSource(Source::Type::DX);
	}
	else if (m_currentSourceTypeSelected == 1)
	{
		//add spiral beam
		m_model->addSource(Source::Type::CTSpiral);
	}
	else if (m_currentSourceTypeSelected == 2)
	{
		//add axial beam
		m_model->addSource(Source::Type::CTAxial);
	}
	else if (m_currentSourceTypeSelected == 3)
	{
		//add DE beam
		m_model->addSource(Source::Type::CTDual);
	}
}

void SourceEditWidget::requestRunSimulation(void)
{
	if (m_model)
	{
		auto sources = m_model->sources();
		emit runSimulation(sources);
	}
}

