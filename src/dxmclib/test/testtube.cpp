 
#include <iostream>

#include "tube.h"



int main (int argc, char *argv[])
{
	Tube* tube = new Tube();
	auto energies = tube->getEnergy();
	auto specter = tube->getSpecter(energies);
    return 0;
}
