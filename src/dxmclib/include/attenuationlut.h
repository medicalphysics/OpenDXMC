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





#pragma once

#include "material.h"
#include <vector>
#include <array>



class AttenuationLut
{
public:
	AttenuationLut() {};
	void setEnergyResolution(double keV) { m_energyStep = keV > 0.1 ? keV : 0.1; }
	double energyResolution()const { return m_energyStep; }
	void generate(const std::vector<Material>& materials, double minEnergy=0.0, double maxEnergy = 150.0);
	void generate(const std::vector<Material>& materials, const std::vector<double>& energies);
	
	double maxMassTotalAttenuation(double energy) const;
	template<typename It1, typename It2>
	void generateMaxMassTotalAttenuation(It1 materialIndexBegin, It1 materialIndexEnd, It2 densityBegin);

	double totalAttenuation(std::size_t material, double energy) const;
	double rayleightAttenuation(std::size_t material, double energy) const;
	double photoelectricAttenuation(std::size_t material, double energy) const;
	double comptonAttenuation(std::size_t material, double energy) const;
	std::array<double, 3> photoComptRayAttenuation(std::size_t material, double energy) const;

	std::vector<double>::iterator energyBegin() { return m_attData.begin(); }
	std::vector<double>::iterator energyEnd() { return m_attData.begin() + m_energyResolution; }
	std::vector<double>::iterator attenuationTotalBegin(std::size_t material) { return m_attData.begin() + m_energyResolution + m_energyResolution * 4 * material; } // todo error checking
	std::vector<double>::iterator attenuationTotalEnd(std::size_t material) { return m_attData.begin() + m_energyResolution + m_energyResolution * 4 * material + m_energyResolution; } // todo error checking


	double momentumTransferSquared(std::size_t material, double cumFormFactorSquared) const;
	double cumFormFactorSquared(std::size_t material, double momentumTransferSquared) const;
	
	static double momentumTransfer(double energy, double angle);
	static double momentumTransferMax(double energy);

protected:
	void generateFFdata(const std::vector<Material>& materials);
private:
	double m_minEnergy = 0;
	double m_maxEnergy = 150.0;
	double m_energyStep = 1.0;
	double m_momtMaxSqr = 0;
	double m_momtStepSqr = 0;
	std::size_t m_energyResolution = 150;
	std::size_t m_materials;
	std::vector<double> m_attData; // energy, array-> total, photo, compton, rauleight
	std::vector<double> m_coherData; //qsquared, array-> A(qsquared)
	std::vector<double> m_maxMassAtt;
};



template<typename It1, typename It2>
void AttenuationLut::generateMaxMassTotalAttenuation(It1 materialIndexBegin, It1 materialIndexEnd, It2 densityBegin)
{
	std::vector<double> maxDens(m_materials, 0.0);
	
	while (materialIndexBegin != materialIndexEnd)
	{
		maxDens[*materialIndexBegin] = std::max(maxDens[*materialIndexBegin], *densityBegin);
		++materialIndexBegin;
		++densityBegin;
	}

	m_maxMassAtt.resize(m_energyResolution);
	std::fill(m_maxMassAtt.begin(), m_maxMassAtt.end(), 0.0);
	for (std::size_t material = 0; material < m_materials; ++material)
	{
		for (std::size_t i = 0; i < m_energyResolution; ++i)
		{
			//m_maxMassAtt[i] = std::max(m_maxMassAtt[i], totalAttenuation(material, m_coherData[i])*maxDens[material]);
			m_maxMassAtt[i] = std::max(m_maxMassAtt[i], totalAttenuation(material, m_attData[i])*maxDens[material]);
		}
	}
}