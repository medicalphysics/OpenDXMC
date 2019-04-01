
#include "xraylib.h"
#include <iostream>




int main (int argc, char *argv[])
{
	double pi = 3.14159265359;
	double ang = 90 * pi / 180.0;
	double energy = 60.0;

    std::cout << "Test passed" << std::endl;
	std::cout << MomentTransf(energy,ang) << std::endl;

	constexpr double hc = 1239.841984 * 1E-8;
	std::cout << energy*std::sin(ang / 2.0) * hc << std::endl;
	std::cout << MomentTransf(energy, ang) / (energy*std::sin(ang / 2.0)) << std::endl;
	std::cout << MomentTransf(energy, ang) / (energy*std::sin(ang / 2.0)) << std::endl;
    return 0;
}
