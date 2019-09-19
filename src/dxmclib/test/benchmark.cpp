


#include "material.h"
#include "transport.h"
#include "world.h"

#include <execution>
#include <algorithm>
#include <array>
#include <iostream>
#include <chrono>


inline std::array<std::size_t, 3> index3(std::size_t idx, const std::array<std::size_t, 3>& dim)
{
	auto x = idx / (dim[1] * dim[2]);  //Note the integer division . This is x
	auto y = (idx - x * dim[1] * dim[2]) / dim[2]; //This is y
	auto z = idx - x * dim[1] * dim[2] - y * dim[2];//This is z 
	return std::array<std::size_t, 3> {x, y, z};
}
inline std::size_t index1(std::size_t x, std::size_t y, std::size_t z, const std::array<std::size_t, 3> & dim)
{
	return z * dim[1] * dim[2] + y * dim[2] + x;
}


inline std::size_t partitionBox(std::size_t x, std::size_t y, std::size_t z, std::size_t partitions, const std::array<std::size_t, 3> & dim)
{
	const std::size_t idx[3] = { x, y, z };
	for (std::size_t p = partitions; p > 0; --p)
	{
		bool inside = true;
		for (std::size_t i = 0; i < 3; ++i)
		{
			auto llim = dim[i] / ((partitions) * 2) * p;
			auto hlim = dim[i] - llim;
			inside = inside && idx[i] >= llim && idx[i] < hlim;
		}
		if (inside)
			return p-1;
	}
	return 0;
}

void run()
{
	std::vector<Material> materials;
	/*materials.push_back(Material("Air, Dry (near sea level)"));
	materials.push_back(Material("Water, Liquid"));
	materials.push_back(Material(13)); // Al
	*/
	materials.push_back(Material("Water, Liquid"));
	materials.push_back(Material("Water, Liquid"));
	materials.push_back(Material("Water, Liquid")); 
	std::array<std::size_t, 3> dim = { 100,100,100 };
	std::array<double, 3> spacing = { 0.1, 0.1, 0.1 };

	std::size_t size = dim[0] * dim[1] * dim[2];

	auto dens = std::make_shared<std::vector<double>>(size, materials[0].standardDensity());
	auto mat = std::make_shared<std::vector<unsigned char>>(size, 0);

	for (std::size_t k = 0; k < dim[2]; ++k)
		for (std::size_t j = 0; j < dim[1]; ++j)
			for (std::size_t i = 0; i < dim[0]; ++i)
			{
				auto idx = index1(i, j, k, dim);
				auto rIdx = partitionBox(i, j, k, 3, dim);
				auto dbuffer = dens->data();
				auto mbuffer = mat->data();
				dbuffer[idx] = materials[rIdx].standardDensity();
				mbuffer[idx] = rIdx;
			}


	//making world
	World w;
	w.setDimensions(dim);
	w.setSpacing(spacing);
	w.setDensityArray(dens);
	w.setMaterialIndexArray(mat);
	w.addMaterialToMap(materials[0]);
	w.addMaterialToMap(materials[1]);
	w.addMaterialToMap(materials[2]);
	auto src = std::make_unique<PencilSource>();
	src->setPhotonEnergy(60.0);
	std::array<double, 3> spos = { 0, 0,  - static_cast<double>(dim[2]) * spacing[2] };
	//std::array<double, 3> spos = { 0, 0, 0};
	std::array<double, 6> scos = { 1,0,0,0,1,0 };
	src->setPosition(spos);
	src->setDirectionCosines(scos);
	src->setHistoriesPerExposure(1000000);
	src->setTotalExposures(80);

	std::array<double, 3> beamDirection;
	vectormath::cross(scos.data(), beamDirection.data());

	w.setAttenuationLutMaxEnergy(src->maxPhotonEnergyProduced());
	w.validate();

	auto start = std::chrono::high_resolution_clock::now();
	auto dose = transport::run(w, src.get());
	auto end = std::chrono::high_resolution_clock::now();
	auto time = end - start;
	auto time_milli = std::chrono::duration_cast<std::chrono::milliseconds>(time).count();
	for (std::size_t i = 0; i < dim[2]; ++i)
	{
		auto idx = index1(dim[0] / 2, dim[1] / 2, i, dim);
		std::cout << i * spacing[2] << ", " << (*dens)[idx] << ", "  << static_cast<int>((*mat)[idx]) << ", "  << dose[idx] << '\n';
	}
	std::cout << "Simulation time: " << time_milli << " milliseconds" << '\n';
}


int main(int argc, char* argv[])
{
	run();
	return 0;
}


