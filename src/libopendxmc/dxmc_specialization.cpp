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

Copyright 2023 Erlend Andersen
*/

#include <dxmc_specialization.hpp>

DXBeam::DXBeam(const std::array<double, 3>& pos,
    const std::array<std::array<double, 3>, 2>& dircosines,
    const std::map<std::size_t, double>& filtrationMaterials)
    : dxmc::DXBeam<double>(pos, dircosines, filtrationMaterials)
{
    updatePosition();
}

const std::array<double, 3>& DXBeam::rotationCenter() const { return m_rotation_center; }
void DXBeam::setRotationCenter(const std::array<double, 3>& c)
{
    m_rotation_center = c;
    updatePosition();
}
double DXBeam::sourcePatientDistance() const { return m_SPD; }
void DXBeam::setSourcePatientDistance(double d)
{
    m_SPD = std::abs(d);
    updatePosition();
}
double DXBeam::sourceDetectorDistance() const { return m_SDD; }
void DXBeam::setSourceDetectorDistance(double d)
{
    m_SDD = std::abs(d);
    updatePosition();
}
void DXBeam::setCollimation(const std::array<double, 2>& coll)
{
    const std::array<double, 2> ang = {
        std::tan(0.5 * std::abs(coll[0]) / m_SDD), std::tan(0.5 * std::abs(coll[1]) / m_SDD)
    };
    setCollimationAngles(ang);
}
std::array<double, 2> DXBeam::collimation() const
{
    const auto& ang = collimationAngles();
    std::array<double, 2> coll = {
        2 * m_SDD * std::atan(ang[0]), 2 * m_SDD * std::atan(ang[1])
    };
    return coll;
}

double DXBeam::primaryAngleDeg() const { return dxmc::RAD_TO_DEG<double>() * m_rotAngles[0]; }
void DXBeam::setPrimaryAngleDeg(double ang)
{
    m_rotAngles[0] = dxmc::DEG_TO_RAD<double>() * std::clamp(ang, -180.0, 180.0);
    updatePosition();
}
double DXBeam::secondaryAngleDeg() const { return dxmc::RAD_TO_DEG<double>() * m_rotAngles[1]; }
void DXBeam::setSecondaryAngleDeg(double ang)
{
    m_rotAngles[1] = dxmc::DEG_TO_RAD<double>() * std::clamp(ang, -90.0, 90.0);
    updatePosition();
}

void DXBeam::updatePosition()
{
    std::array<std::array<double, 3>, 2> cosines = { { { 0, 0, 1 }, { 1, 0, 0 } } };
    cosines[0] = dxmc::vectormath::rotate(cosines[0], { 0, 0, -1 }, m_rotAngles[0]);
    cosines[1] = dxmc::vectormath::rotate(cosines[1], { 0, 0, -1 }, m_rotAngles[0]);
    cosines[0] = dxmc::vectormath::rotate(cosines[0], { 1, 0, 0 }, m_rotAngles[1]);
    cosines[1] = dxmc::vectormath::rotate(cosines[1], { 1, 0, 0 }, m_rotAngles[1]);
    auto dir = dxmc::vectormath::cross(cosines[0], cosines[1]);
    auto ddist = dxmc::vectormath::scale(dir, -m_SPD);

    setPosition(dxmc::vectormath::add(m_rotation_center, ddist));
    setDirectionCosines(cosines);
}
