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

#include "transport.h"
#include "xraylib.h"
#include "world.h"
#include "exposure.h"
#include "dxmcrandom.h"
#include "vectormath.h"
#include "attenuationlut.h"

#include <algorithm>
#include <mutex>
#include <atomic>
#include <execution>
#include <future>
#include <thread>
#include <memory>



namespace transport {

	constexpr double ELECTRON_REST_MASS = 510.9989461; // keV
	constexpr double PI_VAL = 3.14159265358979323846264338327950288;
	constexpr double PI_VAL2 = PI_VAL + PI_VAL;

	constexpr double ENERGY_CUTOFF = 1.0; // 10 ev

	constexpr double RUSSIAN_RULETTE_PROBABILITY = 0.8;
	constexpr double RUSSIAN_RULETTE_THRESHOLD = 10.0; // keV


	std::mutex TRANSPORT_MUTEX;

	constexpr double N_ERROR = 1.0e-9;


	template<typename T>
	void findNearestIndices(const T value, const std::vector<T>& vec, std::size_t& first, std::size_t& last)
	{

		auto beg = vec.begin();
		auto end = vec.end();
		auto upper = std::lower_bound(beg, end, value);

		if (upper == end)
			last = vec.size() - 1;
		else if (upper == beg)
			last = 2;
		else
			last = static_cast<std::size_t>(std::distance(beg, upper));
		first = last - 1;
		return;
	}

	template <typename T>
	inline T interp(T x, T x0, T x1, T y0, T y1)
	{
		return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
	}



	template <typename T>
	T interpolate(const std::vector<T>& xArr, const std::vector<T>& yArr, const T x)
	{
		auto first = xArr.begin();
		auto last = xArr.end();
		auto upper = std::lower_bound(first, last, x);

		if (upper == xArr.cend())
			return yArr.back();

		if (upper == xArr.cbegin())
			return yArr.front();

		const double x1 = *upper;

		auto yupper = yArr.begin();
		auto dist = std::distance(xArr.begin(), upper);
		std::advance(yupper, dist);

		const double y1 = *yupper;

		std::advance(yupper, -1);
		const double y0 = *yupper;

		std::advance(upper, -1);
		const double x0 = *upper;

		return interp(x, x0, x1, y0, y1);
	}



	inline void safeValueAdd(double& value, const double addValue)
	{
		std::lock_guard<std::mutex> lock(TRANSPORT_MUTEX);
		value += addValue;
	}


	/*void rayleightScatterPenelope(Particle& particle, const AttenuationLutElement& lutElement, std::uint64_t seed[2], double& cosAngle)
	{
		const double qmax = 2.0 * particle.energy / ELECTRON_REST_MASS;
		const double qmaxsqr = qmax;// qmax *qmax;
		//const double qmaxsqr = qmax *qmax;

		const double AqmaxSquared = interpolate(lutElement.rayleighMomentumTransferSquared,
			lutElement.rayleighCumFormFactorSquared,
			qmaxsqr);

		double costheta;

		bool rejected;
		do {
			const double r1 = randomUniform<double>(seed);
			const double A = r1 * AqmaxSquared;

			const double qsqr = interpolate(lutElement.rayleighCumFormFactorSquared, lutElement.rayleighMomentumTransferSquared, A);

			costheta = 1.0 - 2.0*qsqr / qmaxsqr;

			const double g = 0.5 * (1.0 + costheta * costheta);
			const double r2 = randomUniform<double>(seed);
			rejected = r2 < g;
		} while (rejected);

		cosAngle = costheta;
		const double theta = std::acos(costheta);
		const double phi = randomUniform<double>(seed, 0.0, PI_VAL2);
		vectormath::peturb<double>(particle.dir, theta, phi);
	}*/

