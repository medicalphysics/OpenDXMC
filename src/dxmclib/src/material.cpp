/*This file is part of OpenDXMC.

OpenDXMC is free software : you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenDXMC is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenDXMC. If not, see < https://www.gnu.org/licenses/>.

Copyright 2019 Erlend Andersen
*/

#include "material.h"
#include "tube.h"
#include "xraylib.h"
#include <numeric>
#include <algorithm>
#include <execution>

double Material::getPhotoelectricAttenuation(double energy) const
{
	return CS_Photo_CP(m_name.c_str(), energy);
}

double Material::getRayleightAttenuation(double energy) const
{
	return CS_Rayl_CP(m_name.c_str(), energy);
}

double Material::getComptonAttenuation(double energy) const
{
	return CS_Compt_CP(m_name.c_str(), energy);
}

double Material::getTotalAttenuation(double energy) const
{
	return CS_Total_CP(m_name.c_str(), energy);
}

double Material::getMassEnergyAbsorbtion(double energy) const
{
	return CS_Energy_CP(m_name.c_str(), energy);
}

double Material::getAtomicWeight(int Z)
{
	return AtomicWeight(Z);
}

std::string Material::getAtomicNumberToSymbol(int Z)
{
	return std::string(AtomicNumberToSymbol(Z));
}

Material::Material(int atomicNumber)
{
	setByAtomicNumber(atomicNumber);
}



Material::Material(const std::string& xraylibMaterialNameOrCompound, const std::string& prettyName)
{
    setByMaterialName(xraylibMaterialNameOrCompound);
    if (!m_valid)
        setByCompoundName(xraylibMaterialNameOrCompound);
	if (prettyName.size() > 0)
		m_prettyName = prettyName;
	else
		m_prettyName = m_name;
}

void Material::setStandardDensity(double density)
{
	if (density > 0.0)
	{
		m_density = density;
		m_hasDensity = true;
	}
}

std::vector<std::string> Material::getNISTCompoundNames(void)
{
	int n_strings;
	auto charArray = GetCompoundDataNISTList(&n_strings);
	auto materialNames = std::vector<std::string>();
	for (int i = 0; i < n_strings; ++i)
	{
		if (charArray[i])
		{
			std::string s(charArray[i]);
			materialNames.push_back(s);
		}
	}
	return materialNames;
}

int Material::getAtomicNumberFromSymbol(const std::string &symbol)
{
	return SymbolToAtomicNumber(symbol.c_str());
}
std::string Material::getSymbolFromAtomicNumber(int Z)
{
	char *chars = AtomicNumberToSymbol(Z);
	std::string st(chars);
	xrlFree(chars);
	return st;
}


std::vector<double> Material::getFormFactorSquared(const std::vector<double>& momentumTransfer) const
{
	std::vector<double> fraction;
	std::vector<int> elements;

	struct compoundData* m = CompoundParser(m_name.c_str());
	if (m)
	{
		fraction.resize(m->nElements);
		elements.resize(m->nElements);
		for (int i = 0; i < m->nElements; i++)
		{
			elements[i] = m->Elements[i];
			fraction[i] = m->massFractions[i];
		}
		FreeCompoundData(m);
		m = nullptr;
	}
	struct compoundDataNIST* n = GetCompoundDataNISTByName(m_name.c_str());
	if (n)
	{
		fraction.resize(n->nElements);
		elements.resize(n->nElements);
		for (int i = 0; i < n->nElements; i++)
		{
			elements[i] = n->Elements[i];
			fraction[i] = n->massFractions[i];
		}
		FreeCompoundDataNIST(n);
		n = nullptr;
	}

	std::vector<double> formFactor(momentumTransfer.size(), 0.0);
	for (std::size_t i = 0; i < formFactor.size(); ++i)
	{
		for (std::size_t j = 0; j < fraction.size(); ++j)
		{
			formFactor[i]+= fraction[j] * FF_Rayl(elements[j], momentumTransfer[i]);
		}
		formFactor[i] = formFactor[i] * formFactor[i];
	}
	return formFactor;
}


void Material::setByCompoundName(const std::string &name)
{
    struct compoundData* m = CompoundParser(name.c_str());
    if (m)
    {
        m_name = name;
        m_valid = true;
        m_hasDensity = false;
        FreeCompoundData(m);
		m = nullptr;
    }
}

void Material::setByAtomicNumber(int atomicNumber)
{
	char *raw_name =  AtomicNumberToSymbol(atomicNumber);
	if (raw_name)
	{
		m_name = raw_name;
		xrlFree(raw_name);
		raw_name = nullptr;
		m_density = ElementDensity(atomicNumber);
		m_hasDensity = true;
		m_valid = true;
	}	
}


void Material::setByMaterialName(const std::string &name)
{
    struct compoundDataNIST* m = GetCompoundDataNISTByName(name.c_str());
    if (m)
    {
        m_name = m->name;
        m_valid = true;
        m_hasDensity = true;
        m_density = m->density;
        FreeCompoundDataNIST(m);
		m = nullptr;
    }
}
