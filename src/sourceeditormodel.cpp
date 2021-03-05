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

#include "opendxmc/sourceeditormodel.h"

template <class S, typename T>
SourceItem<S, T>::SourceItem(std::shared_ptr<S> source, std::function<void(T)> setter, std::function<T(void)> getter)
    : QStandardItem()
{
    f_data = getter;
    f_setData = setter;
    m_source = source;

    //For boolean data we want to use checkboxes
    if constexpr (std::is_same<bool, T>::value) {
        setCheckable(true);
    }
}

template <>
QVariant SourceItem<CTBaseSource, std::shared_ptr<BowTieFilter>>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return QVariant::fromValue(f_data());
    }
    return QVariant();
}

template <>
void SourceItem<CTBaseSource, std::shared_ptr<BowTieFilter>>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto val = qvariant_cast<std::shared_ptr<BowTieFilter>>(data);
        f_setData(val);
        emitDataChanged();
    }
}

template <>
QVariant SourceItem<CTSpiralDualSource, std::shared_ptr<BowTieFilter>>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return QVariant::fromValue(f_data());
    }
    return QVariant();
}

template <>
void SourceItem<CTSpiralDualSource, std::shared_ptr<BowTieFilter>>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto val = qvariant_cast<std::shared_ptr<BowTieFilter>>(data);
        f_setData(val);
        emitDataChanged();
    }
}

template <>
QVariant SourceItem<CTSource, std::shared_ptr<AECFilter>>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        return QVariant::fromValue(f_data());
    }
    return QVariant();
}

template <>
void SourceItem<CTSource, std::shared_ptr<AECFilter>>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto val = qvariant_cast<std::shared_ptr<AECFilter>>(data);
        f_setData(val);
        emitDataChanged();
    }
}

template <>
QVariant SourceItem<CTSource, std::uint64_t>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = f_data();
        const auto data = static_cast<qlonglong>(n);
        return QVariant(data);
    }
    return QVariant();
}

template <>
QVariant SourceItem<CTBaseSource, std::uint64_t>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = f_data();
        const auto data = static_cast<qlonglong>(n);
        return QVariant(data);
    }
    return QVariant();
}

template <>
void SourceItem<CTSource, std::uint64_t>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = static_cast<std::uint64_t>(data.toULongLong());
        f_setData(n);
        emitDataChanged();
    }
}
template <>
void SourceItem<CTBaseSource, std::uint64_t>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = static_cast<std::uint64_t>(data.toULongLong());
        f_setData(n);
        emitDataChanged();
    }
}

template <>
QVariant SourceItem<CTSpiralDualSource, std::uint64_t>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = f_data();
        const auto data = static_cast<qlonglong>(n);
        return QVariant(data);
    }
    return QVariant();
}

template <>
void SourceItem<CTSpiralDualSource, std::uint64_t>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = static_cast<std::uint64_t>(data.toULongLong());
        f_setData(n);
        emitDataChanged();
    }
}

template <>
QVariant SourceItem<DXSource, std::uint64_t>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = f_data();
        const auto data = static_cast<qlonglong>(n);
        return QVariant(data);
    }
    return QVariant();
}

template <>
void SourceItem<DXSource, std::uint64_t>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = static_cast<std::uint64_t>(data.toULongLong());
        f_setData(n);
        emitDataChanged();
    }
}

template <std::size_t N>
QString arrayToQString(const std::array<floating, N>& arr)
{
    QString str;
    for (std::size_t i = 0; i < N - 1; ++i) {
        str += QString::number(arr[i]) + QString(", ");
    }
    str += QString::number(arr.back());
    return str;
}

template <std::size_t N>
std::pair<bool, std::array<floating, N>> qstringToArray(const QString& str)
{
    auto stringList = str.split(",", QString::SkipEmptyParts);
    std::array<floating, N> data;
    if (stringList.size() < data.size())
        return std::make_pair(false, data);
    for (int i = 0; i < N; ++i) {
        bool ok;
        floating value = stringList[i].toDouble(&ok);
        if (ok)
            data[i] = value;
        else
            return std::make_pair(false, data);
    }
    return std::make_pair(true, data);
}

template <>
QVariant SourceItem<DXSource, std::array<floating, 2>>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = f_data();
        auto data = arrayToQString(n);
        return QVariant(data);
    }
    return QVariant();
}

template <>
void SourceItem<DXSource, std::array<floating, 2>>::setData(const QVariant& data, int role)
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto string = data.toString();
        const auto& [valid, data] = qstringToArray<2>(string);
        if (valid) {
            f_setData(data);
            emitDataChanged();
        }
    }
}

template <>
QVariant SourceItem<Source, std::array<floating, 3>>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = f_data();
        const auto data = arrayToQString(n);
        return QVariant(data);
    }
    return QVariant();
}

template <>
void SourceItem<Source, std::array<floating, 3>>::setData(const QVariant& data, int role)
{

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto string = data.toString();
        const auto [valid, data] = qstringToArray<3>(string);
        if (valid) {
            f_setData(data);
            emitDataChanged();
        }
    }
}

template <>
QVariant SourceItem<Source, std::array<floating, 6>>::data(int role) const
{
    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto n = f_data();
        const auto data = arrayToQString(n);
        return QVariant(data);
    }
    return QVariant();
}