	/*void rayleightScatter(Particle& particle, const AttenuationLutElement& lutElement, std::uint64_t seed[2], double& cosAngle)
	{
		// theta is scattering angle
		// see http://rcwww.kek.jp/research/egs/egs5_manual/slac730-150228.pdf


		//finding qmax
		const double k = particle.energy / ELECTRON_REST_MASS;
		const double qmaxSquared = 2.0*k;//4.0 * k * k;
		//const double qmaxSquared = 4.0 * k * k;


		const double AqmaxSquared = interpolate(lutElement.rayleighMomentumTransferSquared,
			lutElement.rayleighCumFormFactorSquared,
			qmaxSquared);

		double mu;
		bool rejected;
		do {
			const double r1 = randomUniform<double>(seed);
			const double AqSquared = AqmaxSquared * r1;

			const double qSquared = interpolate(lutElement.rayleighCumFormFactorSquared,
				lutElement.rayleighMomentumTransferSquared,
				AqSquared);

			mu = 1.0 - qSquared * 2.0 / qmaxSquared;

			const double r2 = randomUniform<double>(seed);

			rejected = r2 < (1.0 + mu * mu) / 2.0;
		} while (rejected);

		cosAngle = mu;
		const double theta = std::acos(mu);
		const double phi = randomUniform<double>(seed, 0.0, PI_VAL2);
		vectormath::peturb<double>(particle.dir, theta, phi);
	}*/

	void rayleightScatter(Particle& particle, unsigned char materialIdx, const AttenuationLut& attLut, std::uint64_t seed[2], double& cosAngle)
	{
		// theta is scattering angle
		// see http://rcwww.kek.jp/research/egs/egs5_manual/slac730-150228.pdf


		//finding qmax
		
		const double qmax = attLut.momentumTransferMax(particle.energy);
		const double qmaxSqr = qmax * qmax;
		const double AqmaxSquared = attLut.cumFormFactorSquared(materialIdx, qmaxSqr);


		double mu;
		bool rejected;
		do {
			const double r1 = randomUniform<double>(seed);
			const double AqSquared = AqmaxSquared * r1;

			const double qSquared = attLut.momentumTransferSquared(materialIdx, AqSquared);

			mu = 1.0 - qSquared / (2.0 * qmaxSqr);

			const double r2 = randomUniform<double>(seed);

			rejected = r2 < (1.0 + mu * mu) / 2.0;
		} while (rejected);

		cosAngle = mu;
		const double theta = std::acos(mu);
		const double phi = randomUniform<double>(seed, 0.0, PI_VAL2);
		vectormath::peturb<double>(particle.dir, theta, phi);
	}


double comptonScatterEGS(Particle& particle, std::uint64_t seed[2], double& cosAngle)
		// see http://geant4-userdoc.web.cern.ch/geant4-userdoc/UsersGuides/PhysicsReferenceManual/fo/PhysicsReferenceManual.pdf
		// and
		// https://nrc-cnrc.github.io/EGSnrc/doc/pirs701-egsnrc.pdf
	{
		const double k = particle.energy / ELECTRON_REST_MASS;
		const double emin = 1.0 / (1.0 + 2.0 * k);
		const double gmax = 1.0 / emin + emin;
		double e;
		for (;;)
		{
			const double r1 = randomUniform<double>(seed);
			const double r2 = randomUniform<double>(seed);

			e = r1 + (1.0 - r1) * emin;
			cosAngle = 1.0 + 1.0 / k - 1.0 / (e*k);
			const double sinangsqr = 1.0 - cosAngle * cosAngle;
			const double g = (1.0 / e + e - sinangsqr) / (1.0 / emin + emin);
			if (r2 <= g)
				break;
		}
		
		const double theta = std::acos(cosAngle);
		const double phi = randomUniform<double>(seed, PI_VAL2);
		vectormath::peturb<double>(particle.dir, theta, phi);

		const double E0 = particle.energy;
		particle.energy *= e;

		return E0 * (1.0 - e);
	}


	double comptonScatter(Particle& particle, std::uint64_t seed[2], double& cosAngle)
		// see http://geant4-userdoc.web.cern.ch/geant4-userdoc/UsersGuides/PhysicsReferenceManual/fo/PhysicsReferenceManual.pdf
		// and
		// https://nrc-cnrc.github.io/EGSnrc/doc/pirs701-egsnrc.pdf
	{
		const double k = particle.energy / ELECTRON_REST_MASS;
		const double emin = 1.0 / (1.0 + 2.0 * k);
		const double gmax = 1.0 / emin + emin;

		bool rejected;
		double sinthetasqr, e, t;
		do {
			const double r1 = randomUniform<double>(seed);
			e = r1 + (1.0 - r1) * emin;

			t = (1.0 - e) / (k * e);
			sinthetasqr = t * (2.0 - t);

			const double g = (1.0 / e + e - sinthetasqr) / gmax;

			const double r2 = randomUniform<double>(seed);
			rejected = r2 > g;
		} while (rejected);

		cosAngle = 1.0 - t;
		const double theta = std::acos(cosAngle);
		const double phi = randomUniform<double>(seed, PI_VAL2);
		vectormath::peturb<double>(particle.dir, theta, phi);

		const double E0 = particle.energy;
		particle.energy *= e;

		return E0 * (1.0 - e);
	}

