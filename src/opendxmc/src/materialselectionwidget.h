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

#include "material.h"

#include <QWidget>
#include <QAbstractTableModel>
#include <QModelIndex>
#include <QVector>
#include <QVariant>
#include <QString>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QTableView>

#include <vector>

#ifndef Q_DECLARE_METATYPE_MATERIALVECTOR
#define Q_DECLARE_METATYPE_MATERIALVECTOR
Q_DECLARE_METATYPE(std::vector<Material>)
#endif 

class MaterialTableModel :public QAbstractTableModel
{
	Q_OBJECT
public:
	MaterialTableModel(QObject *parent=nullptr);
	bool addMaterial(const QString& materialName);
	bool addMaterial(const Material& material);
	bool addMaterial(int atomicNumber);
	const std::vector<Material>& getMaterials() const { return m_materials; }
	bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
	bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
	int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	int columnCount(const QModelIndex &parent = QModelIndex()) const override;
	Qt::ItemFlags flags(const QModelIndex &index) const override;
	QVariant data(const QModelIndex &index, int role) const override;
	bool setData(const QModelIndex &index, const QVariant &value, int role) override;

signals:
	void materialsChanged(bool);
private:
	std::vector<Material> m_materials;
	QStringList m_header;
};


class MaterialSelectionWidget :public QWidget
{
	Q_OBJECT
public:
	MaterialSelectionWidget(QWidget *parent=nullptr);
	void applyMaterials(void);
signals:
	void statusMessage(const QString& message, int timeout);
	void materialsChanged(const std::vector<Material>& materials);
protected:
	void getDensityFromMaterialName(void);
	void tryAddMaterial(void);

private:
	MaterialTableModel *m_tableModel;
	QTableView *m_tableView;
	QLineEdit *m_materialNameEdit;
	QDoubleSpinBox *m_materialDensityEdit;
	
};
