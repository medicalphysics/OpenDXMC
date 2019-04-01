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