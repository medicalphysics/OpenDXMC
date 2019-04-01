#pragma once

#include "material.h"

#include <vector>
#include <utility>


constexpr double TUBEMAXVOLTAGE = 150.0;
constexpr double TUBEMINVOLTAGE = 50.0;



class Tube
{
public:
	Tube(double tubeVoltage = 120.0, double tubeAngle = 12.0, double energyResolution = 1.0);

	double voltage() const { return m_voltage; }
	void setVoltage(double voltage);
	static constexpr double maxVoltage() { return TUBEMAXVOLTAGE; }
	static constexpr double minVoltage() { return TUBEMINVOLTAGE; }

	double tubeAngle() const { return m_angle; }
	double tubeAngleDeg() const;
	void setTubeAngle(double angle);
	void setTubeAngleDeg(double angle);

	void addFiltrationMaterial(const Material &filtrationMaterial, double mm) { m_filtrationMaterials.push_back(std::make_pair(filtrationMaterial, std::abs(mm))); }
	std::vector<std::pair<Material, double>>& filtrationMaterials() { return m_filtrationMaterials; }
	const std::vector<std::pair<Material, double>>& filtrationMaterials() const { return m_filtrationMaterials; }
	void setAlFiltration(double mm);
	void setCuFiltration(double mm);

	double AlFiltration() const;
	double CuFiltration() const;

	void clearFiltrationMaterials() { m_filtrationMaterials.clear(); }
	
	void setEnergyResolution(double energyResolution) { m_energyResolution = energyResolution;}
	std::vector<double> getEnergy() const;

	std::vector<std::pair<double, double>> getSpecter(bool normalize = true) const;
	std::vector<double> getSpecter(const std::vector<double> &energies, bool normalize = true) const;
	

protected:
	void addCharacteristicEnergy(const std::vector<double>& energy, std::vector<double>& specter) const;
	void filterSpecter(const std::vector<double>& energies, std::vector<double>& specter) const;
	void normalizeSpecter(std::vector<double>& specter) const;
private:
	
	double m_voltage, m_energyResolution, m_angle;
	std::vector<std::pair<Material, double>> m_filtrationMaterials;

};