template <>
void SourceItem<Source, std::array<floating, 6>>::setData(const QVariant& data, int role)
{

    if (role == Qt::DisplayRole || role == Qt::EditRole) {
        const auto string = data.toString();
        const auto& [valid, data] = qstringToArray<6>(string);
        if (valid) {
            f_setData(data);
            emitDataChanged();
        }
    }
}

SourceModel::SourceModel(QObject* parent)
    : QStandardItemModel(parent)
{
    qRegisterMetaType<std::vector<std::shared_ptr<Source>>>();
    qRegisterMetaType<VolumeActorContainer*>();
    qRegisterMetaType<std::shared_ptr<Source>>();
    setColumnCount(2);
    connect(this, &SourceModel::dataChanged, this, &SourceModel::sourceDataChanged);
}

QVariant SourceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if (section == 0)
            return QString("Name");
        if (section == 1)
            return QString("Value");
    }
    return QVariant();
}

void SourceModel::addSource(Source::Type type)
{
    using std::placeholders::_1;
    auto parent = this->invisibleRootItem();
    if (type == Source::Type::CTSpiral) {
        auto src = std::make_shared<CTSpiralSource>();
        std::array<floating, 6> cosines = { -1, 0, 0, 0, 0, 1 };
        src->setDirectionCosines(cosines);

        //fitting src position to cover image data
        if (m_currentImageID != 0) {
            std::array<floating, 2> srcCoverage;
            if (m_currentImageExtent[5] - m_currentImageExtent[4] < 500.0) {
                srcCoverage[0] = m_currentImageExtent[4];
                srcCoverage[1] = m_currentImageExtent[5];
            } else {
                floating center = (m_currentImageExtent[5] + m_currentImageExtent[4]) * 0.5;
                srcCoverage[0] = center - 250.0;
                srcCoverage[1] = center + 250.0;
            }
            std::array<floating, 3> position = { 0, 0, srcCoverage[0] };
            src->setPosition(position);
            src->setScanLenght(srcCoverage[1] - srcCoverage[0]);
        }
        m_sources.emplace_back(std::static_pointer_cast<Source>(src));
        setupCTSpiralSource(src, parent);
        auto actor = std::make_shared<CTSpiralSourceContainer>(src);
        m_actors.push_back(std::static_pointer_cast<SourceActorContainer>(actor));
        emit sourceActorAdded(actor.get());
        emit sourceAdded(std::static_pointer_cast<Source>(src));
        emit layoutChanged();
    } else if (type == Source::Type::CTAxial) {
        auto src = std::make_shared<CTAxialSource>();
        std::array<floating, 6> cosines = { -1, 0, 0, 0, 0, 1 };
        src->setDirectionCosines(cosines);
        //fitting src position to cover image data
        if (m_currentImageID != 0) {
            std::array<floating, 2> srcCoverage;
            if (m_currentImageExtent[5] - m_currentImageExtent[4] < 500.0) {
                srcCoverage[0] = m_currentImageExtent[4];
                srcCoverage[1] = m_currentImageExtent[5];
            } else {
                floating center = (m_currentImageExtent[5] + m_currentImageExtent[4]) * 0.5;
                srcCoverage[0] = center - 250.0;
                srcCoverage[1] = center + 250.0;
            }
            std::array<floating, 3> position = { 0, 0, srcCoverage[0] };
            src->setPosition(position);
            src->setScanLenght(srcCoverage[1] - srcCoverage[0]);
        }
        m_sources.emplace_back(std::static_pointer_cast<Source>(src));
        setupCTAxialSource(src, parent);
        auto actor = std::make_shared<CTAxialSourceContainer>(src);
        m_actors.push_back(std::static_pointer_cast<SourceActorContainer>(actor));
        emit sourceActorAdded(actor.get());
        emit sourceAdded(std::static_pointer_cast<Source>(src));
        emit layoutChanged();
    } else if (type == Source::Type::CTDual) {
        auto src = std::make_shared<CTSpiralDualSource>();
        std::array<floating, 6> cosines = { -1, 0, 0, 0, 0, 1 };
        src->setDirectionCosines(cosines);
        //fitting src position to cover image data
        if (m_currentImageID != 0) {
            std::array<floating, 2> srcCoverage;
            if (m_currentImageExtent[5] - m_currentImageExtent[4] < 500.0) {
                srcCoverage[0] = m_currentImageExtent[4];
                srcCoverage[1] = m_currentImageExtent[5];
            } else {
                floating center = (m_currentImageExtent[5] + m_currentImageExtent[4]) * 0.5;
                srcCoverage[0] = center - 250.0;
                srcCoverage[1] = center + 250.0;
            }
            std::array<floating, 3> position = { 0, 0, srcCoverage[0] };
            src->setPosition(position);
            src->setScanLenght(srcCoverage[1] - srcCoverage[0]);
        }
        m_sources.emplace_back(std::static_pointer_cast<Source>(src));
        setupCTDualSource(src, parent);
        auto actor = std::make_shared<CTDualSourceContainer>(src);
        m_actors.push_back(std::static_pointer_cast<SourceActorContainer>(actor));
        emit sourceActorAdded(actor.get());
        emit sourceAdded(std::static_pointer_cast<Source>(src));
        emit layoutChanged();
    } else if (type == Source::Type::DX) {
        auto src = std::make_shared<DXSource>();
        std::array<floating, 3> position = { 0, 0, 0 };
        std::array<floating, 6> cosines = { -1, 0, 0, 0, 0, 1 };
        src->setDirectionCosines(cosines);
        src->setPosition(position);
        m_sources.emplace_back(std::static_pointer_cast<Source>(src));
        setupDXSource(src, parent);
        auto actor = std::make_shared<DXSourceContainer>(src);
        m_actors.push_back(std::static_pointer_cast<SourceActorContainer>(actor));
        emit sourceActorAdded(actor.get());
        emit sourceAdded(std::static_pointer_cast<Source>(src));
        emit layoutChanged();
    } else if (type == Source::Type::CTTopogram) {
        auto src = std::make_shared<CTTopogramSource>();
        std::array<floating, 6> cosines = { -1, 0, 0, 0, 0, 1 };
        src->setDirectionCosines(cosines);
        //fitting src position to cover image data
        if (m_currentImageID != 0) {
            std::array<floating, 2> srcCoverage;
            if (m_currentImageExtent[5] - m_currentImageExtent[4] < 500.0) {
                srcCoverage[0] = m_currentImageExtent[4];
                srcCoverage[1] = m_currentImageExtent[5];
            } else {
                floating center = (m_currentImageExtent[5] + m_currentImageExtent[4]) * 0.5;
                srcCoverage[0] = center - 250.0;
                srcCoverage[1] = center + 250.0;
            }
            std::array<floating, 3> position = { 0, 0, srcCoverage[0] };
            src->setPosition(position);
            src->setScanLenght(srcCoverage[1] - srcCoverage[0]);
        }
        m_sources.emplace_back(std::static_pointer_cast<Source>(src));
        setupCTTopogramSource(src, parent);
        auto actor = std::make_shared<CTTopogramSourceContainer>(src);
        m_actors.push_back(std::static_pointer_cast<SourceActorContainer>(actor));
        emit sourceActorAdded(actor.get());
        emit sourceAdded(std::static_pointer_cast<Source>(src));
        emit layoutChanged();
    }
}

