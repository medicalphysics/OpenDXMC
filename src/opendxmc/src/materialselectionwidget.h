

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