	double comptonScatterGeant(Particle& particle, std::uint64_t seed[2], double& cosAngle)
		// see http://geant4-userdoc.web.cern.ch/geant4-userdoc/UsersGuides/PhysicsReferenceManual/fo/PhysicsReferenceManual.pdf
		// and
		// https://nrc-cnrc.github.io/EGSnrc/doc/pirs701-egsnrc.pdf
	{

		const double e0 = ELECTRON_REST_MASS / (ELECTRON_REST_MASS + 2.0 * particle.energy);

		const double a1 = std::log(1.0 / e0);
		const double a2 = (1.0 - e0 * e0) / 2.0;

		bool rejected;
		const double af = a1 / (a1 + a2);
		double t, e;

		do {
			const double r1 = randomUniform<double>(seed);
			const double r2 = randomUniform<double>(seed);
			const double r3 = randomUniform<double>(seed);
			if (r1 < af)
				e = std::exp(-r2 * a1);
			else
				e = std::sqrt(e0 * e0 + (1.0 - e0 * e0) * r2);

			t = ELECTRON_REST_MASS * (1.0 - e) / (particle.energy * e);
			const double sinthetasqr = t * (2.0 - t);

			const double g = 1.0 - e * sinthetasqr / (1.0 + e * e);

			rejected = g < r3;
		} while (rejected);

		const double costheta = 1.0 - t;

		const double theta = std::acos(costheta);
		const double phi = randomUniform<double>(seed, 0.0, PI_VAL2);
		vectormath::peturb<double>(particle.dir, theta, phi);

		const double E0 = particle.energy;
		particle.energy *= e;
		cosAngle = costheta;
		return E0 * (1.0 - e);
	}


	inline bool particleInsideWorld(const World& world, const Particle& particle)
	{
		const std::array<double, 6>& extent = world.matrixExtent();
		return (particle.pos[0] > extent[0] && particle.pos[0] < extent[1]) &&
			(particle.pos[1] > extent[2] && particle.pos[1] < extent[3]) &&
			(particle.pos[2] > extent[4] && particle.pos[2] < extent[5]);
	}

	inline std::size_t indexFromPosition(const double pos[3], const World& world)
	{
		//assumes particle is inside world
		std::size_t arraypos[3];
		const std::array<double, 6>& wpos = world.matrixExtent();
		const std::array<std::size_t, 3>& wdim = world.dimensions();
		const std::array<double, 3>& wspac = world.spacing();

		for (std::size_t i = 0; i < 3; i++)
			arraypos[i] = static_cast<std::size_t>((pos[i] - wpos[i * 2]) / wspac[i]);
		std::size_t idx = arraypos[2] * wdim[0] * wdim[1] + arraypos[1] * wdim[0] + arraypos[0];
		return idx;
	}


