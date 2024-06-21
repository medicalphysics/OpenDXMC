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
#include <bowtiefilterreader.hpp>

#include <array>
#include <charconv>

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
        if constexpr (std::is_same_v<T, bool>) {
            setCheckable(true);
        }
    }

    QVariant data(int role = Qt::UserRole + 1) const override
    {
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            if constexpr (std::is_convertible_v<T, std::array<double, 2>> || std::is_convertible_v<T, std::array<double, 3>> || std::is_convertible_v<T, std::array<double, 6>>) {
                return arrayToString(m_getter());
            } else if constexpr (std::is_same_v<T, bool>) {
                // We handle bool types by checked state
                return QVariant {};
            } else if constexpr (std::is_integral_v<T>) {
                return QVariant(static_cast<qulonglong>(m_getter()));
            } else {
                return QVariant::fromValue(m_getter());
            }
        }
        if constexpr (std::is_same_v<T, bool>) {
            if (role == Qt::CheckStateRole) {
                return QVariant(m_getter() ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
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
            } else if constexpr (std::is_same_v<T, bool>) {
                // We handle bool types with checkstate
                return;
            } else if constexpr (std::is_integral_v<T>) {
                qulonglong v = value.toULongLong();
                m_setter(static_cast<T>(v));
                QStandardItem::setData(value, role);
            } else {
                T val = get<T>(value);
                m_setter(val);
                QStandardItem::setData(value, role);
            }
        } else {
            if constexpr (std::is_same_v<T, bool>) {
                if (role == Qt::CheckStateRole) {
                    m_setter(value == Qt::CheckState::Checked);
                }
            }
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
                if constexpr (std::is_integral_v<T>) {
                    return QVariant(static_cast<qulonglong>(m_getter()));
                } else {
                    return QVariant(m_getter());
                }
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

    // reading bowtiefilters
    m_bowtieFilters = BowtieFilterReader::read(":bowtiefilters/bowtiefilters.json");
}

void BeamSettingsModel::updateImageData(std::shared_ptr<DataContainer> data)
{
    m_image = data;
}

const QMap<QString, BowtieFilter>& BeamSettingsModel::bowtieFilters() const
{
    return m_bowtieFilters;
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
        auto setter = [=](double d) {
            auto& dx = std::get<T>(*beam);
            dx.addTubeFiltrationMaterial(47, d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            const auto& tube = dx.tube();
            return tube.filtration(47);
        };
        addItem(tubeItem, "Tube Ag filtration [mm]", setter, getter);
    }
    {
        auto getter = [=]() -> double {
            auto& dx = std::get<T>(*beam);
            return dx.tubeAlHalfValueLayer();
        };
        addItem(tubeItem, "Tube half value layer [mmAl]", getter);
    }
}

void BeamSettingsModel::addBeam(std::shared_ptr<BeamActorContainer> actor)
{
    if (!actor)
        return;
    if (!actor->getBeam())
        return;

    if (std::holds_alternative<DXBeam>(*actor->getBeam())) {
        addDXBeam(actor);
    } else if (std::holds_alternative<CTSpiralBeam>(*actor->getBeam())) {
        addCTSpiralBeam(actor);
    } else if (std::holds_alternative<CTSpiralDualEnergyBeam>(*actor->getBeam())) {
        addCTSpiralDualEnergyBeam(actor);
    } else if (std::holds_alternative<CBCTBeam>(*actor->getBeam())) {
        addCBCTBeam(actor);
    } else if (std::holds_alternative<CTSequentialBeam>(*actor->getBeam())) {
        addCTSequentialBeam(actor);
    }
}

void BeamSettingsModel::addDXBeam(std::shared_ptr<BeamActorContainer> actor)
{
    auto root = new LabelItem(tr("DX Beam"));
    std::shared_ptr<Beam> beam = nullptr;
    if (actor) {
        beam = actor->getBeam();
        if (beam)
            if (!std::holds_alternative<DXBeam>(*beam))
                beam = nullptr;
    }
    if (!beam) {
        const std::map<std::size_t, double> filt_init = { { 13, 2.0 }, { 29, 0.1 } };
        beam = std::make_shared<Beam>(DXBeam(filt_init));
    }
    auto beamActor = std::make_shared<BeamActorContainer>(beam);
    m_beams.push_back(std::make_pair(beam, beamActor));

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setRotationCenter(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.rotationCenter();
        };
        addItem(root, "Rotation center [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setSourcePatientDistance(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.sourcePatientDistance();
        };
        addItem(root, "Source rotation center distance [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setPrimaryAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.primaryAngleDeg();
        };
        addItem(root, "Primary angle [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setSecondaryAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.secondaryAngleDeg();
        };
        addItem(root, "Secondary angle [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setSourceDetectorDistance(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.sourceDetectorDistance();
        };
        addItem(root, "Source detector distance [cm]", setter, getter);
    }
    {
        std::get<DXBeam>(*beam).setCollimation({ 20., 20. });
        auto setter = [=](std::array<double, 2> d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setCollimation(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 2> {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.collimation();
        };
        addItem(root, "Collimation [cm x cm]", setter, getter);
    }
    {
        auto setter = [=](std::array<double, 2> d) {
            auto& dx = std::get<DXBeam>(*beam);
            dx.setCollimationAnglesDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 2> {
            auto& dx = std::get<DXBeam>(*beam);
            return dx.collimationAnglesDeg();
        };
        addItem(root, "Collimation angles [Deg]", setter, getter);
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
        std::get<DXBeam>(*beam).setNumberOfExposures(64);
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
        std::get<DXBeam>(*beam).setNumberOfParticlesPerExposure(1e6);

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

void BeamSettingsModel::addCBCTBeam(std::shared_ptr<BeamActorContainer> actor)
{
    auto root = new LabelItem(tr("CBCT Beam"));
    std::shared_ptr<Beam> beam = nullptr;
    if (actor) {
        beam = actor->getBeam();
        if (beam)
            if (!std::holds_alternative<CBCTBeam>(*beam))
                beam = nullptr;
    }
    if (!beam) {
        const std::map<std::size_t, double> filt_init = { { 13, 2.0 }, { 29, 0.1 } };
        beam = std::make_shared<Beam>(CBCTBeam({ 0, 0, 0 }, { 0, 0, 1 }, filt_init));
    }
    auto beamActor = std::make_shared<BeamActorContainer>(beam);
    m_beams.push_back(std::make_pair(beam, beamActor));

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& dx = std::get<CBCTBeam>(*beam);
            dx.setIsocenter(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& dx = std::get<CBCTBeam>(*beam);
            return dx.isocenter();
        };
        addItem(root, "Isocenter [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& dx = std::get<CBCTBeam>(*beam);
            dx.setSourceDetectorDistance(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<CBCTBeam>(*beam);
            return dx.sourceDetectorDistance();
        };
        addItem(root, "Source detector distance [cm]", setter, getter);
    }

    {
        auto setter = [=](double d) {
            auto& ct = std::get<CBCTBeam>(*beam);
            ct.setStartAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CBCTBeam>(*beam);
            return ct.startAngleDeg();
        };
        addItem(root, "Set start angle [deg]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CBCTBeam>(*beam);
            ct.setStopAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CBCTBeam>(*beam);
            return ct.stopAngleDeg();
        };
        addItem(root, "Set stop angle [deg]", setter, getter);
    }
    {
        std::get<CBCTBeam>(*beam).setStepAngleDeg(5);
        auto setter = [=](double d) {
            auto& ct = std::get<CBCTBeam>(*beam);
            ct.setStepAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CBCTBeam>(*beam);
            return ct.stepAngleDeg();
        };
        addItem(root, "Set angle step [deg]", setter, getter);
    }

    {
        std::get<CBCTBeam>(*beam).setCollimationAnglesDeg({ 5, 5 });
        auto setter = [=](std::array<double, 2> d) {
            auto& dx = std::get<CBCTBeam>(*beam);
            dx.setCollimationAnglesDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 2> {
            auto& dx = std::get<CBCTBeam>(*beam);
            return dx.collimationAnglesDeg();
        };
        addItem(root, "Collimation angles [Deg]", setter, getter);
    }

    auto tubeItem = new LabelItem(tr("Tube"));
    root->appendRow(tubeItem);
    addTubeItems<CBCTBeam>(tubeItem, beam);
    {
        auto setter = [=](double d) {
            auto& dx = std::get<CBCTBeam>(*beam);
            dx.setDAPvalue(d);
        };
        auto getter = [=]() -> double {
            auto& dx = std::get<CBCTBeam>(*beam);
            return dx.DAPvalue();
        };

        addItem(root, "Dose area product [mGycm^2]", setter, getter);
    }

    {
        auto getter = [=]() -> std::uint64_t {
            auto& dx = std::get<CBCTBeam>(*beam);
            return dx.numberOfExposures();
        };
        addItem(root, "Number of exposures", getter);
    }
    {
        std::get<CBCTBeam>(*beam).setNumberOfParticlesPerExposure(1e6);

        auto setter = [=](std::uint64_t d) {
            auto& dx = std::get<CBCTBeam>(*beam);
            dx.setNumberOfParticlesPerExposure(d);
        };
        auto getter = [=]() -> std::uint64_t {
            auto& dx = std::get<CBCTBeam>(*beam);
            return dx.numberOfParticlesPerExposure();
        };
        addItem(root, "Particles per exposure", setter, getter);
    }
    {
        auto getter = [=]() -> std::uint64_t {
            auto& dx = std::get<CBCTBeam>(*beam);
            return dx.numberOfParticles();
        };
        addItem(root, "Total number of particles", getter);
    }

    beamActor->update();
    emit beamActorAdded(beamActor);
    appendRow(root);
}

void BeamSettingsModel::addCTSequentialBeam(std::shared_ptr<BeamActorContainer> actor)
{
    auto root = new LabelItem(tr("CT Sequential Beam"));

    std::shared_ptr<Beam> beam = nullptr;
    if (actor) {
        beam = actor->getBeam();
        if (beam)
            if (!std::holds_alternative<CTSequentialBeam>(*beam))
                beam = nullptr;
    }

    if (!beam) {
        if (m_image) {
            const auto& s = m_image->spacing();
            const auto& d = m_image->dimensions();
            const std::array<double, 3> start_init = { 0, 0, 0 };
            const std::array<double, 3> normal = { 0, 0, 1 };
            const std::map<std::size_t, double> filt_init = { { 13, 9.0 } };
            beam = std::make_shared<Beam>(CTSequentialBeam(start_init, normal, filt_init));
        } else {
            constexpr std::array<double, 3> start_init = { 0, 0, 0 };
            constexpr std::array<double, 3> normal = { 0, 0, 1 };
            const std::map<std::size_t, double> filt_init = { { 13, 9.0 } };
            beam = std::make_shared<Beam>(CTSequentialBeam(start_init, normal, filt_init));
        }
    }
    auto beamActor = std::make_shared<BeamActorContainer>(beam);
    m_beams.push_back(std::make_pair(beam, beamActor));

    {
        auto setter = [=](std::array<double, 3> d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setPosition(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.position();
        };
        addItem(root, "Start position [cm]", setter, getter);
    }
    {
        auto setter = [=](std::array<double, 3> d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setScanNormal(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::array<double, 3> {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.scanNormal();
        };
        addItem(root, "Direction vector", setter, getter);
    }
    {
        auto setter = [=](std::uint64_t d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setNumberOfSlices(d);
            beamActor->update();
        };
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.numberOfSlices();
        };
        addItem(root, "Number of slices", setter, getter);
    }

    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setSliceSpacing(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.sliceSpacing();
        };

        addItem(root, "Slice spacing [cm]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setScanFieldOfView(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.scanFieldOfView();
        };
        addItem(root, "Scan FOV [cm]", setter, getter);
    }
    {
        std::get<CTSequentialBeam>(*beam).setSourceDetectorDistance(119);

        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setSourceDetectorDistance(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.sourceDetectorDistance();
        };
        addItem(root, "Source detector distance [cm]", setter, getter);
    }
    {
        std::get<CTSequentialBeam>(*beam).setCollimation(3.84);
        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setCollimation(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.collimation();
        };
        addItem(root, "Total collimation [cm]", setter, getter);
    }

    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setStartAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.startAngleDeg();
        };
        addItem(root, "Set start angle [deg]", setter, getter);
    }
    {
        std::get<CTSequentialBeam>(*beam).setStepAngleDeg(5);
        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setStepAngleDeg(d);
            beamActor->update();
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.stepAngleDeg();
        };
        addItem(root, "Set angle step [deg]", setter, getter);
    }

    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setCTDIw(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.CTDIw();
        };
        addItem(root, "CTDIw [mGy]", setter, getter);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setCTDIdiameter(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.CTDIdiameter();
        };
        addItem(root, "CTDI phantom diameter [cm]", setter, getter);
    }

    auto tubeItem = new LabelItem(tr("Tube"));
    root->appendRow(tubeItem);
    {

        std::shared_ptr<QString> bowtiekey = nullptr;
        QString searchKey("Siemens Definition Flash W1 120kV");
        if (m_bowtieFilters.contains(searchKey)) {
            bowtiekey = std::make_shared<QString>(searchKey);
        } else {
            bowtiekey = std::make_shared<QString>(m_bowtieFilters.firstKey());
        }

        auto& ct = std::get<CTSequentialBeam>(*beam);
        ct.setBowtieFilter(m_bowtieFilters.value(*bowtiekey));
        auto setter = [=](const BeamSettingsModel::BowtieSelection& d) {
            if (d.bowtieMap->contains(d.currentKey)) {
                *bowtiekey = d.currentKey;
                auto& ct = std::get<CTSequentialBeam>(*beam);
                ct.setBowtieFilter(d.bowtieMap->value(d.currentKey));
            }
        };
        auto getter = [=, this]() -> BeamSettingsModel::BowtieSelection {
            BeamSettingsModel::BowtieSelection d { .bowtieMap = &(this->m_bowtieFilters) };
            d.currentKey = *bowtiekey;
            return d;
        };
        addItem(tubeItem, "Bowtie filter", setter, getter);
    }
    addTubeItems<CTSequentialBeam>(tubeItem, beam);

    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.numberOfExposures();
        };
        addItem(root, "Number of exposures", getter);
    }
    {
        std::get<CTSequentialBeam>(*beam).setNumberOfParticlesPerExposure(1e6);
        auto setter = [=](std::uint64_t d) {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            ct.setNumberOfParticlesPerExposure(d);
        };
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.numberOfParticlesPerExposure();
        };
        addItem(root, "Particles per exposure", setter, getter);
    }
    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSequentialBeam>(*beam);
            return ct.numberOfParticles();
        };
        addItem(root, "Total number of particles", getter);
    }

    beamActor->update();
    emit beamActorAdded(beamActor);
    appendRow(root);
}

