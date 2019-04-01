
#include "world.h"
#include "material.h"
#include "transport.h"
#include "vectormath.h"
#include "exposure.h"
#include "dxmcrandom.h"
#include <xraylib.h>
#include "attenuationlut.h"
#include "source.h"

#include <iostream>
#include <memory>
#include <vector>
#include <array>
#include <map>

constexpr double PIVAL = 3.14159265359;
constexpr double RAD_TO_DEG = 180.0 / PIVAL;
constexpr double DEG_TO_RAD = 1.0 / RAD_TO_DEG;
//constexpr double MEC2 = 510.9989461;

inline std::size_t index(std::size_t x, std::size_t y, std::size_t z, const std::array<std::size_t, 3>& dim)
{
	return z * dim[0] * dim[1] + y * dim[0] + x;
}

std::pair<std::shared_ptr<std::vector<double>>, std::shared_ptr<std::vector<unsigned char>>>
waterSylinder(const std::array<std::size_t, 3>& dim, const std::array<double, 3>& spacing, const double radius = 200 )
{
	auto size = dim[0] * dim[1] * dim[2];
	auto dens = std::make_shared<std::vector<double>>(size, 0.0);
	auto mat = std::make_shared<std::vector<unsigned char>>(size, 0);

	const std::array<double, 2> center = { spacing[0] * dim[0] / 2.0, spacing[1] * dim[1] / 2.0 };
	
	const double r2 = radius * radius;

	for (std::size_t i = 0;i < dim[0];++i)
		for (std::size_t j = 0; j < dim[1]; ++j)
			for (std::size_t k = 0; k < dim[2]; ++k)
			{
				const auto idx = index(i, j, k, dim);
				const double posx = i * spacing[0] - center[0];
				const double posy = j * spacing[1] - center[1];
				if ((posx*posx + posy * posy) < r2)
				{
					(dens->data())[idx] = 1.0;
					(mat->data())[idx] = 1;
				}
			}
	return std::make_pair(dens, mat);
}


void validateDose()
{
	const double energy = 60.0; //kev
	const std::uint64_t nHistories = 100000000;
	
	Material air("Air, Dry (near sea level)");
	Material water("Water, Liquid");
	
	World w;

	w.setAttenuationLutMaxEnergy(energy);
	w.addMaterialToMap(air);
	w.addMaterialToMap(water);
	//std::array<std::size_t, 3> dim = {1024,1024, 100 };
	std::array<std::size_t, 3> dim = { 401,401, 100 };
	std::array<double, 3> spacing = { 1, 1,1 };
	w.setDimensions(dim);
	w.setSpacing(spacing);

	auto[dens, mat] = waterSylinder(dim, spacing, 200.0);

	w.setDensityArray(dens);
	w.setMaterialIndexArray(mat);

	auto valid = w.validate();

	PencilSource src;

	std::array<double, 3> position = { 0,0,-(dim[2] * spacing[2]) };
	std::array<double, 6> direction = { 1,0,0,0,1,0 };
	src.setPosition(position);
	src.setDirectionCosines(direction);
	src.setHistoriesPerExposure(nHistories);
	src.setTotalExposures(2);
	src.setPhotonEnergy(energy);
	src.updateFromWorld(w); // not needed really
	src.validate();

	const auto dose = transport::run(w, &src);

	std::vector<double> doseDepth(dim[2], 0.0);
	
	for (std::size_t k = 0; k < dim[2]; ++k)
	{
		double doseSlice = 0;
		for (std::size_t i = 0; i < dim[0]; ++i)
			for (std::size_t j = 0; j < dim[1]; ++j)
			{
				const auto idx = index(i, j, k, dim);
				doseSlice += dose[idx];
			}
		doseDepth[k] = doseSlice;
	}
	for (std::size_t i = 0; i < dim[2]; ++i)
	{
		std::cout << spacing[2]*i << " ,  " << doseDepth[i] << '\n';
	}

}