	void sampleParticleSteps(const World& world, Particle& p, std::uint64_t seed[2], double* energyImparted)
	{
		const auto& lutTable = world.attenuationLut();
		const double* densityBuffer = world.densityBuffer();
		const auto materialBuffer = world.materialIndexBuffer();
		
		double maxAttenuation;
		bool updateMaxAttenuation = true;
		bool continueSampling = true;
		bool ruletteCandidate = true;
		while (continueSampling)
		{
			if (updateMaxAttenuation)
			{
				maxAttenuation = lutTable.maxMassTotalAttenuation(p.energy);
				updateMaxAttenuation = false;
			}
			const double r1 = randomUniform<double>(seed);
			const double stepLenght = -std::log(r1) / maxAttenuation * 10.0; // cm -> mm
			for (std::size_t i = 0; i < 3; i++)
				p.pos[i] += p.dir[i] * stepLenght;

			if (particleInsideWorld(world, p))
			{
				const std::size_t bufferIdx = indexFromPosition(p.pos, world);
				const auto matIdx = materialBuffer[bufferIdx];
				
				
				const double attenuationTotal = lutTable.totalAttenuation(matIdx, p.energy) * densityBuffer[bufferIdx];

				const double r2 = randomUniform<double>(seed);
				if (r2 < (attenuationTotal / maxAttenuation)) // An event will happend
				{
					auto atts = lutTable.photoComptRayAttenuation(matIdx, p.energy);
					const double attPhoto = atts[0];
					const double attCompt = atts[1];
					const double attRayl = atts[2];

					const double r3 = randomUniform(seed, attPhoto + attCompt + attRayl);
					if (r3 <= attPhoto) // Photoelectric event
					{
						safeValueAdd(energyImparted[bufferIdx], p.energy * p.weight);
						p.energy = 0.0;
						continueSampling = false;
					}
					else if (r3 <= attPhoto + attCompt) // Compton event
					{
						double cosangle;
						const double e = comptonScatter(p, seed, cosangle);
						safeValueAdd(energyImparted[bufferIdx], e * p.weight);
						updateMaxAttenuation = true;
						if (p.energy < ENERGY_CUTOFF)
						{
							safeValueAdd(energyImparted[bufferIdx], p.energy * p.weight);
							p.energy = 0.0;
							continueSampling = false;
						}
					}
					else // Rayleigh scatter event
					{
						double cosangle;
						rayleightScatter(p, matIdx, world.attenuationLut(), seed, cosangle);
					}


					// russian rulette
					if ((p.energy < RUSSIAN_RULETTE_THRESHOLD) && ruletteCandidate)
					{
						ruletteCandidate = false;
						double r4 = randomUniform<double>(seed);
						if (r4 < RUSSIAN_RULETTE_PROBABILITY)
						{
							continueSampling = false;
						}
						else {
							constexpr double factor = 1.0 / (1.0 - RUSSIAN_RULETTE_PROBABILITY);
							p.weight *= factor;
						}
					}


				}

			}
			else // particle not inside world
			{
				continueSampling = false;
			}

		}
	}

	bool transportParticleToWorld(const World& world, Particle& particle)
		//returns false if particle do not intersect world
	{
		bool isInside = particleInsideWorld(world, particle);
		if (isInside)
			return true;

		double amin = std::numeric_limits<double>::min();
		double amax = std::numeric_limits<double>::max();

		const std::array<double, 6>& extent = world.matrixExtent();
		for (std::size_t i = 0; i < 3; i++)
		{
			if (std::abs(particle.dir[i]) > N_ERROR)
			{
				double a0, an;
				a0 = (extent[i * 2] - particle.pos[i]) / particle.dir[i];
				an = (extent[i * 2 + 1] - particle.pos[i]) / particle.dir[i];
				amin = std::max(amin, std::min(a0, an));
				amax = std::min(amax, std::max(a0, an));
			}

		}
		if (amin < amax && amin > 0.0)
		{
			for (std::size_t i = 0; i < 3; i++)
			{
				particle.pos[i] += (amin + N_ERROR) * particle.dir[i]; // making sure particle is inside world
			}
			return true;
		}
		return false;
	}



	void transport(const World& world, const Exposure& exposure, std::uint64_t seed[2], double* energyImparted)
	{
		Particle particle;
		const std::size_t nHistories = exposure.numberOfHistories();
		for (std::size_t i = 0; i < nHistories; ++i)
		{
			//Draw a particle
			exposure.sampleParticle(particle, seed);

			// Is particle intersecting with world
			bool isInWorld = transportParticleToWorld(world, particle);
			if (isInWorld)
				sampleParticleSteps(world, particle, seed, energyImparted);
		}
	}
	