void SourceModel::addSource(std::shared_ptr<Source> src)
{
    std::shared_ptr<SourceActorContainer> actor = nullptr;
    auto parent = this->invisibleRootItem();
    switch (src->type()) {
    case Source::Type::DX:
        actor = std::make_shared<DXSourceContainer>(std::static_pointer_cast<DXSource>(src));
        setupDXSource(std::static_pointer_cast<DXSource>(src), parent);
        break;
    case Source::Type::CTAxial:
        actor = std::make_shared<CTAxialSourceContainer>(std::static_pointer_cast<CTAxialSource>(src));
        setupCTAxialSource(std::static_pointer_cast<CTAxialSource>(src), parent);
        break;
    case Source::Type::CTSpiral:
        actor = std::make_shared<CTSpiralSourceContainer>(std::static_pointer_cast<CTSpiralSource>(src));
        setupCTSpiralSource(std::static_pointer_cast<CTSpiralSource>(src), parent);
        break;
    case Source::Type::CTDual:
        actor = std::make_shared<CTDualSourceContainer>(std::static_pointer_cast<CTSpiralDualSource>(src));
        setupCTDualSource(std::static_pointer_cast<CTSpiralDualSource>(src), parent);
        break;
    case Source::Type::CTTopogram:
        actor = std::make_shared<CTTopogramSourceContainer>(std::static_pointer_cast<CTTopogramSource>(src));
        setupCTTopogramSource(std::static_pointer_cast<CTTopogramSource>(src), parent);
        break;
    default:
        return;
    }

    m_sources.push_back(src);
    m_actors.push_back(actor);
    emit sourceActorAdded(actor.get());
    emit sourceAdded(src);
    emit layoutChanged();
}

void SourceModel::setSources(const std::vector<std::shared_ptr<Source>>& sources)
{
    //removing all sources
    auto parentItem = invisibleRootItem();
    removeRows(0, m_sources.size(), parentItem->index());

    for (auto actor : m_actors) {
        emit sourceActorRemoved(actor.get());
    }
    m_actors.clear();
    for (auto src : m_sources) {
        emit sourceRemoved(src);
    }
    m_sources.clear();
    for (auto s : sources) {
        addSource(s);
    }
}

bool SourceModel::removeSource(std::shared_ptr<Source> src)
{
    auto pos = std::find(m_sources.begin(), m_sources.end(), src);
    if (pos != m_sources.end()) // we found it
    {
        auto dist = std::distance(m_sources.begin(), pos);
        auto apos = m_actors.begin();
        std::advance(apos, dist);

        emit sourceActorRemoved((*apos).get());
        emit sourceRemoved(src);
        m_sources.erase(pos);
        m_actors.erase(apos);
        return true;
    }
    return false;
}

bool SourceModel::removeRow(int row, const QModelIndex& parent)
{
    return removeRows(row, 1, parent);
}

bool SourceModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if ((!parent.isValid()) && (row >= 0)) // top level, we handle this
    {
        if (row < static_cast<int>(m_sources.size())) {
            auto src = m_sources[row];
            if (!removeSource(src)) // we could not remove it for some reason
                return false;
        }
    }
    return QStandardItemModel::removeRows(row, count, parent);
}

