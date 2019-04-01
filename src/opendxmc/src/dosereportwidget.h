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

#pragma once

#include "dosereportcontainer.h"

#include <QWidget>
#include <QString>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVariant>
#include <QTableView>



class DoseReportModel : public QAbstractTableModel
{
	Q_OBJECT
public:
	DoseReportModel(const QString& name, QObject *parent = nullptr);
	void setDoseData(std::shared_ptr<std::vector<DoseReportElement>> data);
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	//Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant data(const QModelIndex &index, int role) const override;
private:
	std::shared_ptr<std::vector<DoseReportElement>> m_data;
	QString m_name ="";
};

class DoseReportView :public QTableView
{
	Q_OBJECT
public:
	DoseReportView(QWidget* parent = nullptr);
protected:
	void DoseReportView::keyPressEvent(QKeyEvent *event) override;
};

class DoseReportWidget :public QWidget
{
	Q_OBJECT
public:
	DoseReportWidget(QWidget* parent = nullptr);
	void setDoseData(const DoseReportContainer& doseData);

private:
	std::uint64_t m_ID = 0;
	DoseReportModel* m_organModel;
	DoseReportModel* m_materialModel;
	
};
