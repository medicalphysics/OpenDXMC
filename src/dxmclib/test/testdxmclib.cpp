
#include "transport.h"
#include "vectormath.h"
#include <iostream>
#include <fstream>
#include <string>
#include <array>

inline std::size_t index(std::size_t i, std::size_t j, std::size_t k, const std::size_t dim[3])
{
    return i*dim[1]*dim[2] + j*dim[2]+k;
}

template<typename T>
void writeToFile(const std::string& filename, const std::vector<T>& array)
{
    std::ofstream ofs(filename.c_str(), std::ios::binary);
    if(ofs.is_open())
    {
        ofs.write(reinterpret_cast<const char*>(array.data()), sizeof(T) * array.size() );
        ofs.close();
    }
}


int testTransport()
{
    std::size_t N = 100000;
    double energy = 5.0;


    Particle p;
    for (std::size_t i = 0;i < 3; i++)
        p.pos[i] = 0.0;
    p.dir[0] = 0.0;
    p.dir[1] = 0.0;
    p.dir[2] = 1.0;
    p.energy = energy;

    std::vector<Material> materials;
    //materials.push_back(Material("Water, Liquid"));
    materials.push_back(Material("C"));
   
   
	return 1;
}

int testWorldSetup()
{
    //setting up world
    World w = World();
    w.setDimensions(27, 27, 128);
    w.setSpacing(1, 1, 1);

    auto dens = w.densityArray();
    auto matInd = w.materialIndexArray();

    Material air("Air, Dry (near sea level)");
    Material water("Water, Liquid");
    Material lead("Pb");
    // filling buffers
    auto dim = w.dimensions();
    std::size_t p1[3], p2[3];
    for (std::size_t i = 0;i < 3;i++)
    {
        p1[i] = dim[i] / 4;
        p2[i] = dim[i] / 4 + p1[i]*2;
    }

    for(std::size_t i = 0; i < dim[0]; i++)
        for(std::size_t j = 0; j < dim[1]; j++)
            for(std::size_t k = 0; k < dim[2]; k++)
            {
                std::size_t ind = index(i, j, k, dim.data());
                //dens[ind] = 1.0;
                //matInd[ind] = 1;
                //filling center of array
                if (i < p1[0] || i > p2[0] || j < p1[1] || j > p2[1] || k < p1[2] || k >p2[2] )
                {
                    (*dens)[ind] = air.standardDensity();
                    (*matInd)[ind] = 0;
                }
                else
                {
					(*dens)[ind] = water.standardDensity();
					(*matInd)[ind] = 1;
                }
            }
    //dens[index(dim[0]/2, dim[1]/2, dim[2]/2, dim)] = 11.34;
    //matInd[index(dim[0]/2, dim[1]/2, dim[2]/2, dim)] = 2;
    w.addMaterialToMap(air);
    w.addMaterialToMap(water);
    w.addMaterialToMap(lead);




    DXSource src = DXSource();

    auto tube = src.tube();
    tube.setAlFiltration(7.0);
	tube.setTubeAngleDeg(12.0);
    tube.setVoltage(120.0);
    src.validate();

	std::array<double, 6> cosines{ 1, 0, 0, 0, 1, 0 };
    //double cosines[6] = { 1, 0, 0, 0, 1, 0 };
    double rotAxis[3] = { 0, 1, 0 };
    //vectormath::rotate(cosines, rotAxis, M_PI / 4.0);
    //vectormath::rotate(&cosines[3], rotAxis, M_PI / 4.0);
    w.setDirectionCosines(cosines);

    w.setOrigin(0, 0, 0);
    auto worldCenter = w.origin();
   
    w.validate();
    bool validWorld = w.isValid();

    std::array<double, 3> sourcePos;
    for (std::size_t i = 0; i < 3; i++)
        sourcePos[i] = worldCenter[i] - w.depthDirection()[i] * 512.0;
    src.setPosition(sourcePos);
    src.setDirectionCosines(cosines);
    src.setHistoriesPerExposure(500000);
	/*
    Simulation sim = Simulation();
    sim.addSource(&src);
    sim.run(w);
	*/

    //write debug files
    //writeToFile("density.bin", *w.densityArray());
    //writeToFile("material.bin", *w.materialIndexArray());


    return 0;
}

int main (int argc, char *argv[])
{
    //SetErrorMessages(0); // supress warning messages from xraylib

    int valid = 0;
    //valid += testTransport();
    valid += testWorldSetup();

    return valid;
}
