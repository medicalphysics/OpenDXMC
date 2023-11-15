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

#pragma once

#include "dxmc/material/material.hpp"
#include "dxmc/material/nistmaterials.hpp"
#include "dxmc/beams/tube/tube.hpp"
#include "dxmc/beams/dxbeam.hpp"
#include "dxmc/beams/ctspiralbeam.hpp"
#include "dxmc/beams/ctspiraldualenergybeam.hpp"

#include <variant>

//Here we specialize types from the dxmc template library.

using Material = dxmc::Material<double, 5>;
using Tube = dxmc::Tube<double>;
using NISTMaterials = dxmc::NISTMaterials<double>;

using CTSpiralBeam = dxmc::CTSpiralBeam<double>;
using CTSpiralDualEnergyBeam = dxmc::CTSpiralDualEnergyBeam<double>;
using DXBeam = dxmc::DXBeam<double>;

using Beam = std::variant<DXBeam, CTSpiralBeam, CTSpiralDualEnergyBeam>;