void validateCTAngle()
{
	CTAxialSource src;

}


void validateSpecter()
{
	Tube t;
	t.setVoltage(100.0);
	t.setAlFiltration(7.0);
	t.setTubeAngleDeg(12.0);
	t.setEnergyResolution(0.2);
	auto specter = t.getSpecter();
	for (auto[e, n] : specter)
	{
		std::cout << e << " ,  " << n <<  '\n';
	}

}

void validateSpecterSampling()
{
	const std::uint64_t nParticles = 10000000;
	CTAxialSource src;
	Exposure e;
	
	src.tube().setVoltage(100.0);
	const auto nVals = static_cast<std::uint64_t>(src.tube().voltage());
	
	std::vector<std::uint64_t> hist(nVals+1, 0);
	Particle p;
	std::uint64_t seed[2];
	randomSeed(seed);
	src.validate();
	src.getExposure(e, 0);
	for (std::uint64_t i = 0; i < nParticles; ++i)
	{
		e.sampleParticle(p, seed);
		auto idx = static_cast<std::uint64_t>(p.energy);
		hist[idx] += 1;
	}
	for (std::size_t i = 0; i < hist.size(); ++i)
	{
		std::cout << i << " ,  " << hist[i] << " ,  " << '\n';
	}
}

void validateReyleight()
{
	const double energy = 100.0; //kev
	const std::uint64_t nHistories = 1000000;

	Material water("H2O");
	water.setStandardDensity(1.0);
	World w;
	auto dump = std::make_shared<std::vector<double>>(3*3*3, 1.0);
	auto dumm = std::make_shared<std::vector<unsigned char>>(3 * 3 * 3, 0);
	w.setAttenuationLutMaxEnergy(energy);
	w.setDensityArray(dump);
	w.setMaterialIndexArray(dumm);
	w.addMaterialToMap(water);
	std::array<std::size_t, 3> dim = { 3, 3,3 };
	std::array<double, 3> spacing = { 3, 3,3 };
	w.setDimensions(dim);
	w.setSpacing(spacing);
	std::uint64_t seed[2];

	bool test = w.validate();
	auto attlut = w.attenuationLut();


	randomSeed(seed);
	Particle p;
	double cosAngle = 0.0;

	std::array<std::uint64_t, 180> hist;
	std::fill(hist.begin(), hist.end(), 0);
	for (std::uint64_t i = 0; i < nHistories; ++i)
	{
		p.energy = energy;
		transport::rayleightScatter(p, 0, attlut, seed, cosAngle);
		std::size_t angle = static_cast<std::size_t>(std::acos(cosAngle)*RAD_TO_DEG);
		hist[angle] += 1;
	}


	for (std::size_t i = 0; i < hist.size(); ++i)
	{
		const double rad = i * DEG_TO_RAD;
		//const double analytical = DCS_KN(energy, rad);
		const double analytical = DCS_Rayl_CP("H20", energy, rad);
		std::cout << i << " ,  " << hist[i] << " ,  " <<  analytical << '\n';
	}
}

double comptDiffCross(double E0, double angle)
{
	constexpr double mec2 = 510.9989461;
	const double E1 = E0 * mec2 / (mec2 + E0 * (1.0 - std::cos(angle)));
	const double e = E1 / E0;
	const double sinang = std::sin(angle);
	return (1.0 / e + e)*(1.0 - e * sinang*sinang / (1.0 + e * e));
}

