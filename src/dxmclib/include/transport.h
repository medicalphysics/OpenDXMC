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
#include "world.h"
#include "exposure.h"
#include "source.h"
#include "dxmcrandom.h"
#include "vectormath.h"

#include <memory>
#include <algorithm>
#include <mutex>

namespace transport {
	
	double comptonScatter(Particle& particle, std::uint64_t seed[2], double& cosAngle);
	double comptonScatterGeant(Particle& particle, std::uint64_t seed[2], double& cosAngle);
	double comptonScatterEGS(Particle& particle, std::uint64_t seed[2], double& cosAngle);
	void rayleightScatter(Particle& particle, unsigned char materialIdx, const AttenuationLut& attLut, std::uint64_t seed[2], double& cosAngle);
	std::vector<double> run(const World& world, Source* source);
	std::vector<double> run(const CTDIPhantom& world, CTSource* source);
}