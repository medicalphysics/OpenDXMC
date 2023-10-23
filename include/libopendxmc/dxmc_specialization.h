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
#include "dxmc.hpp"
#include "opendxmc/precision_specialization.h"

//Here we specialize types from the dxmc template library.

using Material = dxmc::Material;
using Tube = dxmc::Tube<floating>;
using Transport = dxmc::Transport<floating>;
using AECFilter = dxmc::AECFilter<floating>;
using BowTieFilter = dxmc::BowTieFilter<floating>;
using Source = dxmc::Source<floating>;
using DAPSource = dxmc::DAPSource<floating>;
using DXSource = dxmc::DXSource<floating>;
using CTBaseSource = dxmc::CTBaseSource<floating>;
using CTSource = dxmc::CTSource<floating>;
using CTAxialSource = dxmc::CTAxialSource<floating>;
using CTTopogramSource = dxmc::CTTopogramSource<floating>;
using CTSpiralSource = dxmc::CTSpiralSource<floating>;
using CTSpiralDualSource = dxmc::CTSpiralDualSource<floating>;
using CBCTSource =dxmc::CBCTSource<floating>;
using World = dxmc::World<floating>;
using CTDIPhantom = dxmc::CTDIPhantom<floating>;
using ProgressBar = dxmc::ProgressBar<floating>;
using AttenuationLut = dxmc::AttenuationLut<floating>;
using DoseProgressImageData = dxmc::DoseProgressImageData<floating>;
