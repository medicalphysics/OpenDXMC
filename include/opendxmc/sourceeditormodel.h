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
#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/imagecontainer.h"
#include "opendxmc/imageimportpipeline.h" // for qt meta aecfilter
#include "opendxmc/volumeactorcontainer.h"

#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QVariant>

#include <array>
#include <functional>
#include <memory>
#include <vector>

#ifndef Q_DECLARE_METATYPE_SOURCEVECTOR
#define Q_DECLARE_METATYPE_SOURCEVECTOR
Q_DECLARE_METATYPE(std::vector<std::shared_ptr<Source>>)
#endif
#ifndef Q_DECLARE_METATYPE_SOURCE
#define Q_DECLARE_METATYPE_SOURCE
Q_DECLARE_METATYPE(std::shared_ptr<Source>)
#endif
#ifndef Q_DECLARE_METATYPE_VOLUMEACTORCONTAINERPTR
#define Q_DECLARE_METATYPE_VOLUMEACTORCONTAINERPTR
Q_DECLARE_METATYPE(VolumeActorContainer*)
#endif
#ifndef Q_DECLARE_METATYPE_AECFILTER
#define Q_DECLARE_METATYPE_AECFILTER
Q_DECLARE_METATYPE(std::shared_ptr<AECFilter>)
#endif
#ifndef Q_DECLARE_METATYPE_BOWTIEFILTER
#define Q_DECLARE_METATYPE_BOWTIEFILTER
Q_DECLARE_METATYPE(std::shared_ptr<BowTieFilter>)
#endif

template <class S, typename T>
class SourceItem : public QStandardItem {
public:
    SourceItem(std::shared_ptr<S> source, std::function<void(T)> setter, std::function<T(void)> getter);
    QVariant data(int role) const override
    {
        if constexpr (std::is_same<T, bool>::value) {
            // For boelan data we want to use checkboxes
            if (role == Qt::CheckStateRole) {
                return f_data() ? Qt::Checked : Qt::Unchecked;
            }
        } else {
            if (role == Qt::DisplayRole) {
                return f_data();
            }
        }

        return QStandardItem::data(role);
    }
    void setData(const QVariant& data, int role) override
    {
        if constexpr (std::is_same<T, bool>::value) {
            // For bolean data we want to use checkboxes
            if (role == Qt::CheckStateRole) {
                const T val = data.value<int>() == Qt::Checked;
                f_setData(val);
                emitDataChanged();
            } else {
                QStandardItem::setData(data, role);
            }
        } else {
            if (role == Qt::EditRole) {
                const T val = data.value<T>();
                f_setData(val);
                emitDataChanged();
            } else {
                QStandardItem::setData(data, role);
            }
        }
    }

protected:
    std::function<void(T)> f_setData;
    std::function<T()> f_data;

private:
    std::shared_ptr<S> m_source;
};

class SourceModel : public QStandardItemModel {
    Q_OBJECT
public:
    SourceModel(QObject* parent = nullptr);
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void addSource(Source::Type type = Source::Type::CTSpiral);
    void addSource(std::shared_ptr<Source> src);
    void setSources(const std::vector<std::shared_ptr<Source>>& sources);
    std::vector<std::shared_ptr<Source>>& sources() { return m_sources; }
    bool removeRow(int row, const QModelIndex& parent = QModelIndex());
    bool removeRows(int rows, int count, const QModelIndex& parent = QModelIndex()) override;
    void setImageData(std::shared_ptr<ImageContainer> image);
signals:
    void sourceActorAdded(SourceActorContainer* actorContainer);
    void sourceActorRemoved(SourceActorContainer* actorContainer);
    void sourceAdded(std::shared_ptr<Source> source);
    void sourceRemoved(std::shared_ptr<Source> source);
    void actorsChanged();
    void sourcesForSimulation(const std::vector<std::shared_ptr<Source>>& sources);

protected:
    void setupSource(std::shared_ptr<Source> src, QStandardItem* parent);
    void setupCTSource(std::shared_ptr<CTSource> src, QStandardItem* parent);
    void setupCTBaseSource(std::shared_ptr<CTBaseSource> src, QStandardItem* parent);
    void setupCTAxialSource(std::shared_ptr<CTAxialSource> src, QStandardItem* parent);
    void setupCTSpiralSource(std::shared_ptr<CTSpiralSource> src, QStandardItem* parent);
    void setupCTDualSource(std::shared_ptr<CTSpiralDualSource> src, QStandardItem* parent);
    void setupCTTopogramSource(std::shared_ptr<CTTopogramSource> src, QStandardItem* parent);
    void setupDXSource(std::shared_ptr<DXSource> src, QStandardItem* parent);
    void sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles);
    bool removeSource(std::shared_ptr<Source> src);

private:
    std::vector<std::shared_ptr<SourceActorContainer>> m_actors;
    std::vector<std::shared_ptr<Source>> m_sources;
    std::uint64_t m_currentImageID = 0;
    std::array<double, 6> m_currentImageExtent = { 0, 0, 0, 0, 0, 0 };
};