	std::uint64_t parallell_run(const World& w, const Source* source, double* energyImparted,
		const std::uint64_t expBeg, const std::uint64_t expEnd, std::uint64_t nJobs)
	{
		std::uint64_t len = expEnd - expBeg;
		if ((len <= 1) || (nJobs <= 1))
		{
			std::uint64_t seed[2];
			randomSeed(seed);  // get random seed from OS
			Exposure exposure = Exposure();
			const auto& worldBasis = w.directionCosines();
			for (std::size_t i = expBeg; i < expEnd; i++)
			{
				source->getExposure(exposure, i);
				exposure.alignToDirectionCosines(worldBasis);
				transport(w, exposure, seed, energyImparted);
			}
			return source->historiesPerExposure() * (expEnd - expBeg);
		}
		auto mid = expBeg + len / 2;
		auto handle = std::async(std::launch::async, parallell_run, w, source, energyImparted, mid, expEnd, nJobs - 2);
		std::uint64_t nHistories = parallell_run(w, source, energyImparted, expBeg, mid, nJobs - 2);
		return handle.get() + nHistories;
	}

	std::uint64_t parallell_run_ctdi(const CTDIPhantom& w, const CTSource* source, double* energyImparted,
		const std::uint64_t expBeg, const std::uint64_t expEnd, std::uint64_t nJobs)
	{
		std::uint64_t len = expEnd - expBeg;

		if ((len == 1 ) || (nJobs <= 1))
		{
			std::uint64_t seed[2];
			randomSeed(seed);  // get random seed from OS
			Exposure exposure = Exposure();
			const auto& worldBasis = w.directionCosines();
			for (std::size_t i = expBeg; i < expEnd; i++)
			{
				source->getExposure(exposure, i);
				exposure.alignToDirectionCosines(worldBasis);
				exposure.setPositionZ(0.0);
				exposure.setBeamIntensityWeight(1.0);
				transport(w, exposure, seed, energyImparted);
			}
			return source->historiesPerExposure() * (expEnd - expBeg);
		}
		auto mid = expBeg + len / 2;
		auto handle = std::async(std::launch::async, parallell_run_ctdi, w, source, energyImparted, mid, expEnd, nJobs - 2);
		std::uint64_t nHistories = parallell_run_ctdi(w, source, energyImparted, expBeg, mid, nJobs - 2);
		return handle.get() + nHistories;
		
	}

	void energyImpartedToDose(const World & world, const Source* source, std::vector<double>& energyImparted, const double calibrationValue)
	{
		constexpr double KEV_TO_MJ = 1.6021773e-13;
		auto spacing = world.spacing();
		const double voxelVolume = spacing[0] * spacing[1] * spacing[2] / 1000.0; // cm3
		auto density = world.densityArray();
		
		std::transform(std::execution::par_unseq, energyImparted.begin(), energyImparted.end(), density->begin(), energyImparted.begin(), 
			[=](double ei, double de)->double {

			const double voxelMass = de * voxelVolume * 0.001; //kg
			return de > 0.0 ? calibrationValue * KEV_TO_MJ * ei / voxelMass : 0.0;
		});
	}
	

	std::vector<double> run(const World & world, Source* source)
	{
		std::vector<double> dose(world.size(), 0.0);
		if (!source)
			return dose;
	
		if (!world.isValid())
			return dose;

		source->updateFromWorld(world);
		source->validate();
		if(!source->isValid())
			return dose;

		const std::uint64_t totalExposures = source->totalExposures();
		
		std::uint64_t nThreads = std::thread::hardware_concurrency();
		std::uint64_t nJobs = nThreads;//*2;
		if (nJobs < 1)
			nJobs = 1;

		auto nHistories = parallell_run(world, source, dose.data(), 0, totalExposures, nJobs);
		
		double calibrationValue = source->getCalibrationValue(nHistories);
		//energy imparted to dose
		energyImpartedToDose(world, source, dose, calibrationValue);
		return dose;
	}


	std::vector<double> run(const CTDIPhantom & world, CTSource* source)
	{
		std::vector<double> dose(world.size(), 0.0);
		if (!source)
			return dose;

		if (!world.isValid())
			return dose;

		
		if (!source->isValid())
			return dose;

		const std::uint64_t totalExposures = source->exposuresPerRotatition();
		
		std::uint64_t nThreads = std::thread::hardware_concurrency();
		std::uint64_t nJobs = nThreads;
		if (nJobs < 1)
			nJobs = 1;
		parallell_run_ctdi(world, source, dose.data(), 0, totalExposures, nJobs);

		energyImpartedToDose(world, source, dose, 1.0);
		return dose;
	}
	
}