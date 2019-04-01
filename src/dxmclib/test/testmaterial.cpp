 

#include "material.h"
#include "transport.h"
#include <iostream>
#include <numeric>

template<typename T>
void writeToFile(const std::string& filename, const std::vector<T>& array)
{
	std::ofstream ofs(filename.c_str(), std::ios::binary);
	if (ofs.is_open())
	{
		ofs.write(reinterpret_cast<const char*>(array.data()), sizeof(T) * array.size());
		ofs.close();
	}
}



int main (int argc, char *argv[])
{
	auto materialNISTList = Material::getNISTCompoundNames();

    std::vector<Material> materials;

    materials.push_back(Material("Urea"));
    materials.push_back(Material(2));
    materials.push_back(Material("H2O"));
    materials.push_back(Material("Bone, Compact (ICRU)"));

    std::map<std::size_t, Material> mapping;
    for (std::size_t i =0;i < materials.size();i++)
		mapping[i] = materials[i];


    for (auto& m : materials)
        std::cout << m.name() << " Valid: " <<  m.isValid() <<  std::endl;
    

    return 0;
}