void validateCompton()
{

	const double energy = 662.0; //kev
	const std::uint64_t nHistories = 1000000;

	std::uint64_t seed[2];
	randomSeed(seed);
	Particle p;
	double cosAngle=0.0;
	
	std::array<std::uint64_t, 180> hist;
	std::fill(hist.begin(), hist.end(), 0);
	for (std::uint64_t i = 0; i < nHistories; ++i)
	{
		p.energy = energy;
		transport::comptonScatter(p, seed ,cosAngle);
		std::size_t angle = static_cast<std::size_t>(std::acos(cosAngle)*RAD_TO_DEG);
		hist[angle] += 1;
	}


	std::array<std::uint64_t, 180> histG;
	std::fill(histG.begin(), histG.end(), 0);
	for (std::uint64_t i = 0; i < nHistories; ++i)
	{
		p.energy = energy;
		const double absEnergy =  transport::comptonScatterEGS(p, seed, cosAngle);

		const double energyFinalAna = energy * MEC2 / (MEC2 + energy * (1 - cosAngle));
		const double energyFinalCalc = energy - absEnergy;

		const double cosAngleAna = MEC2 * (1.0 / energy - 1.0 / energyFinalAna) + 1;
		const double anglef = std::acos(cosAngle);
		

		std::size_t angle = static_cast<std::size_t>(std::abs(std::acos(cosAngle))*RAD_TO_DEG);
		histG[angle] += 1;
	}

	for (std::size_t i =0 ; i < hist.size() ; ++i)
	{
		const double rad = i * DEG_TO_RAD;
		//const double analytical = DCS_KN(energy, rad);
		const double analytical =  DCS_Compt(8, energy, rad);
		std::cout << i << " ,  " << hist[i] << " ,  " << histG[i] << " ,  " << comptDiffCross(energy, rad) << " ,  " <<analytical << '\n';
	}

}

void validateTransport()
{
	SetErrorMessages(0);
	Material air("Air, Dry (near sea level)");
	Material water("Water, Liquid");
	Material al(13);
	std::array<std::size_t, 3> dim = { 3,3,100 };
	std::array<double, 3> spacing = { 0.1,0.1,1 };

	std::size_t size = dim[0] * dim[1] * dim[2];

	auto dens = std::make_shared<std::vector<double>>(size, air.standardDensity());
	auto mat = std::make_shared<std::vector<unsigned char>>(size, 0);

	for (std::size_t i = 0; i < dim[2]; ++i)
	{
		auto dbuffer = dens->data();
		auto mbuffer = mat->data();
		auto idx = index(dim[0] / 2, dim[1] / 2, i, dim);
		dbuffer[idx] = water.standardDensity();
		mbuffer[idx] = 1;
		if (i == dim[2] - 1)
		{
			dbuffer[idx] = al.standardDensity();
			mbuffer[idx] = 2;
		}
	}


	//making world
	World w;
	w.setDimensions(dim);
	w.setSpacing(spacing);
	w.setDensityArray(dens);
	w.setMaterialIndexArray(mat);
	w.addMaterialToMap(air);
	w.addMaterialToMap(water);
	w.addMaterialToMap(al);
	auto src = std::make_unique<PencilSource>();
	src->setPhotonEnergy(60.0);
	std::array<double, 3> spos = { 0,0,-(dim[2] * spacing[2]) };
	//for (std::size_t i = 0; i < 2; ++i)
	//	spos[i] += dim[i] * spacing[i] / 2.0;
	std::array<double, 6> scos = { 1,0,0,0,1,0 };
	src->setPosition(spos);
	src->setDirectionCosines(scos);
	src->setHistoriesPerExposure(1000000);
	src->setTotalExposures(10);

	std::array<double, 3> beamDirection;
	vectormath::cross(scos.data(), beamDirection.data());

	w.setAttenuationLutMaxEnergy(src->maxPhotonEnergyProduced());
	w.validate();

	auto dose = transport::run(w, src.get());


	for (std::size_t i = 0; i < dim[2]; ++i)
	{
		auto idx = index(dim[0] / 2, dim[1] / 2, i, dim);
		std::cout << i * spacing[2] << " ,  " << dose[idx] << '\n';
	}

}

int main(int argc, char *argv[])
{
	//validateReyleight();
	//validateCompton();
	validateDose();
	//validateSpecter();
	//validateTransport();
	return 0;
}