void SourceModel::setImageData(std::shared_ptr<ImageContainer> image)
{
    if (m_currentImageID != image->ID) {
        m_currentImageID = image->ID;
        auto origin = image->image->GetOrigin();
        auto extent = image->image->GetExtent();
        auto spacing = image->image->GetSpacing();
        for (std::size_t i = 0; i < 3; ++i) {
            auto idx = 2 * i;
            m_currentImageExtent[idx] = extent[idx] * spacing[i] + origin[i];
            m_currentImageExtent[idx + 1] = extent[idx + 1] * spacing[i] + origin[i];
        }
    }
}

void addModelItems(const QVector<QPair<QString, QStandardItem*>>& nodes, QStandardItem* parent)
{
    if (!parent)
        return;
    for (int i = 0; i < nodes.count(); ++i) {
        if (nodes[i].first.isEmpty()) {
            parent->appendRow(nodes[i].second);
            nodes[i].second->setEditable(false);
        } else {
            auto descItem = new QStandardItem(nodes[i].first);
            descItem->setEditable(false);
            QList<QStandardItem*> row;
            row.append(descItem);
            row.append(nodes[i].second);
            parent->appendRow(row);
        }
    }
    parent->setEditable(false);
}

template <class S>
void setupTube(std::shared_ptr<S> src, QStandardItem* parent)
{
    QVector<QPair<QString, QStandardItem*>> nodes;
    auto l2Item = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->tube().setVoltage(val); },
        [=]() { return src->tube().voltage(); });
    nodes.append(qMakePair(QString("Tube voltage [kV]"), static_cast<QStandardItem*>(l2Item)));

    auto l3Item = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->tube().setAnodeAngleDeg(val); },
        [=]() { return src->tube().anodeAngleDeg(); });
    nodes.append(qMakePair(QString("Tube anode angle [deg]"), static_cast<QStandardItem*>(l3Item)));

    auto l4Item = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->tube().setAlFiltration(val); },
        [=]() { return src->tube().AlFiltration(); });
    nodes.append(qMakePair(QString("Tube Al filtration [mm]"), static_cast<QStandardItem*>(l4Item)));

    auto l5Item = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->tube().setCuFiltration(val); },
        [=]() { return src->tube().CuFiltration(); });
    nodes.append(qMakePair(QString("Tube Cu filtration [mm]"), static_cast<QStandardItem*>(l5Item)));

    auto l6Item = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->tube().setSnFiltration(val); },
        [=]() { return src->tube().SnFiltration(); });
    nodes.append(qMakePair(QString("Tube Sn (Tin) filtration [mm]"), static_cast<QStandardItem*>(l6Item)));

    auto l7Item = new SourceItem<S, floating>(
        src,
        [=](floating val) {},
        [=]() { return src->tube().mmAlHalfValueLayer(); });
    l7Item->setEditable(false);
    nodes.append(qMakePair(QString("Half value layer in Al [mm]"), static_cast<QStandardItem*>(l7Item)));

    addModelItems(nodes, parent);
}

void setupTubeB(std::shared_ptr<CTSpiralDualSource> src, QStandardItem* parent)
{
    QVector<QPair<QString, QStandardItem*>> nodes;
    auto l2Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->tubeB().setVoltage(val); },
        [=]() { return src->tubeB().voltage(); });
    nodes.append(qMakePair(QString("Tube voltage [kV]"), static_cast<QStandardItem*>(l2Item)));

    auto l3Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->tubeB().setAnodeAngleDeg(val); },
        [=]() { return src->tubeB().anodeAngleDeg(); });
    nodes.append(qMakePair(QString("Tube anode angle [deg]"), static_cast<QStandardItem*>(l3Item)));

    auto l4Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->tubeB().setAlFiltration(val); },
        [=]() { return src->tubeB().AlFiltration(); });
    nodes.append(qMakePair(QString("Tube Al filtration [mm]"), static_cast<QStandardItem*>(l4Item)));

    auto l5Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->tubeB().setCuFiltration(val); },
        [=]() { return src->tubeB().CuFiltration(); });
    nodes.append(qMakePair(QString("Tube Cu filtration [mm]"), static_cast<QStandardItem*>(l5Item)));

    auto l6Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->tubeB().setSnFiltration(val); },
        [=]() { return src->tubeB().SnFiltration(); });
    nodes.append(qMakePair(QString("Tube Sn (Tin) filtration [mm]"), static_cast<QStandardItem*>(l6Item)));

    auto l7Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) {},
        [=]() { return src->tubeB().mmAlHalfValueLayer(); });
    l7Item->setEditable(false);
    nodes.append(qMakePair(QString("Half value layer in Al [mm]"), static_cast<QStandardItem*>(l7Item)));

    addModelItems(nodes, parent);
}

