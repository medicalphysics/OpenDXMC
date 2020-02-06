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



#include "opendxmc/dosereportwidget.h"
#include "opendxmc/colormap.h"

#include <QBrush>
#include <QHeaderView>
#include <QKeyEvent>
#include <QKeySequence>
#include <QString>
#include <QApplication>
#include <QClipboard>

#include <string>
#include <algorithm>

DoseReportModel::DoseReportModel(const QString& name, QObject * parent)
	:QAbstractTableModel(parent), m_name(name)
{
	m_data = std::make_shared<std::vector<DoseReportElement>>();
}

void DoseReportModel::setDoseData(std::shared_ptr<std::vector<DoseReportElement>> data)
{
	QList<QPersistentModelIndex> changedList; // this is empty since the whole model is changed
	emit layoutAboutToBeChanged(changedList);
	m_data = data;
	emit layoutChanged(changedList);
}

QVariant DoseReportModel::headerData(int section, Qt::Orientation orientation, int role) const
{

	if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole))
	{
		if (section == 0)
			return QString(tr("Color"));
		else if (section == 1)
			return m_name;
		else if (section == 2)
			return QString(tr("Mass [kg]"));
		else if (section == 3)
			return QString(tr("Volume [cm3]"));
		else if (section == 4)
			return QString(tr("Dose [") + QString(m_dataUnits) + QString("]"));
		else if (section == 5)
			return QString(tr("Dose stddev [") + QString(m_dataUnits) + QString("]"));
		else if (section == 6)
			return QString(tr("Dose max value [") + QString(m_dataUnits) + QString("]"));
		else if (section == 7)
			return QString(tr("Number of voxels [N]"));
		else if (section == 8)
			return QString(tr("ID"));
	}
	return QVariant();
}

void DoseReportModel::sort(int column, Qt::SortOrder order)
{
	QList<QPersistentModelIndex> changedList; // this is empty since the whole model is changed
	emit layoutAboutToBeChanged(changedList);
	auto beg = m_data->begin();
	auto end = m_data->end();
	if (column == 1)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto &left, auto &right) {return left.name < right.name; });
		else
			std::sort(beg, end, [=](auto &left, auto &right) {return left.name > right.name; });
	else if (column == 2)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto &left, auto &right) {return left.mass < right.mass; });
		else
			std::sort(beg, end, [=](auto &left, auto &right) {return left.mass > right.mass; });
	else if (column == 3)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto &left, auto &right) {return left.volume < right.volume; });
		else
			std::sort(beg, end, [=](auto &left, auto &right) {return left.volume > right.volume; });
	else if (column == 4)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto &left, auto &right) {return left.dose < right.dose; });
		else
			std::sort(beg, end, [=](auto &left, auto &right) {return left.dose > right.dose; });
	else if (column == 5)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto &left, auto &right) {return left.doseStd < right.doseStd; });
		else
			std::sort(beg, end, [=](auto &left, auto &right) {return left.doseStd > right.doseStd; });
	else if (column == 6)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto &left, auto &right) {return left.doseMax < right.doseMax; });
		else
			std::sort(beg, end, [=](auto &left, auto &right) {return left.doseMax > right.doseMax; });
	else if (column == 7)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto& left, auto& right) {return left.voxels < right.voxels; });
		else
			std::sort(beg, end, [=](auto& left, auto& right) {return left.voxels > right.voxels; });
	else if (column == 8)
		if (order == Qt::AscendingOrder)
			std::sort(beg, end, [=](auto &left, auto &right) {return left.ID < right.ID; });
		else 
			std::sort(beg, end, [=](auto& left, auto& right) {return left.ID > right.ID; });
		
	emit layoutChanged(changedList);
}

int DoseReportModel::rowCount(const QModelIndex & parent) const
{
	return static_cast<int>(m_data->size());
}

int DoseReportModel::columnCount(const QModelIndex & parent) const
{
	return 9;
}

QVariant DoseReportModel::data(const QModelIndex& index, int role) const
{
	if (role == Qt::DisplayRole)
	{
		if (index.column() == 1)
			return QString::fromStdString((*m_data)[index.row()].name);
		else if (index.column() == 2)
			return (*m_data)[index.row()].mass;
		else if (index.column() == 3)
			return (*m_data)[index.row()].volume;
		else if (index.column() == 4)
			return (*m_data)[index.row()].dose;
		else if (index.column() == 5)
			return (*m_data)[index.row()].doseStd;
		else if (index.column() == 6)
			return (*m_data)[index.row()].doseMax;
		else if (index.column() == 7)
			return (*m_data)[index.row()].voxels;
		else if (index.column() == 8)
			return (*m_data)[index.row()].ID;
	}
	else if (role == Qt::BackgroundRole) {
		if (index.column() == 0)
			return QBrush(getQColor(static_cast<int>((*m_data)[index.row()].ID)));
		else
			return QVariant();
	}
	return QVariant();
}


DoseReportView::DoseReportView(QWidget* parent)
	:QTableView(parent)
{
}
void DoseReportView::keyPressEvent(QKeyEvent *event)
{
	QModelIndexList selectedRows = selectionModel()->selectedRows();
	if (!selectedIndexes().isEmpty())
	{
		if (event->matches(QKeySequence::Copy))
		{
			QString text;
			QItemSelectionRange range = selectionModel()->selection().first();
			QStringList headerContents;
			for (auto j = range.left(); j <= range.right(); ++j)
				headerContents << model()->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString();
			text += headerContents.join("\t");
			text += "\n";
			for (auto i = range.top(); i <= range.bottom(); ++i)
			{
				QStringList rowContents;
				for (auto j = range.left(); j <= range.right(); ++j)
					rowContents << model()->index(i, j).data().toString();
				text += rowContents.join("\t");
				text += "\n";
			}
			QApplication::clipboard()->setText(text);
			return;
		}
	}
	QTableView::keyPressEvent(event);
}
DoseReportWidget::DoseReportWidget(QWidget *parent)
	:QWidget(parent)
{
	m_organModel = new DoseReportModel(tr("Organ name"), this);
	m_materialModel = new DoseReportModel(tr("Material name"), this);
	auto layout = new QVBoxLayout(this);

	auto view1 = new DoseReportView(this);
	auto view2 = new DoseReportView(this);
	layout->addWidget(view1);
	layout->addWidget(view2);
	view1->setModel(m_materialModel);
	view2->setModel(m_organModel);
	view1->setSortingEnabled(true);
	view2->setSortingEnabled(true);
	connect(m_materialModel, &DoseReportModel::layoutChanged, [=](const auto & parents, auto hint) {view1->resizeColumnsToContents(); });
	connect(m_organModel, &DoseReportModel::layoutChanged, [=](const auto & parents, auto hint) {view2->resizeColumnsToContents(); });

	this->setLayout(layout);
}

void DoseReportWidget::setDoseData(const DoseReportContainer & doseData)
{
	m_organModel->setDoseData(doseData.organData());
	m_materialModel->setDoseData(doseData.materialData());
	
	m_organModel->setDataUnits(doseData.doseUnits());
	m_materialModel->setDataUnits(doseData.doseUnits());
}
