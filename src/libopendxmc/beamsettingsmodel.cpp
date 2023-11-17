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

template <std::size_t N>
QString arrayToString(const std::array<double, N>& arr)
{
    QString str;
    for (std::size_t i = 0; i < N - 1; ++i)
        str += QString::number(arr[i]) + ", ";
    str += QString::number(arr[N - 1]);
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
class EditableItem : public QStandardItem {
public:
    EditableItem(T data, S setter, G getter, bool editable = true)
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
            if constexpr (std::is_convertible_v<T, std::array<double, 3>> || std::is_convertible_v<T, std::array<double, 2>>) {
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
            } else if constexpr (std::is_convertible_v<T, std::array<double, 2>>) {
                auto str = value.toString();
                auto r = stringToArray<2>(str);
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

private:
    S m_setter;
    G m_getter;
};

template <typename T, Getter<T> G>
class ReadableItem : public QStandardItem {
public:
    ReadableItem(T data, G getter)
        : m_getter(getter)
        , QStandardItem()
    {
        setEditable(false);
        emitDataChanged();
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

private:
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
    row[1] = new EditableItem(value, setter, getter, editable);
    parent->appendRow(row);
}

void addItem(QStandardItem* parent, QString label, auto getter)
{
    QList<QStandardItem*> row(2);
    row[0] = new LabelItem(label);
    auto value = getter();
    row[1] = new ReadableItem(value, getter);
    parent->appendRow(row);
}

void BeamSettingsModel::addDXBeam()
{
    auto parent = invisibleRootItem();

    auto root = new LabelItem(tr("DX Beam"));
    appendRow(root);

    auto beam = std::make_shared<Beam>(DXBeam());
    m_beams.push_back(beam);

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setPosition(d);
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.position();
        };

        addItem(root, "Tube position [cm]", setter, getter);
    }

    {
        std::get<DXBeam>(*beam).setCollimationAnglesDeg(15, 15);
        auto setter = [=](std::array<double, 2> d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setCollimationAnglesDeg(d);
        };
        auto getter = [=]() -> std::array<double, 2> {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.collimationAnglesDeg();
        };

        addItem(root, "Collimation [Deg]", setter, getter);
    }

    auto tubeItem = new LabelItem(tr("Tube"));
    root->appendRow(tubeItem);
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setTubeVoltage(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            const auto& tube = dx.tube();
            return tube.voltage();
        };
        addItem(tubeItem, "Tube potential [kV]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setTubeAnodeAngleDeg(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            const auto& tube = dx.tube();
            return tube.anodeAngleDeg();
        };
        addItem(tubeItem, "Tube anode angle [deg]", setter, getter);
    }
    {
        std::get<DXBeam>(*beam).addTubeFiltrationMaterial(13, 2);
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.addTubeFiltrationMaterial(13, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            const auto& tube = dx.tube();
            return tube.filtration(13);
        };
        addItem(tubeItem, "Tube Al filtration [mm]", setter, getter);
    }
    {
        std::get<DXBeam>(*beam).addTubeFiltrationMaterial(29, 0.1);
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.addTubeFiltrationMaterial(29, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            const auto& tube = dx.tube();
            return tube.filtration(29);
        };
        addItem(tubeItem, "Tube Cu filtration [mm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.addTubeFiltrationMaterial(50, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            const auto& tube = dx.tube();
            return tube.filtration(50);
        };
        addItem(tubeItem, "Tube Sn filtration [mm]", setter, getter);
    }
    {
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.tubeAlHalfValueLayer();
        };
        addItem(tubeItem, "Tube half value layer [mmAl]", getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setDAPvalue(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.DAPvalue();
        };

        addItem(root, "Dose area product [mGycm^2]", setter, getter);
    }
    {
        auto setter = [=](std::uint64_t d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setNumberOfExposures(d);
        };
        auto getter = [=]() -> std::uint64_t {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.numberOfExposures();
        };
        addItem(root, "Number of exposures", setter, getter);
    }
    {
        auto setter = [=](std::uint64_t d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setNumberOfParticlesPerExposure(d);
        };
        auto getter = [=]() -> std::uint64_t {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.numberOfParticlesPerExposure();
        };
        addItem(root, "Particles per exposure", setter, getter);
    }
    {
        auto getter = [=]() -> std::uint64_t {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.numberOfParticles();
        };
        addItem(root, "Total number of particles", getter);
    }
}

void BeamSettingsModel::addCTSpiralBeam()
{
    auto parent = invisibleRootItem();

    auto root = new LabelItem(tr("CT Spiral Beam"));
    appendRow(root);

    auto beam = std::make_shared<Beam>(CTSpiralBeam());
    m_beams.push_back(beam);

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setStartPosition(d);
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.startPosition();
        };

        addItem(root, "Start position [cm]", setter, getter);
    }
    {
        auto setter = [=](std::array<double, 3> d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setStopPosition(d);
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.stopPosition();
        };

        addItem(root, "Stop position [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setScanFieldOfView(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.scanFieldOfView();
        };
        addItem(root, "Scan FOV [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setSourceDetectorDistance(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.sourceDetectorDistance();
        };
        addItem(root, "Source detector distance [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setCollimation(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.collimation();
        };
        addItem(root, "Total collimation [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setStartAngleDeg(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.startAngleDeg();
        };
        addItem(root, "Set start angle [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setStepAngleDeg(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.stepAngleDeg();
        };
        addItem(root, "Set angle step [deg]", setter, getter);
    }

    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setPitch(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.pitch();
        };
        addItem(root, "Pitch", setter, getter);
    }

    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setCTDIvol(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.CTDIvol();
        };
        addItem(root, "CTDIvol [mGycm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setCTDIdiameter(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.CTDIdiameter();
        };
        addItem(root, "CTDI phantom diameter [cm]", setter, getter);
    }

    auto tubeItem = new LabelItem(tr("Tube"));
    root->appendRow(tubeItem);
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setTubeVoltage(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            const auto& tube = ct.tube();
            return tube.voltage();
        };
        addItem(tubeItem, "Tube potential [kV]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setTubeAnodeAngleDeg(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            const auto& tube = ct.tube();
            return tube.anodeAngleDeg();
        };
        addItem(tubeItem, "Tube anode angle [deg]", setter, getter);
    }
    {
        std::get<CTSpiralBeam>(*beam).addTubeFiltrationMaterial(13, 9);
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.addTubeFiltrationMaterial(13, d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            const auto& tube = ct.tube();
            return tube.filtration(13);
        };
        addItem(tubeItem, "Tube Al filtration [mm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.addTubeFiltrationMaterial(29, d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            const auto& tube = ct.tube();
            return tube.filtration(29);
        };
        addItem(tubeItem, "Tube Cu filtration [mm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.addTubeFiltrationMaterial(50, d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            const auto& tube = ct.tube();
            return tube.filtration(50);
        };
        addItem(tubeItem, "Tube Sn filtration [mm]", setter, getter);
    }
    {
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.tubeAlHalfValueLayer();
        };
        addItem(tubeItem, "Tube half value layer [mmAl]", getter);
    }
    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.numberOfExposures();
        };
        addItem(root, "Number of exposures", getter);
    }
    {
        auto setter = [=](std::uint64_t d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setNumberOfParticlesPerExposure(d);
        };
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.numberOfParticlesPerExposure();
        };
        addItem(root, "Particles per exposure", setter, getter);
    }
    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.numberOfParticles();
        };
        addItem(root, "Total number of particles", getter);
    }
}

void BeamSettingsModel::addCTSpiralDualSourceBeam()
{
}