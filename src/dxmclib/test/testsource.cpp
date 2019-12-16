 


#include "source.h"
#include <iostream>
#include <vector>

constexpr double pi = 3.14159265358979323846;  /* pi */

void testBowTieFilter()
{
    std::vector<double> ang({-1, -.5, 0, .5, 1});
    std::vector<double> w({0.1, 0.4, 1, 0.4, 0.1});
    auto bow = BowTieFilter(ang, w);

    std::cout << bow.sampleIntensityWeight(-0.25) << std::endl;
    std::cout << bow.sampleIntensityWeight(-2) << std::endl;
    std::cout << bow.sampleIntensityWeight(2) << std::endl;
    std::cout << bow.sampleIntensityWeight(1) << std::endl;
    std::cout << bow.sampleIntensityWeight(-1) << std::endl;
    std::cout << bow.sampleIntensityWeight(0) << std::endl;

}


void testSource()
{
    auto s1 = CTSpiralSource();
    auto t = s1.tube();
    t.setEnergyResolution(0.9);
	t.setAlFiltration(7.0);
    s1.validate();
    Exposure e1;

    s1.getExposure(e1, 0);

    Particle p1;
    std::uint64_t seed[2];
    randomSeed(seed);
    e1.sampleParticle(p1, seed);

	Exposure e2;
	s1.setStartAngle(pi);
	s1.validate();
	s1.getExposure(e2, 0);
	Particle p2;
	e2.sampleParticle(p2, seed);

	Exposure e3;
	s1.getExposure(e3, 180);



    /*Tube* tube = new Tube();
    tube->setAlFiltration(7.0);

    std::vector<double> energies, specter;
    for (std::size_t i = 10; i < 250; i++)
    {
        energies.push_back(i / 2.0);
    }
    tube->getEnergySpecter(energies, specter);
    SpecterDistribution* specterDist = new SpecterDistribution(specter, energies);

    BeamFilter* beamFilter = new BeamFilter();

    Source s2 = Source(tube, beamFilter);


    std::vector<double> angles({-0.523598775598, -0.261799387799, 0.0, 0.261799387799, 0.523598775598});
    std::vector<double> weight({0.5, 0.75, 1.0, 0.75, 0.5});

    BowTieFilter* bowFilter = new BowTieFilter(angles, weight);


    CTSource s3 = CTSource(tube, bowFilter);


    Exposure e = Exposure();

    s3.getExposure(e, 0);

    std::uint64_t seed[2];
    randomSeed(seed);

    Particle p;
    e.sampleParticle(p, seed, *specterDist, *bowFilter);
*/


}


int main (int argc, char *argv[])
{


    testSource();

    testBowTieFilter();



    std::uint64_t seed[2];
    randomSeed(seed);

    Tube tube = Tube(120.0);
    tube.setAlFiltration(7.0);

    std::vector<double> energies;
    energies.resize(120);
    double step = 10.0;
    for (double& e : energies)
    {
        e = step;
        step += 1.0;
    }

    std::vector<double> specter = tube.getSpecter(energies);

    SpecterDistribution specterDist = SpecterDistribution(specter, energies);

    Exposure exp = Exposure();

    

    Particle p;
    for (std::size_t i = 0; i < 100000; i++ )
    {
        exp.sampleParticle(p, seed);
    }

    return 0;
}
