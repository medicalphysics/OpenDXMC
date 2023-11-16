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

Copyright 2024 Erlend Andersen
*/

#include <beamsettingsmodel.hpp>

#include <array>
#include <charconv>
#include <concepts>
#include <functional>

class LabelItem : public QStandardItem {
public:
    LabelItem(const QString& txt)
        : QStandardItem(txt)
    {
        setEditable(false);
    }
};

QString arrayToString(const std::array<double, 3>& arr)
{
    auto str = QString::number(arr[0]) + ", " + QString::number(arr[1]) + ", " + QString::number(arr[2]);
    return str;
}

template <std::size_t N>
std::optional<std::array<double, N>> stringToArray(const QString& str)
{
    std::array<double, N> r;
    auto list = str.split(',');
    if (list.size() < N)
        return std::nullopt;

    for (std::size_t i = 0; i < N; ++i) {
        const auto& s = list[i].trimmed().toStdString();

        auto res = std::from_chars(s.data(), s.data() + s.size(), r[i]);
        if (res.ec != std::errc {})
            return std::nullopt;
    }
    return std::make_optional(r);
}

template <typename G, typename T>
concept Getter = requires(G func, T val) {
    {
        func()
    } -> std::convertible_to<T>;
};

template <typename G, typename T>
concept Setter = requires(G func, T val) {
    func(val);
};

template <typename T, Setter<T> S, Getter<T> G>
class VectorItem : public QStandardItem {
public:
    VectorItem(T data, S setter, G getter, bool editable = true)
        : m_setter(setter)
        , m_getter(getter)
        , QStandardItem()
    {
        emitDataChanged();
        setEditable(editable);
    }

    QVariant data(int role = Qt::UserRole + 1) const override
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if constexpr (std::is_convertible_v<T, std::array<double, 3>>) {
                return arrayToString(m_getter());
            } else {
                return QVariant(m_getter());
            }
        }
        return QStandardItem::data(role);
    }

    void setData(const QVariant& value, int role = Qt::UserRole + 1) override
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if constexpr (std::is_convertible_v<T, std::array<double, 3>>) {
                auto str = value.toString();
                auto r = stringToArray<3>(str);
                if (r) {
                    m_setter(r.value());
                    QStandardItem::setData(value, role);
                }
            } else {
                auto val = get<T>(value);
                m_setter(val);
                QStandardItem::setData(value, role);
            }
        } else {
            QStandardItem::setData(value, role);
        }
    }

protected:
private:
    S m_setter;
    G m_getter;
};

BeamSettingsModel::BeamSettingsModel(QObject* parent)
    : QStandardItemModel(parent)
{
    QStringList header;
    header.append(tr("Settings"));
    header.append(tr("Value"));
    setHorizontalHeaderLabels(header);
}

void addItem(QStandardItem* parent, QString label, auto setter, auto getter, bool editable = true)
{
    QList<QStandardItem*> row(2);
    row[0] = new LabelItem(label);
    auto value = getter();
    row[1] = new VectorItem(value, setter, getter, editable);
    parent->appendRow(row);
}

void BeamSettingsModel::addDXBeam()
{
    auto parent = invisibleRootItem();

    auto root = new LabelItem(tr("DX Beam"));
    appendRow(root);

    auto beam = std::make_shared<DXBeam>();
    auto setter = [=](std::array<double, 3> d) { beam->setPosition(d); };
    auto getter = [=]() -> std::array<double, 3> { return beam->position(); };
    auto t = beam->position();

    addItem(root, "Test 1", setter, getter);
}