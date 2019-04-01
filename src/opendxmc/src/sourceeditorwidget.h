#pragma once



#include "sourceeditormodel.h"

#include <QWidget>
#include <QTreeView>
#include <QColumnView>
#include <QStyledItemDelegate>
#include <QJsonObject>


class BowtieFilterReader :public QWidget
{
	Q_OBJECT
public:
	BowtieFilterReader(QWidget *parent = nullptr);
	void addFilter(const QString& name, std::shared_ptr<BowTieFilter>& filter);
	bool loadFilters();
	bool saveFilters();
	std::vector<std::pair<QString, std::shared_ptr<BowTieFilter>>>& filters() { return m_bowtieFilters; }

protected:
	void readJson(const QJsonObject &json);
	void writeJson(QJsonObject &json) const;
	std::pair<QString, std::shared_ptr<BowTieFilter>> readFilter(QJsonObject &json) const;
	void writeFilter(QJsonObject& json, const std::pair<QString, std::shared_ptr<BowTieFilter>>& filter) const;
private:
	std::vector<std::pair<QString, std::shared_ptr<BowTieFilter>>> m_bowtieFilters;
};

class SourceDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	SourceDelegate(QObject *parent = nullptr);
	QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
	void setEditorData(QWidget *editor, const QModelIndex &index) const override;
	void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override;
	QString displayText(const QVariant& value, const QLocale& locale) const override;

	void addBowtieFilter(const QString& name, std::shared_ptr<BeamFilter> filter) { m_bowtieFilters.push_back(std::make_pair(name, filter)); }
	void addAecFilter(const QString& name, std::shared_ptr<PositionalFilter> filter) { m_aecFilters.push_back(std::make_pair(name, filter)); }

private:
	std::vector<std::pair<QString, std::shared_ptr<BeamFilter>>> m_bowtieFilters;
	std::vector<std::pair<QString, std::shared_ptr<PositionalFilter>>> m_aecFilters;
};

class SourceModelView : public QTreeView
{
	Q_OBJECT
public:
	SourceModelView(QWidget* parent = nullptr);
protected:
	void keyPressEvent(QKeyEvent *event) override;

};


class SourceEditWidget : public QWidget
{
	Q_OBJECT
public:
	SourceEditWidget(QWidget *parent = nullptr);
	SourceModel* model() { return m_model; }
	SourceDelegate* delegate() { return m_delegate; }
signals:
	void runSimulation(std::vector<std::shared_ptr<Source>> sources);
protected:
	void setCurrentSourceTypeSelected(int index) { m_currentSourceTypeSelected = index; }
	void addCurrentSourceType(void);
	void requestRunSimulation(void);


private:
	QMap<int, QString> m_sourceTypes;
	SourceModel *m_model = nullptr;
	SourceDelegate *m_delegate = nullptr;
	int m_currentSourceTypeSelected;
	
};

