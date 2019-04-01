



#include "xraylib.h"
#include <iostream>
#include "attenuationlut.h"
#include "material.h"
#include "dxmcrandom.h"

#include <vector>

int main(int argc, char *argv[])
{

	Material m(13);
	Material c(14);
	AttenuationLut att;
	std::vector<Material> mats;
	mats.push_back(m);
	mats.push_back(c);

	std::vector<double> energies;
	for (std::size_t i = 1; i < 150; ++i)
		energies.push_back(static_cast<double>(i));



	att.generate(mats, energies);
	
	double maxAtt = 0.0;
	double energy;
	std::uint64_t seed[2];
	randomSeed(seed);
	
	std::size_t teller = 0;

	while (teller < 1E7)
	{
		energy = randomUniform(seed, 150.0);
		maxAtt = att.maxMassTotalAttenuation(energy);
		if (maxAtt <= 0.0) {
			return 1;
		}
		++teller;
	}
	


	std::cout << att.totalAttenuation(0, 60.5)<< " "<<att.totalAttenuation(1, 60.5) << std::endl;
	std::cout << CS_Total(13, 60.5) << " "<<CS_Total(14, 60.5)<<  std::endl;

	return 0;
}
