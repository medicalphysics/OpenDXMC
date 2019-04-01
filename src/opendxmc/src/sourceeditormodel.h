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

#include "source.h"
#include "volumeactorcontainer.h"
#include "beamfilters.h"
#include "imageimportpipeline.h" // for qt meta aecfilter

#include <QStandardItemModel>
#include <QStandardItem>
#include <QVariant>
#include <QString>

#include <vector>
#include <memory>
#include <functional>
#include <array>

template<class S, typename T>
class SourceItem :public QStandardItem
{
public:
	SourceItem(std::shared_ptr<S> source, std::function<void(T)> setter, std::function<T(void)> getter);
	QVariant data(int role) const override { if (role == Qt::DisplayRole || role == Qt::EditRole) return f_data(); else return QVariant(); }
	void setData(const QVariant& data, int role) override { if (role == Qt::DisplayRole || role ==Qt::EditRole) { T val = data.value<T>(); f_setData(val); emitDataChanged(); } }
protected:
	std::function<void(T)> f_setData;
	std::function<T()> f_data;
private:

	std::shared_ptr<S> m_source;
};
Q_DECLARE_METATYPE(std::shared_ptr<BeamFilter>);
Q_DECLARE_METATYPE(std::shared_ptr<PositionalFilter>);

class SourceModel :public QStandardItemModel
{
	Q_OBJECT
public:
	SourceModel(QObject *parent = nullptr);
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	void addSource(Source::Type type = Source::CTSpiral);
	std::vector<std::shared_ptr<Source>>& sources() { return m_sources; }
	bool removeRow(int row, const QModelIndex &parent = QModelIndex());
	bool removeRows(int rows, int count, const QModelIndex &parent = QModelIndex()) override;

signals:
	void sourceAdded(VolumeActorContainer* actorContainer);
	void sourceRemoved(VolumeActorContainer* actorContainer);
	void actorsChanged();
	void sourcesForSimulation(std::vector<std::shared_ptr<Source>>& sources);
protected:
	void setupSource(std::shared_ptr<Source> src, QStandardItem *parent);
	void setupCTSource(std::shared_ptr<CTSource> src, QStandardItem *parent);
	void setupCTAxialSource(std::shared_ptr<CTAxialSource> src);
	void setupCTSpiralSource(std::shared_ptr<CTSpiralSource> src);
	void setupCTDualSource(std::shared_ptr<CTDualSource> src);
	void setupDXSource(std::shared_ptr<DXSource> src);
	void sourceDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight, const QVector<int> &roles);
	bool removeSource(std::shared_ptr<Source> src);

private:
	std::vector<std::shared_ptr<VolumeActorContainer>> m_actors;
	std::vector<std::shared_ptr<Source>> m_sources;
	//std::vector<std::shared_ptr<BowTieFilter>> m_bowTieFilters;

};