template <class S>
void setupXCare(std::shared_ptr<S> src, QStandardItem* parent)
{
    QVector<QPair<QString, QStandardItem*>> xcareNodes;
    auto useXCare = new SourceItem<S, bool>(
        src,
        [=](bool val) { src->setUseXCareFilter(val); },
        [=]() -> bool { return src->useXCareFilter(); });
    xcareNodes.append(qMakePair(QString("Use organ exposure control"), static_cast<QStandardItem*>(useXCare)));
    auto angleXCare = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->xcareFilter().setFilterAngleDeg(val); },
        [=]() { return src->xcareFilter().filterAngleDeg(); });
    xcareNodes.append(qMakePair(QString("Angle of filter [deg]"), static_cast<QStandardItem*>(angleXCare)));
    auto spanXCare = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->xcareFilter().setSpanAngleDeg(val); },
        [=]() { return src->xcareFilter().spanAngleDeg(); });
    xcareNodes.append(qMakePair(QString("Filter span angle [deg]"), static_cast<QStandardItem*>(spanXCare)));
    auto rampXCare = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->xcareFilter().setRampAngleDeg(val); },
        [=]() { return src->xcareFilter().rampAngleDeg(); });
    xcareNodes.append(qMakePair(QString("Filter ramp angle (included in span angle) [deg]"), static_cast<QStandardItem*>(rampXCare)));
    auto lowXCare = new SourceItem<S, floating>(
        src,
        [=](floating val) { src->xcareFilter().setLowWeight(val); },
        [=]() { return src->xcareFilter().lowWeight(); });
    xcareNodes.append(qMakePair(QString("Lowest beam intensity. Value between (0.0, 1.0]"), static_cast<QStandardItem*>(lowXCare)));
    auto highXCare = new SourceItem<S, floating>(
        src,
        [=](floating val) {},
        [=]() { return src->xcareFilter().highWeight(); });
    highXCare->setEditable(false);
    xcareNodes.append(qMakePair(QString("Highest beam intensity (calculated value)"), static_cast<QStandardItem*>(highXCare)));

    addModelItems(xcareNodes, parent);
}

void SourceModel::setupSource(std::shared_ptr<Source> src, QStandardItem* parent)
{
    QVector<QPair<QString, QStandardItem*>> nodes;

    auto posItem = new SourceItem<Source, std::array<floating, 3>>(
        src,
        [=](const std::array<floating, 3>& val) { src->setPosition(val); },
        [=]() { return src->position(); });
    nodes.append(qMakePair(QString("Source isocenter position [mm]"), static_cast<QStandardItem*>(posItem)));

    auto l1Item = new SourceItem<Source, std::array<floating, 6>>(
        src,
        [=](const std::array<floating, 6>& val) { src->setDirectionCosines(val); },
        [=]() { return src->directionCosines(); });
    nodes.append(qMakePair(QString("Source direction cosines"), static_cast<QStandardItem*>(l1Item)));

    addModelItems(nodes, parent);
}
void SourceModel::setupCTBaseSource(std::shared_ptr<CTBaseSource> src, QStandardItem* parent)
{
    setupSource(std::static_pointer_cast<Source>(src), parent);

    //CT parameters
    QVector<QPair<QString, QStandardItem*>> nodes;

    auto sddItem = new SourceItem<CTBaseSource, floating>(
        src,
        [=](floating val) { src->setSourceDetectorDistance(val); },
        [=]() { return src->sourceDetectorDistance(); });
    nodes.append(qMakePair(QString("Source detector distance [mm]"), static_cast<QStandardItem*>(sddItem)));

    auto fovItem = new SourceItem<CTBaseSource, floating>(
        src,
        [=](floating val) { src->setFieldOfView(val); },
        [=]() { return src->fieldOfView(); });
    nodes.append(qMakePair(QString("Field of view [mm]"), static_cast<QStandardItem*>(fovItem)));

    auto colItem = new SourceItem<CTBaseSource, floating>(
        src,
        [=](floating val) { src->setCollimation(val); },
        [=]() { return src->collimation(); });
    nodes.append(qMakePair(QString("Collimation [mm]"), static_cast<QStandardItem*>(colItem)));

    auto tubeNode = new QStandardItem("X-ray tube settings");
    setupTube(src, tubeNode);
    nodes.append(qMakePair(QString(), tubeNode));

    auto heelItem = new SourceItem<CTBaseSource, bool>(
        src,
        [=](const bool val) { src->setModelHeelEffect(val); },
        [=]() -> auto { return src->modelHeelEffect(); });
    nodes.append(qMakePair(QString("Model Heel effect"), static_cast<QStandardItem*>(heelItem)));

    auto bowItem = new SourceItem<CTBaseSource, std::shared_ptr<BowTieFilter>>(
        src,
        [=](std::shared_ptr<BowTieFilter> val) { src->setBowTieFilter(val); },
        [=]() { return src->bowTieFilter(); });
    nodes.append(qMakePair(QString("Select bowtie filter"), static_cast<QStandardItem*>(bowItem)));

    auto gangItem = new SourceItem<CTBaseSource, floating>(
        src,
        [=](floating val) { src->setGantryTiltAngleDeg(val); },
        [=]() { return src->gantryTiltAngleDeg(); });
    nodes.append(qMakePair(QString("Gantry tilt angle [deg]"), static_cast<QStandardItem*>(gangItem)));

    auto l3Item = new SourceItem<CTBaseSource, floating>(
        src,
        [=](floating val) { src->setScanLenght(val); },
        [=]() { return src->scanLenght(); });
    nodes.append(qMakePair(QString("Scan lenght [mm]"), static_cast<QStandardItem*>(l3Item)));

    auto l14Item = new SourceItem<CTBaseSource, std::uint64_t>(
        src,
        [=](auto val) { src->setHistoriesPerExposure(val); },
        [=]() { return src->historiesPerExposure(); });
    nodes.append(qMakePair(QString("Histories per exposure"), static_cast<QStandardItem*>(l14Item)));

    auto l5Item = new SourceItem<CTBaseSource, floating>(
        src,
        [=](floating val) { src->setCtdiVol(val); },
        [=]() { return src->ctdiVol(); });
    nodes.append(qMakePair(QString("CTDIvol mean value [mGy] "), static_cast<QStandardItem*>(l5Item)));

    auto l6Item = new SourceItem<CTBaseSource, std::uint64_t>(
        src,
        [=](auto val) { src->setCtdiPhantomDiameter(val); },
        [=]() { return src->ctdiPhantomDiameter(); });
    nodes.append(qMakePair(QString("CTDI phantom diameter [mm] "), static_cast<QStandardItem*>(l6Item)));

    auto sangItem = new SourceItem<CTBaseSource, floating>(
        src,
        [=](floating val) { src->setStartAngleDeg(val); },
        [=]() { return src->startAngleDeg(); });
    nodes.append(qMakePair(QString("Start angle [deg]"), static_cast<QStandardItem*>(sangItem)));

    addModelItems(nodes, parent);
}
void SourceModel::setupCTSource(std::shared_ptr<CTSource> src, QStandardItem* parent)
{
    setupCTBaseSource(std::static_pointer_cast<CTBaseSource>(src), parent);

    //CT parameters
    QVector<QPair<QString, QStandardItem*>> nodes;

    auto l1Item = new SourceItem<CTSource, floating>(
        src,
        [=](floating val) { src->setExposureAngleStepDeg(val); },
        [=]() { return src->exposureAngleStepDeg(); });
    nodes.append(qMakePair(QString("Step angle [deg]"), static_cast<QStandardItem*>(l1Item)));

    auto xcareItem = new QStandardItem("Organ exposure control");
    setupXCare(src, xcareItem);
    nodes.append(qMakePair(QString(), xcareItem));

    auto l4Item = new SourceItem<CTSource, std::uint64_t>(
        src,
        [=](auto val) {},
        std::bind(&CTSource::totalExposures, src));
    nodes.append(qMakePair(QString("Total number of exposures"), static_cast<QStandardItem*>(l4Item)));
    l4Item->setEditable(false);

    auto aecItem = new SourceItem<CTSource, std::shared_ptr<AECFilter>>(
        src,
        [=](std::shared_ptr<AECFilter> val) { src->setAecFilter(val); },
        [=]() { return src->aecFilter(); });
    nodes.append(qMakePair(QString("Select tube current modulation profile"), static_cast<QStandardItem*>(aecItem)));

    addModelItems(nodes, parent);
}

