 
#pragma once

#include <string>
#include <vector>

class Material
{
public:
    Material(const std::string& xraylibMaterialNameOrCompound = "", const std::string& prettyName = "");
    Material(int atomicNumber);
	bool isValid(void) const { return m_valid && m_hasDensity; }
	const std::string& name(void) const { return m_name; }
	const std::string& prettyName(void) const { return m_prettyName; }
	

    bool hasStandardDensity(void) const {return m_hasDensity;}
    double standardDensity(void) const {return m_density;}
	void setStandardDensity(double density); // g/cm3

	std::vector<double> getFormFactorSquared(const std::vector<double>& momentumTransfer) const;
	
	double getPhotoelectricAttenuation(double energy) const;
	double getRayleightAttenuation(double energy) const;
	double getComptonAttenuation(double energy) const;
	double getTotalAttenuation(double energy) const;
	double getMassEnergyAbsorbtion(double energy) const;
	
	static double getAtomicWeight(int Z);
	static std::string getAtomicNumberToSymbol(int Z);

	static std::vector<std::string> getNISTCompoundNames(void);
	static int getAtomicNumberFromSymbol(const std::string &symbol);
	static std::string getSymbolFromAtomicNumber(int Z);

protected:
    void setByCompoundName(const std::string& name);
    void setByMaterialName(const std::string& name);
	void setByAtomicNumber(int index);
private:
	std::string m_name;
	std::string m_prettyName;
    double m_density = -1.0;
	bool m_valid = false;
	bool m_hasDensity = false;
};


