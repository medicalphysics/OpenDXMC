 

#include "world.h"
#include <iostream>


inline std::size_t index(std::size_t i, std::size_t j, std::size_t k, std::size_t dim[3])
{
	return i + dim[0] * j + dim[0] * dim[1] * k;
}


int testCTDIgeneration(void)
{
	CTDIPhantom w;

	w.validate();
	bool valid = w.isValid();



	return !valid;
}


int main (int argc, char *argv[])
{

	auto ctdiphantom = testCTDIgeneration();    
    return ctdiphantom;
}