void SourceModel::setupCTAxialSource(std::shared_ptr<CTAxialSource> src, QStandardItem* parent)
{
    //sourve root node
    auto sourceItem = new QStandardItem("CT Axial Source");

    //standard parameters
    setupCTSource(std::static_pointer_cast<CTSource>(src), sourceItem);

    QVector<QPair<QString, QStandardItem*>> nodes;

    auto l2Item = new SourceItem<CTAxialSource, floating>(
        src,
        [=](floating val) { src->setStep(val); },
        [=]() { return src->step(); });
    nodes.append(qMakePair(QString("Rotation step [mm]"), static_cast<QStandardItem*>(l2Item)));

    addModelItems(nodes, sourceItem);

    //adding source
    parent->appendRow(sourceItem);
}

void SourceModel::setupCTSpiralSource(std::shared_ptr<CTSpiralSource> src, QStandardItem* parent)
{
    //sourve root node
    auto sourceItem = new QStandardItem("CT Spiral Source");

    //standard parameters
    setupCTSource(std::static_pointer_cast<CTSource>(src), sourceItem);

    QVector<QPair<QString, QStandardItem*>> nodes;

    auto l2Item = new SourceItem<CTSpiralSource, floating>(
        src,
        [=](floating val) { src->setPitch(val); },
        [=]() { return src->pitch(); });
    nodes.append(qMakePair(QString("Pitch"), static_cast<QStandardItem*>(l2Item)));

    addModelItems(nodes, sourceItem);

    //adding source
    parent->appendRow(sourceItem);
}

