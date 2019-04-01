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
