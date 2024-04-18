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

#pragma once

#include "dxmc/beams/beamtype.hpp"
#include "dxmc/beams/cbctbeam.hpp"
#include "dxmc/beams/ctsequentialbeam.hpp"
#include "dxmc/beams/ctspiralbeam.hpp"
#include "dxmc/beams/ctspiraldualenergybeam.hpp"
#include "dxmc/beams/dxbeam.hpp"
#include "dxmc/beams/tube/tube.hpp"
#include "dxmc/material/material.hpp"
#include "dxmc/material/nistmaterials.hpp"

#include <memory>
#include <variant>

// Here we specialize types from the dxmc template library.

using Material = dxmc::Material<5>;
using Tube = dxmc::Tube;
using NISTMaterials = dxmc::NISTMaterials;

using CTSequentialBeam = dxmc::CTSequentialBeam<false>;
using CTSpiralBeam = dxmc::CTSpiralBeam<false>;
using CTSpiralDualEnergyBeam = dxmc::CTSpiralDualEnergyBeam<false>;
using CBCTBeam = dxmc::CBCTBeam<false>;
using CTAECFilter = dxmc::CTAECFilter;
using BowtieFilter = dxmc::BowtieFilter;

class DXBeam : public dxmc::DXBeam<false> {
public:
    DXBeam(const std::map<std::size_t, double>& filtrationMaterials = {});

    const std::array<double, 3>& rotationCenter() const;
    void setRotationCenter(const std::array<double, 3>& c);
    double sourcePatientDistance() const;
    void setSourcePatientDistance(double d);
    double sourceDetectorDistance() const;
    void setSourceDetectorDistance(double d);
    void setCollimation(const std::array<double, 2>& coll);
    std::array<double, 2> collimation() const;

    double primaryAngleDeg() const;
    void setPrimaryAngleDeg(double ang);
    double secondaryAngleDeg() const;
    void setSecondaryAngleDeg(double ang);

protected:
    void updatePosition();

private:
    std::array<double, 3> m_rotation_center = { 0, 0, 0 };
    double m_SPD = 100;
    double m_SDD = 100;
    std::array<double, 2> m_rotAngles = { 0, 0 };
};

using Beam = std::variant<DXBeam, CTSpiralBeam, CTSpiralDualEnergyBeam, CBCTBeam, CTSequentialBeam>;