void SourceModel::setupCTDualSource(std::shared_ptr<CTSpiralDualSource> src, QStandardItem* parent)
{
    auto sourceItem = new QStandardItem("CT Dual Source");

    setupSource(std::static_pointer_cast<Source>(src), sourceItem);

    QVector<QPair<QString, QStandardItem*>> commonNodes;
    QVector<QPair<QString, QStandardItem*>> tubeANodes;
    QVector<QPair<QString, QStandardItem*>> tubeBNodes;

    auto sddItem = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setSourceDetectorDistance(val); },
        [=]() { return src->sourceDetectorDistance(); });
    tubeANodes.append(qMakePair(QString("Source detector distance [mm]"), static_cast<QStandardItem*>(sddItem)));
    auto sddItemb = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setSourceDetectorDistanceB(val); },
        [=]() { return src->sourceDetectorDistanceB(); });
    tubeBNodes.append(qMakePair(QString("Source detector distance [mm]"), static_cast<QStandardItem*>(sddItemb)));

    auto fovItem = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setFieldOfView(val); },
        [=]() { return src->fieldOfView(); });
    tubeANodes.append(qMakePair(QString("Field of view [mm]"), static_cast<QStandardItem*>(fovItem)));
    auto fovItemb = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setFieldOfViewB(val); },
        [=]() { return src->fieldOfViewB(); });
    tubeBNodes.append(qMakePair(QString("Field of view [mm]"), static_cast<QStandardItem*>(fovItemb)));

    auto colItem = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setCollimation(val); },
        [=]() { return src->collimation(); });
    commonNodes.append(qMakePair(QString("Collimation [mm]"), static_cast<QStandardItem*>(colItem)));

    auto tubeNodeA = new QStandardItem("X-ray tube A settings");
    auto tubeNodeB = new QStandardItem("X-ray tube B settings");

    setupTube(src, tubeNodeA);
    setupTubeB(src, tubeNodeB);

    auto bowItema = new SourceItem<CTSpiralDualSource, std::shared_ptr<BowTieFilter>>(
        src,
        [=](std::shared_ptr<BowTieFilter> val) { src->setBowTieFilter(val); },
        [=]() { return src->bowTieFilter(); });
    tubeANodes.append(qMakePair(QString("Select bowtie filter"), static_cast<QStandardItem*>(bowItema)));
    auto bowItemb = new SourceItem<CTSpiralDualSource, std::shared_ptr<BowTieFilter>>(
        src,
        [=](std::shared_ptr<BowTieFilter> val) { src->setBowTieFilterB(val); },
        [=]() { return src->bowTieFilterB(); });
    tubeBNodes.append(qMakePair(QString("Select bowtie filter"), static_cast<QStandardItem*>(bowItemb)));

    auto masItema = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setTubeAmas(val); },
        [=]() { return src->tubeAmas(); });
    tubeANodes.append(qMakePair(QString("Relative tube current for tube A [mAs]"), static_cast<QStandardItem*>(masItema)));
    auto masItemb = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setTubeBmas(val); },
        [=]() { return src->tubeBmas(); });
    tubeBNodes.append(qMakePair(QString("Relative tube current for tube B [mAs]"), static_cast<QStandardItem*>(masItemb)));

    auto aecItema = new SourceItem<CTSource, std::shared_ptr<AECFilter>>(
        src,
        [=](std::shared_ptr<AECFilter> val) { src->setAecFilter(val); },
        [=]() { return src->aecFilter(); });
    commonNodes.append(qMakePair(QString("Select tube current modulation profile"), static_cast<QStandardItem*>(aecItema)));

    auto xcareItem = new QStandardItem("Organ exposure control");
    setupXCare(src, xcareItem);
    commonNodes.append(qMakePair(QString(), xcareItem));

    auto gangItem = new SourceItem<CTSource, floating>(
        src,
        [=](floating val) { src->setGantryTiltAngleDeg(val); },
        [=]() { return src->gantryTiltAngleDeg(); });
    commonNodes.append(qMakePair(QString("Gantry tilt angle [deg]"), static_cast<QStandardItem*>(gangItem)));

    auto sangItema = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setStartAngleDeg(val); },
        [=]() { return src->startAngleDeg(); });
    tubeANodes.append(qMakePair(QString("Start angle [deg]"), static_cast<QStandardItem*>(sangItema)));
    auto sangItemb = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setStartAngleDegB(val); },
        [=]() { return src->startAngleDegB(); });
    tubeBNodes.append(qMakePair(QString("Start angle [deg]"), static_cast<QStandardItem*>(sangItemb)));

    auto l1Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setExposureAngleStepDeg(val); },
        [=]() { return src->exposureAngleStepDeg(); });
    commonNodes.append(qMakePair(QString("Step angle [deg]"), static_cast<QStandardItem*>(l1Item)));

    auto l3Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setScanLenght(val); },
        [=]() { return src->scanLenght(); });
    commonNodes.append(qMakePair(QString("Scan lenght [mm]"), static_cast<QStandardItem*>(l3Item)));

    auto pitchItem = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setPitch(val); },
        [=]() { return src->pitch(); });
    commonNodes.append(qMakePair(QString("Pitch [A.U]"), static_cast<QStandardItem*>(pitchItem)));

    auto l4Item = new SourceItem<CTSpiralDualSource, std::uint64_t>(
        src,
        [=](auto val) {},
        std::bind(&CTSource::totalExposures, src));
    commonNodes.append(qMakePair(QString("Total number of exposures"), static_cast<QStandardItem*>(l4Item)));
    l4Item->setEditable(false);

    auto l14Item = new SourceItem<CTSpiralDualSource, std::uint64_t>(
        src,
        [=](auto val) { src->setHistoriesPerExposure(val); },
        [=]() { return src->historiesPerExposure(); });
    commonNodes.append(qMakePair(QString("Histories per exposure"), static_cast<QStandardItem*>(l14Item)));

    auto l5Item = new SourceItem<CTSpiralDualSource, floating>(
        src,
        [=](floating val) { src->setCtdiVol(val); },
        [=]() { return src->ctdiVol(); });
    commonNodes.append(qMakePair(QString("CTDIvol for scan [mGy] "), static_cast<QStandardItem*>(l5Item)));

    auto l6Item = new SourceItem<CTSpiralDualSource, std::uint64_t>(
        src,
        [=](auto val) { src->setCtdiPhantomDiameter(val); },
        [=]() { return src->ctdiPhantomDiameter(); });
    commonNodes.append(qMakePair(QString("CTDI phantom diameter [mm] "), static_cast<QStandardItem*>(l6Item)));

    addModelItems(commonNodes, sourceItem);

    addModelItems(tubeANodes, tubeNodeA);

    addModelItems(tubeBNodes, tubeNodeB);

    sourceItem->appendRow(tubeNodeA);
    sourceItem->appendRow(tubeNodeB);

    //adding source
    parent->appendRow(sourceItem);
}

