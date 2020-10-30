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
#include "dxmc.h"

//Here we spesialize types from the dxmc template library.

using Material = dxmc::Material;
using Tube = dxmc::Tube<double>;
using Transport = dxmc::Transport<double>;
using AECFilter = dxmc::AECFilter<double>;
using BowTieFilter = dxmc::BowTieFilter<double>;
using Source = dxmc::Source<double>;
using DXSource = dxmc::DXSource<double>;
using CTSource = dxmc::CTSource<double>;
using CTAxialSource = dxmc::CTAxialSource<double>;
using CTSpiralSource = dxmc::CTSpiralSource<double>;
using CTSpiralDualSource = dxmc::CTSpiralDualSource<double>;
using World = dxmc::World<double>;
using CTDIPhantom = dxmc::CTDIPhantom<double>;
using ProgressBar = dxmc::ProgressBar<double>;
using AttenuationLut = dxmc::AttenuationLut<double>;
using DoseProgressImageData = dxmc::DoseProgressImageData<double>;
