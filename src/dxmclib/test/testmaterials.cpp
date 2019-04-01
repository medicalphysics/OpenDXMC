

#include "materials.h"
#include <iostream>


int main (int argc, char *argv[])
{
    Materials materials;
    materials.addNISTMaterial("Urea");
	materials.addNISTMaterial(2);
    materials.addByCompoundName("H2O");
	materials.addByCompoundName("H2O");
	materials.addNISTMaterial("Bone, Compact (ICRU)");
	materials.addNISTMaterial("Bone, Compact (ICRU)");
	if (materials.count() != 4)
	{
		return 1;
	}
	std::vector<std::string> names;
	std::vector<double> densities;
    materials.getNames(names);
	materials.getDensities(densities);
	for (auto it = names.begin(); it != names.end(); ++it) std::cout << *it << ", ";
	std::cout << std::endl;
	for (auto it = densities.begin(); it != densities.end(); ++it) std::cout << *it << ", ";
	std::cout << std::endl;

	double d1 = materials.getDensity("H2O");
	bool d = materials.setDensity("H2O", 0.5);
	double d2 = materials.getDensity("H2O");


    materials.removeAll();

    return 0;
}