void SourceModel::setupCTTopogramSource(std::shared_ptr<CTTopogramSource> src, QStandardItem* parent)
{
    //sourve root node
    auto sourceItem = new QStandardItem("CT Topogram");
    //setupSource(std::static_pointer_cast<Source>(src), sourceItem);
    setupCTBaseSource(std::static_pointer_cast<CTBaseSource>(src), sourceItem);
    QVector<QPair<QString, QStandardItem*>> nodes;

    auto l2Item = new SourceItem<CTTopogramSource, std::uint64_t>(
        src,
        [=](std::uint64_t val) { },
        [=]() -> auto { return src->totalExposures(); });
    nodes.append(qMakePair(QString("Total number of exposures"), static_cast<QStandardItem*>(l2Item)));

    addModelItems(nodes, sourceItem);
    this->invisibleRootItem()->appendRow(sourceItem);
}

void SourceModel::setupDXSource(std::shared_ptr<DXSource> src, QStandardItem* parent)
{
    //sourve root node
    auto sourceItem = new QStandardItem("DX Source");

    //standard parameters
    setupSource(std::static_pointer_cast<Source>(src), sourceItem);

    QVector<QPair<QString, QStandardItem*>> nodes;

    auto tubeNode = new QStandardItem("X-ray tube settings");
    setupTube(src, tubeNode);
    nodes.append(qMakePair(QString(), tubeNode));

    auto heelItem = new SourceItem<DXSource, bool>(
        src,
        [=](const bool val) { src->setModelHeelEffect(val); },
        [=]() -> auto { return src->modelHeelEffect(); });
    nodes.append(qMakePair(QString("Model Heel effect"), static_cast<QStandardItem*>(heelItem)));

    //DX parameters

    auto sourceAngleItem = new SourceItem<DXSource, std::array<floating, 2>>(
        src,
        [=](const auto& val) { src->setSourceAnglesDeg(val); },
        [=]() { return src->sourceAnglesDeg(); });
    nodes.append(qMakePair(QString("Source angles (primary angle, secondary angle) [deg]"), static_cast<QStandardItem*>(sourceAngleItem)));

    auto tubeRotItem = new SourceItem<DXSource, floating>(
        src,
        [=](const auto& val) { src->setTubeRotationDeg(val); },
        [=]() { return src->tubeRotationDeg(); });
    nodes.append(qMakePair(QString("X-ray tube rotation angle [deg]"), static_cast<QStandardItem*>(tubeRotItem)));

    auto l1Item = new SourceItem<DXSource, std::array<floating, 2>>(
        src,
        [=](const auto& val) { src->setCollimationAnglesDeg(val); },
        [=]() { return src->collimationAnglesDeg(); });
    nodes.append(qMakePair(QString("Collimation angles [deg]"), static_cast<QStandardItem*>(l1Item)));

    auto l11Item = new SourceItem<DXSource, std::array<floating, 2>>(
        src,
        [=](const auto& val) { src->setFieldSize(val); },
        [=]() { return src->fieldSize(); });
    nodes.append(qMakePair(QString("Field size [mm]"), static_cast<QStandardItem*>(l11Item)));

    auto l12Item = new SourceItem<DXSource, floating>(
        src,
        [=](const auto& val) { src->setSourceDetectorDistance(val); },
        [=]() { return src->sourceDetectorDistance(); });
    nodes.append(qMakePair(QString("Source detector distance [mm]"), static_cast<QStandardItem*>(l12Item)));

    auto l2Item = new SourceItem<DXSource, std::size_t>(
        src,
        [=](std::size_t val) { src->setTotalExposures(val); },
        std::bind(&DXSource::totalExposures, src));
    nodes.append(qMakePair(QString("Total number of exposures"), static_cast<QStandardItem*>(l2Item)));

    auto l14Item = new SourceItem<DXSource, std::uint64_t>(
        src,
        [=](auto val) { src->setHistoriesPerExposure(val); },
        [=]() { return src->historiesPerExposure(); });
    nodes.append(qMakePair(QString("Histories per exposure"), static_cast<QStandardItem*>(l14Item)));

    auto l5Item = new SourceItem<DXSource, floating>(
        src,
        [=](floating val) { src->setDap(val); },
        [=]() { return src->dap(); });
    nodes.append(qMakePair(QString("Dose Area Product for beam [Gycm2]"), static_cast<QStandardItem*>(l5Item)));

    addModelItems(nodes, sourceItem);

    //adding source
    this->invisibleRootItem()->appendRow(sourceItem);
}

void SourceModel::sourceDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles)
{
    for (auto actor : m_actors)
        actor->update();
    emit actorsChanged();
}
