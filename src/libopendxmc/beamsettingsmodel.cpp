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
            if constexpr (std::is_convertible_v<T, std::array<double, 2>> || std::is_convertible_v<T, std::array<double, 3>> || std::is_convertible_v<T, std::array<double, 6>>) {
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
            if constexpr (std::is_convertible_v<T, std::array<double, 2>>) {
                auto str = value.toString();
                auto r = stringToArray<2>(str);
                if (r) {
                    m_setter(r.value());
                    QStandardItem::setData(value, role);
                }
            } else if constexpr (std::is_convertible_v<T, std::array<double, 3>>) {
                auto str = value.toString();
                auto r = stringToArray<3>(str);
                if (r) {
                    m_setter(r.value());
                    QStandardItem::setData(value, role);
                }
            } else if constexpr (std::is_convertible_v<T, std::array<double, 6>>) {
                auto str = value.toString();
                auto r = stringToArray<6>(str);
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
    connect(this, &QStandardItemModel::dataChanged, [this](const auto& arg) { emit this->requestRender(); });
}

void BeamSettingsModel::updateImageData(std::shared_ptr<DataContainer> data)
{
    m_image = data;
}

void BeamSettingsModel::deleteBeam(int index)
{
    auto root = invisibleRootItem();
    if (root->rowCount() > index) {
        root->removeRows(index, 1);
        auto [beam, actor] = m_beams[index];
        m_beams.erase(m_beams.begin() + index);
        emit beamActorRemoved(actor);
    }
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

template <typename T>
void addTubeItems(LabelItem* tubeItem, std::shared_ptr<Beam> beam)
{
    {
        auto setter = [=](double d) {
            auto& dx = std::get<T>(*beam);
            dx.setTubeVoltage(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            const auto& tube = dx.tube();
            return tube.voltage();
        };
        addItem(tubeItem, "Tube potential [kV]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<T>(*beam);
            dx.setTubeAnodeAngleDeg(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            const auto& tube = dx.tube();
            return tube.anodeAngleDeg();
        };
        addItem(tubeItem, "Tube anode angle [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<T>(*beam);
            dx.addTubeFiltrationMaterial(13, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            const auto& tube = dx.tube();
            return tube.filtration(13);
        };
        addItem(tubeItem, "Tube Al filtration [mm]", setter, getter);
    }
    {

        auto setter = [=](double d) {
            auto& dx = std::get<T>(*beam);
            dx.addTubeFiltrationMaterial(29, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            const auto& tube = dx.tube();
            return tube.filtration(29);
        };
        addItem(tubeItem, "Tube Cu filtration [mm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<T>(*beam);
            dx.addTubeFiltrationMaterial(50, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            const auto& tube = dx.tube();
            return tube.filtration(50);
        };
        addItem(tubeItem, "Tube Sn filtration [mm]", setter, getter);
    }
    {
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            return dx.tubeAlHalfValueLayer();
        };
        addItem(tubeItem, "Tube half value layer [mmAl]", getter);
    }
}

void BeamSettingsModel::addDXBeam()
{
    auto root = new LabelItem(tr("DX Beam"));

    constexpr std::array<double, 3> pos_init = { 0, -50, 0 };
    constexpr std::array<std::array<double, 3>, 2> cos_init = { 0, 0, 1, 1, 0, 0 };
    const std::map<std::size_t, double> filt_init = { { 13, 2.0 }, { 29, 0.1 } };

    auto beam = std::make_shared<Beam>(DXBeam(pos_init, cos_init, filt_init));
    auto beamActor = std::make_shared<BeamActorContainer>(beam);
    m_beams.push_back(std::make_pair(beam, beamActor));

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setPosition(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.position();
        };
        addItem(root, "Tube position [cm]", setter, getter);
    }

    {
        auto setter = [=](std::array<double, 6> d) {
            auto& dx = std::get<DXBeam>(*beam);
            std::array<double, 3> cosx, cosy;
            for (std::size_t i = 0; i < 3; ++i) {
                cosx[i] = d[i];
                cosy[i] = d[i + 3];
            }
            dx.setDirectionCosines(cosx, cosy);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 6> {
            auto& dx = std::get<DXBeam>(*beam);
            const auto& cos = dx.directionCosines();
            std::array<double, 6> r = {
                cos[0][0],
                cos[0][1],
                cos[0][2],
                cos[1][0],
                cos[1][1],
                cos[1][2]
            };
            return r;
        };
        addItem(root, "Tube cosine vectors [x1, y1, z1, x2, y2, z2]", setter, getter);
    }

    {
        std::get<DXBeam>(*beam).setCollimationAnglesDeg(15, 15);
        auto setter = [=](std::array<double, 2> d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setCollimationAnglesDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 2> {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.collimationAnglesDeg();
        };
        addItem(root, "Collimation [Deg]", setter, getter);
    }

    auto tubeItem = new LabelItem(tr("Tube"));
    root->appendRow(tubeItem);
    addTubeItems<DXBeam>(tubeItem, beam);

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
    beamActor->update();
    emit beamActorAdded(beamActor);
    appendRow(root);
}

void BeamSettingsModel::addCTSpiralBeam()
{
    auto root = new LabelItem(tr("CT Spiral Beam"));

    std::shared_ptr<Beam> beam = nullptr;

    if (m_image) {
        const auto& s = m_image->spacing();
        const auto& d = m_image->dimensions();
        const std::array<double, 3> start_init = { 0, 0, -(s[2] * d[2]) / 2 };
        const std::array<double, 3> stop_init = { 0, 0, (s[2] * d[2]) / 2 };
        const std::map<std::size_t, double> filt_init = { { 13, 9.0 } };
        beam = std::make_shared<Beam>(CTSpiralBeam(start_init, stop_init, filt_init));
    } else {
        constexpr std::array<double, 3> start_init = { 0, 0, -20 };
        constexpr std::array<double, 3> stop_init = { 0, 0, 20 };
        const std::map<std::size_t, double> filt_init = { { 13, 9.0 } };
        beam = std::make_shared<Beam>(CTSpiralBeam(start_init, stop_init, filt_init));
    }

    auto beamActor = std::make_shared<BeamActorContainer>(beam);
    m_beams.push_back(std::make_pair(beam, beamActor));

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            ct.setStartPosition(d);
            beamActor->update();
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
            beamActor->update();
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
            beamActor->update();
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
            beamActor->update();
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
            beamActor->update();
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
            beamActor->update();
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
            beamActor->update();
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
            beamActor->update();
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
    addTubeItems<CTSpiralBeam>(tubeItem, beam);

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

    beamActor->update();
    emit beamActorAdded(beamActor);
    appendRow(root);
}

void BeamSettingsModel::addCTSpiralDualEnergyBeam()
{
    auto root = new LabelItem(tr("CT Spiral Dual Energy Beam"));

    std::shared_ptr<Beam> beam = nullptr;

    if (m_image) {
        const auto& s = m_image->spacing();
        const auto& d = m_image->dimensions();
        const std::array<double, 3> start_init = { 0, 0, -(s[2] * d[2]) / 2 };
        const std::array<double, 3> stop_init = { 0, 0, (s[2] * d[2]) / 2 };
        const std::map<std::size_t, double> filt_init = { { 13, 9.0 } };
        beam = std::make_shared<Beam>(CTSpiralDualEnergyBeam(start_init, stop_init, filt_init));
    } else {
        constexpr std::array<double, 3> start_init = { 0, 0, -20 };
        constexpr std::array<double, 3> stop_init = { 0, 0, 20 };
        const std::map<std::size_t, double> filt_init = { { 13, 9.0 } };
        beam = std::make_shared<Beam>(CTSpiralDualEnergyBeam(start_init, stop_init, filt_init));
    }

    auto beamActor = std::make_shared<BeamActorContainer>(beam);
    m_beams.push_back(std::make_pair(beam, beamActor));

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setStartPosition(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.startPosition();
        };

        addItem(root, "Start position [cm]", setter, getter);
    }
    {
        auto setter = [=](std::array<double, 3> d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setStopPosition(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.stopPosition();
        };

        addItem(root, "Stop position [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setScanFieldOfViewA(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.scanFieldOfViewB();
        };
        addItem(root, "Scan FOV Tube A [cm]", setter, getter);
        auto setterB = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setScanFieldOfViewA(d);
            beamActor->update();
        };
        auto getterB = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.scanFieldOfViewB();
        };
        addItem(root, "Scan FOV Tube B [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setSourceDetectorDistance(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.sourceDetectorDistance();
        };
        addItem(root, "Source detector distance [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setCollimation(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.collimation();
        };
        addItem(root, "Total collimation [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setStartAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.startAngleDeg();
        };
        addItem(root, "Set start angle [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setStepAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.stepAngleDeg();
        };
        addItem(root, "Set angle step [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setTubeBoffsetAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.tubeBoffsetAngleDeg();
        };
        addItem(root, "Tube B offset angle [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setPitch(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.pitch();
        };
        addItem(root, "Pitch", setter, getter);
    }

    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setCTDIvol(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.CTDIvol();
        };
        addItem(root, "CTDIvol [mGycm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setCTDIdiameter(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.CTDIdiameter();
        };
        addItem(root, "CTDI phantom diameter [cm]", setter, getter);
    }

    auto tubeAItem = new LabelItem(tr("Tube A"));
    root->appendRow(tubeAItem);
    std::get<CTSpiralDualEnergyBeam>(*beam).addTubeAFiltrationMaterial(13, 9);
    auto tubeBItem = new LabelItem(tr("Tube B"));
    root->appendRow(tubeBItem);
    std::get<CTSpiralDualEnergyBeam>(*beam).addTubeBFiltrationMaterial(13, 9);
    {
        auto setter = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.setTubeAVoltage(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeA();
            return tube.voltage();
        };
        auto setterB = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.setTubeBVoltage(d);
        };
        auto getterB = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeB();
            return tube.voltage();
        };
        addItem(tubeAItem, "Tube A potential [kV]", setter, getter);
        addItem(tubeBItem, "Tube B potential [kV]", setterB, getterB);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.setTubesAnodeAngleDeg(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeA();
            return tube.anodeAngleDeg();
        };

        addItem(tubeAItem, "Tubes anode angle [deg]", setter, getter);
        addItem(tubeBItem, "Tubes anode angle [deg]", getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.addTubeAFiltrationMaterial(13, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeA();
            return tube.filtration(13);
        };
        auto setterB = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.addTubeBFiltrationMaterial(13, d);
        };
        auto getterB = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeB();
            return tube.filtration(13);
        };
        addItem(tubeAItem, "Tube A Al filtration [mm]", setter, getter);
        addItem(tubeBItem, "Tube B Al filtration [mm]", setterB, getterB);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.addTubeAFiltrationMaterial(29, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeA();
            return tube.filtration(29);
        };
        auto setterB = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.addTubeBFiltrationMaterial(29, d);
        };
        auto getterB = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeB();
            return tube.filtration(29);
        };
        addItem(tubeAItem, "Tube A Cu filtration [mm]", setter, getter);
        addItem(tubeBItem, "Tube B Cu filtration [mm]", setterB, getterB);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.addTubeAFiltrationMaterial(50, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeA();
            return tube.filtration(50);
        };
        auto setterB = [=](double d) {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            dx.addTubeBFiltrationMaterial(50, d);
        };
        auto getterB = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& tube = dx.tubeB();
            return tube.filtration(50);
        };
        addItem(tubeAItem, "Tube A Sn filtration [mm]", setter, getter);
        addItem(tubeBItem, "Tube B Sn filtration [mm]", setterB, getterB);
    }
    {
        auto getter = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            return dx.tubeAAlHalfValueLayer();
        };
        auto getterB = [=]() -> double {
            auto& dx = std::get<CTSpiralDualEnergyBeam>(*beam);
            return dx.tubeBAlHalfValueLayer();
        };

        addItem(tubeAItem, "Tube half value layer [mmAl]", getter);
        addItem(tubeBItem, "Tube half value layer [mmAl]", getterB);
    }
    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.numberOfExposures();
        };
        addItem(root, "Number of exposures", getter);
    }
    {
        auto setter = [=](std::uint64_t d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setNumberOfParticlesPerExposure(d);
        };
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.numberOfParticlesPerExposure();
        };
        addItem(root, "Particles per exposure", setter, getter);
    }
    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.numberOfParticles();
        };
        addItem(root, "Total number of particles", getter);
    }
    beamActor->update();
    emit beamActorAdded(beamActor);
    appendRow(root);
}