void BeamSettingsModel::addCTSpiralBeam(std::shared_ptr<BeamActorContainer> actor)
{
    auto root = new LabelItem(tr("CT Spiral Beam"));

    std::shared_ptr<Beam> beam = nullptr;
    if (actor) {
        beam = actor->getBeam();
        if (beam)
            if (!std::holds_alternative<CTSpiralBeam>(*beam))
                beam = nullptr;
    }

    if (!beam) {
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
        std::get<CTSpiralBeam>(*beam).setSourceDetectorDistance(119);

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
        std::get<CTSpiralBeam>(*beam).setCollimation(3.84);
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
        std::get<CTSpiralBeam>(*beam).setStepAngleDeg(5);
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
        addItem(root, "CTDIvol [mGy]", setter, getter);
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

    {
        auto setter = [=, this](bool d) {
            if (this->m_image) {
                auto& ct = std::get<CTSpiralBeam>(*beam);
                if (d) {
                    const auto& ct_aec = this->m_image->aecData();
                    if (ct_aec.isEmpty())
                        ct.setAECFilter(this->m_image->calculateAECfilterFromWaterEquivalentDiameter());
                    else
                        ct.setAECFilter(ct_aec);
                } else {
                    ct.setAECFilter(CTAECFilter {});
                }
            }
        };
        auto getter = [=]() -> bool {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            const auto& aec = ct.AECFilter();
            return !aec.isEmpty();
        };
        addItem(root, "Use current AEC profile", setter, getter);
    }

    auto tubeItem = new LabelItem(tr("Tube"));
    root->appendRow(tubeItem);

    {
        std::shared_ptr<QString> bowtiekey = nullptr;
        QString searchKey("Siemens Definition Flash W1 120kV");
        if (m_bowtieFilters.contains(searchKey)) {
            bowtiekey = std::make_shared<QString>(searchKey);
        } else {
            bowtiekey = std::make_shared<QString>(m_bowtieFilters.firstKey());
        }

        auto& ct = std::get<CTSpiralBeam>(*beam);
        ct.setBowtieFilter(m_bowtieFilters.value(*bowtiekey));
        auto setter = [=](const BeamSettingsModel::BowtieSelection& d) {
            if (d.bowtieMap->contains(d.currentKey)) {
                *bowtiekey = d.currentKey;
                auto& ct = std::get<CTSpiralBeam>(*beam);
                ct.setBowtieFilter(d.bowtieMap->value(d.currentKey));
            }
        };
        auto getter = [=, this]() -> BeamSettingsModel::BowtieSelection {
            BeamSettingsModel::BowtieSelection d { .bowtieMap = &(this->m_bowtieFilters) };
            d.currentKey = *bowtiekey;
            return d;
        };
        addItem(tubeItem, "Bowtie filter", setter, getter);
    }
    addTubeItems<CTSpiralBeam>(tubeItem, beam);

    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralBeam>(*beam);
            return ct.numberOfExposures();
        };
        addItem(root, "Number of exposures", getter);
    }
    {
        std::get<CTSpiralBeam>(*beam).setNumberOfParticlesPerExposure(1e6);
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

void BeamSettingsModel::addCTSpiralDualEnergyBeam(std::shared_ptr<BeamActorContainer> actor)
{
    auto root = new LabelItem(tr("CT Spiral Dual Energy Beam"));

    std::shared_ptr<Beam> beam = nullptr;
    if (actor) {
        beam = actor->getBeam();
        if (beam)
            if (!std::holds_alternative<CTSpiralBeam>(*beam))
                beam = nullptr;
    }

    if (!beam) {
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
            return ct.scanFieldOfViewA();
        };
        addItem(root, "Scan FOV Tube A [cm]", setter, getter);
        auto setterB = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setScanFieldOfViewB(d);
            beamActor->update();
        };
        auto getterB = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.scanFieldOfViewB();
        };
        addItem(root, "Scan FOV Tube B [cm]", setterB, getterB);
    }
    {
        std::get<CTSpiralDualEnergyBeam>(*beam).setSourceDetectorDistance(119);
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
        std::get<CTSpiralDualEnergyBeam>(*beam).setCollimation(3.84);
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
        std::get<CTSpiralDualEnergyBeam>(*beam).setStepAngleDeg(5);
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
        addItem(root, "CTDIvol [mGy]", setter, getter);
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
        std::shared_ptr<QString> bowtiekey = nullptr;
        QString searchKey("Siemens Definition Flash W1 120kV");
        if (m_bowtieFilters.contains(searchKey)) {
            bowtiekey = std::make_shared<QString>(searchKey);
        } else {
            bowtiekey = std::make_shared<QString>(m_bowtieFilters.firstKey());
        }
        auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
        ct.setBowtieFilterA(m_bowtieFilters.value(*bowtiekey));
        auto setter = [=](const BeamSettingsModel::BowtieSelection& d) {
            if (d.bowtieMap->contains(d.currentKey)) {
                *bowtiekey = d.currentKey;
                auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
                ct.setBowtieFilterA(d.bowtieMap->value(d.currentKey));
            }
        };
        auto getter = [=, this]() -> BeamSettingsModel::BowtieSelection {
            BeamSettingsModel::BowtieSelection d { .bowtieMap = &(this->m_bowtieFilters) };
            d.currentKey = *bowtiekey;
            return d;
        };
        addItem(tubeAItem, "Bowtie filter", setter, getter);
    }
    {
        std::shared_ptr<QString> bowtiekey = nullptr;
        QString searchKey("Siemens Definition Flash W1 120kV");
        if (m_bowtieFilters.contains(searchKey)) {
            bowtiekey = std::make_shared<QString>(searchKey);
        } else {
            bowtiekey = std::make_shared<QString>(m_bowtieFilters.firstKey());
        }
        auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
        ct.setBowtieFilterB(m_bowtieFilters.value(*bowtiekey));
        auto setter = [=](const BeamSettingsModel::BowtieSelection& d) {
            if (d.bowtieMap->contains(d.currentKey)) {
                *bowtiekey = d.currentKey;
                auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
                ct.setBowtieFilterB(d.bowtieMap->value(d.currentKey));
            }
        };
        auto getter = [=, this]() -> BeamSettingsModel::BowtieSelection {
            BeamSettingsModel::BowtieSelection d { .bowtieMap = &(this->m_bowtieFilters) };
            d.currentKey = *bowtiekey;
            return d;
        };
        addItem(tubeBItem, "Bowtie filter", setter, getter);
    }
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
        addItem(tubeAItem, "Tube potential [kV]", setter, getter);
        addItem(tubeBItem, "Tube potential [kV]", setterB, getterB);
    }
    {
        auto setter = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setRelativeMasTubeA(d);
        };
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.relativeMasTubeA();
        };
        addItem(tubeAItem, "Relative tube current", setter, getter);

        auto setterB = [=](double d) {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            ct.setRelativeMasTubeB(d);
        };
        auto getterB = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.relativeMasTubeB();
        };
        addItem(tubeBItem, "Relative tube current", setterB, getterB);
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
        addItem(tubeAItem, "Tube Al filtration [mm]", setter, getter);
        addItem(tubeBItem, "Tube Al filtration [mm]", setterB, getterB);
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
        addItem(tubeAItem, "Tube Cu filtration [mm]", setter, getter);
        addItem(tubeBItem, "Tube Cu filtration [mm]", setterB, getterB);
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
        addItem(tubeAItem, "Tube Sn filtration [mm]", setter, getter);
        addItem(tubeBItem, "Tube Sn filtration [mm]", setterB, getterB);
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
        auto getter = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.tubeRelativeWeightA();
        };
        addItem(tubeAItem, "Relative photon count weight", getter);

        auto getterB = [=]() -> double {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.tubeRelativeWeightB();
        };
        addItem(tubeBItem, "Relative photon count weight", getterB);
    }

    {
        auto setter = [=, this](bool d) {
            if (m_image) {
                auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
                if (d) {
                    ct.setAECFilter(this->m_image->aecData());
                } else {
                    ct.setAECFilter(CTAECFilter {});
                }
            }
        };
        auto getter = [=]() -> bool {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            const auto& aec = ct.AECFilter();
            return aec.weights().size() > 2;
        };
        addItem(root, "Use current AEC profile", setter, getter);
    }

    {
        auto getter = [=]() -> std::uint64_t {
            auto& ct = std::get<CTSpiralDualEnergyBeam>(*beam);
            return ct.numberOfExposures();
        };
        addItem(root, "Number of exposures", getter);
    }
    {
        std::get<CTSpiralDualEnergyBeam>(*beam).setNumberOfParticlesPerExposure(1e6);
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
