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

#pragma once // include guard


#include "dxmcrandom.h"
#include "beamfilters.h"
#include "exposure.h"
#include "beamfilters.h"
#include "progressbar.h"
#include "tube.h"
#include "world.h"
#include "dxmcrandom.h"
#include <vector>
#include <memory>
#include <array>


class Source
{
public:
	enum Type { None, CTSpiral, CTAxial, DX, CTDual, Pencil };
    Source();
    virtual bool getExposure(Exposure& exposure, std::uint64_t i) const = 0;
	
	//Tube& tube(void);
	//const Tube& tube(void) const { return m_tube; }
	
	virtual double maxPhotonEnergyProduced() const {return  Tube::maxVoltage(); }

	void setPositionalFilter(std::shared_ptr<PositionalFilter> filter) { m_positionalFilter = filter; }
	std::shared_ptr<PositionalFilter> positionalFilter(void) { return m_positionalFilter; }
	const std::shared_ptr<PositionalFilter> positionalFilter() const { return m_positionalFilter; }
	
	//virtual const SpecterDistribution* specterDistribution(void) const { return m_specterDistribution.get(); }
	
	void setPosition(const std::array<double, 3>& position) { m_position = position; }
	std::array<double, 3>& position(void) { return m_position; }
	const std::array<double, 3>& position(void) const { return m_position; }
	
	void setDirectionCosines(const std::array<double, 6>& cosines);
	const std::array<double, 6>& directionCosines(void) const { return m_directionCosines; }
	std::array<double, 6>& directionCosines(void) { return m_directionCosines; }

	void setHistoriesPerExposure(std::uint64_t histories);
	std::uint64_t historiesPerExposure(void) const;

	virtual std::uint64_t totalExposures(void) const = 0;

	Type type() { return m_type; }

	virtual double getCalibrationValue(std::uint64_t nHistories, ProgressBar*) = 0;
	
	virtual bool isValid(void) const = 0;
	virtual bool validate(void) = 0;
	void updateFromWorld(const World& world) { if (m_positionalFilter) m_positionalFilter->updateFromWorld(world); }

protected:
	
	std::array<double, 3> m_position = { 0,0,0 };
	std::array<double, 6> m_directionCosines = { 1,0,0,0,1,0 };
	std::uint64_t m_historiesPerExposure = 1;
	std::shared_ptr<PositionalFilter> m_positionalFilter;
	Type m_type = None;
	
private:
	void normalizeDirectionCosines(void);
};


class PencilSource :public Source
{
public:
	PencilSource();

	bool getExposure(Exposure& exposure, std::uint64_t i) const override;

	void setPhotonEnergy(double energy);
	double photonEnergy() const { return m_photonEnergy; }

	double maxPhotonEnergyProduced() const override { return m_photonEnergy; }

	std::uint64_t totalExposures(void) const override { return m_totalExposures; }
	void setTotalExposures(std::uint64_t exposures) { if (exposures > 0) m_totalExposures = exposures; }

	void setAirDose(double Gycm2) { if (Gycm2 > 0.0) m_airDose = Gycm2; }
	double airDose(void) const { return m_airDose; } // Gycm2

	double getCalibrationValue(std::uint64_t nHistories, ProgressBar* = nullptr) override;

	bool isValid(void) const override { return true; }
	bool validate(void) override {return true; }

protected:
	double m_photonEnergy = 100.0;
	double m_airDose = 1.0;
	std::uint64_t m_totalExposures = 10;
};


class DXSource : public Source
{
public:
	DXSource();
	bool getExposure(Exposure& exposure, std::uint64_t i) const override;
	
	Tube& tube(void) { m_specterValid = false; return m_tube; };
	const Tube& tube(void) const { return m_tube; }

	double maxPhotonEnergyProduced() const override { return m_tube.voltage(); }

	std::uint64_t totalExposures(void) const override;
	void setTotalExposures(std::uint64_t exposures);

	void setCollimationAngles(const std::array<double, 2>& radians);
	const std::array<double, 2>& collimationAngles(void) const;

	void setCollimationAnglesDeg(const std::array<double, 2>& degrees);
	const std::array<double, 2> collimationAnglesDeg(void) const;

	void setFieldSize(const std::array<double, 2>& mm);
	const std::array<double, 2>& fieldSize(void) const;

	void setSourceDetectorDistance(double mm);
	double sourceDetectorDistance() const;

	void setDap(double Gycm2) { if (Gycm2 > 0.0) m_dap = Gycm2; }
	double dap(void) const { return m_dap; } // Gycm2

	double getCalibrationValue(std::uint64_t nHistories, ProgressBar* = nullptr) override;
	
	const std::array<double, 3> tubePosition(void) const;

	bool isValid(void) const override { return m_specterValid; }
	bool validate(void) override { updateSpecterDistribution(); return m_specterValid; }

protected:
	void updateFieldSize(const std::array<double, 2>& collimationAngles);
	void updateCollimationAngles(const std::array<double, 2>& fieldSize);
	void updateSpecterDistribution();

private:
	double m_sdd = 1000.0;
	double m_dap = 1.0; // Gycm2
	std::array<double, 2> m_fieldSize;
	std::array<double, 2> m_collimationAngles;
	std::uint64_t m_totalExposures = 1000;
	Tube m_tube;
	bool m_specterValid = false;
	std::unique_ptr<SpecterDistribution> m_specterDistribution;

};


class CTSource : public Source
{
public:
    CTSource();
	virtual bool getExposure(Exposure& exposure, std::uint64_t i) const = 0;

	Tube& tube(void) { m_specterValid = false; return m_tube; };
	const Tube& tube(void) const { return m_tube; }

	virtual double maxPhotonEnergyProduced() const override {return m_tube.voltage(); }

	void setBowTieFilter(std::shared_ptr<BeamFilter> filter) { m_bowTieFilter = filter; }
	std::shared_ptr<BeamFilter> bowTieFilter(void) { return m_bowTieFilter; }
	const std::shared_ptr<BeamFilter> bowTieFilter() const { return m_bowTieFilter; }

	void setSourceDetectorDistance(double sdd);
    double sourceDetectorDistance(void) const;
    
	void setCollimation(double mmCollimation);
    double collimation(void) const;
    
	void setFieldOfView(double fov);
    double fieldOfView(void) const;

    void setStartAngle(double angle);
    double startAngle(void) const;
	void setStartAngleDeg(double angle);
	double startAngleDeg(void) const;
    
	void setExposureAngleStep(double);
    double exposureAngleStep(void) const;
	void setExposureAngleStepDeg(double);
	double exposureAngleStepDeg(void) const;
    
    virtual void setScanLenght(double scanLenght);
    double scanLenght(void) const;

	void setCtdiVol(double ctdivol) { if (ctdivol > 0.0) m_ctdivol = ctdivol; }
	double ctdiVol(void) const { return m_ctdivol; }

	bool useXCareFilter() const { return m_useXCareFilter; }
	void setUseXCareFilter(bool use) { m_useXCareFilter = use; }
	XCareFilter& xcareFilter() { return m_xcareFilter; }
	const XCareFilter& xcareFilter() const { return m_xcareFilter; }

	virtual std::uint64_t totalExposures(void) const = 0;

	void setCtdiPhantomDiameter(std::uint64_t mm) { if (mm > 160) m_ctdiPhantomDiameter = mm; else m_ctdiPhantomDiameter = 160; }
	std::uint64_t ctdiPhantomDiameter(void) const { return m_ctdiPhantomDiameter; }

	virtual double getCalibrationValue(std::uint64_t nHistories, ProgressBar* = nullptr) override;
	
	virtual std::uint64_t exposuresPerRotatition() const;
	

	bool isValid(void) const override { return m_specterValid; };
	virtual bool validate(void) override { updateSpecterDistribution(); return m_specterValid; };

protected:
	virtual void updateSpecterDistribution();

    double m_sdd;
    double m_collimation;
    double m_fov;
    double m_startAngle;
    double m_exposureAngleStep;
    double m_scanLenght;
	double m_ctdivol = 1.0;
	std::uint64_t m_ctdiPhantomDiameter = 320;
	std::shared_ptr<BeamFilter> m_bowTieFilter;
	XCareFilter m_xcareFilter;
	bool m_useXCareFilter = false;
	bool m_specterValid = false;
	Tube m_tube;
	std::unique_ptr<SpecterDistribution> m_specterDistribution;
};

class CTSpiralSource : public CTSource
{
public:
	CTSpiralSource();
	
	bool getExposure(Exposure& exposure, std::uint64_t i) const override;
	std::array<double, 3> getExposurePosition(std::uint64_t i) const;
	
	void setPitch(double pitch);
	double pitch(void) const;

	//void setScanLenght(double scanLenght) override;

	std::uint64_t totalExposures(void) const override;
	double getCalibrationValue(std::uint64_t nHistories, ProgressBar* = nullptr) override;
protected:
private:
	double m_pitch;

};

class CTAxialSource : public CTSource
{
public:
	CTAxialSource();

	bool getExposure(Exposure& exposure, std::uint64_t i) const override;
	std::array<double, 3> getExposurePosition(std::size_t i) const;

	void setStep(double step);
	double step(void) const;
	void setScanLenght(double scanLenght) override;

	std::uint64_t totalExposures(void) const override;
	
protected:
private:
	double m_step;

};

class CTDualSource :public CTSource
{
public:

	//Source overrides
	CTDualSource();
	bool getExposure(Exposure& exposure, std::uint64_t i) const override;
	std::array<double, 3> getExposurePosition(std::uint64_t i) const;

	double tubeAmas() const { return m_tubeAmas; }
	double tubeBmas() const { return m_tubeBmas; }
	void setTubeAmas(double mas) { m_specterValid = false; m_tubeAmas = std::max(0.0, mas); }
	void setTubeBmas(double mas) { m_specterValid = false; m_tubeBmas = std::max(0.0, mas); }

	Tube& tubeB(void) { m_specterValid = false; return m_tubeB; };
	const Tube& tubeB(void) const { return m_tubeB; }

	double maxPhotonEnergyProduced()const { return std::max(m_tube.voltage(), m_tubeB.voltage()); }


	std::uint64_t totalExposures(void) const override;
	std::uint64_t exposuresPerRotatition() const;
	double getCalibrationValue(std::uint64_t nHistories, ProgressBar* = nullptr) override;


	// CT

	void setBowTieFilterB(std::shared_ptr<BeamFilter> filter) { m_bowTieFilterB = filter; }
	std::shared_ptr<BeamFilter> bowTieFilterB(void) { return m_bowTieFilterB; }
	const std::shared_ptr<BeamFilter> bowTieFilterB() const { return m_bowTieFilterB; }

	void setSourceDetectorDistanceB(double sdd) { m_sddB = std::abs(sdd); }
	double sourceDetectorDistanceB(void) const { return m_sddB; }	

	void setFieldOfViewB(double fov) { m_fovB = std::abs(fov); }
	double fieldOfViewB(void) const { return m_fovB; }

	void setPitch(double pitch);
	double pitch(void) const;

	void setStartAngleB(double angle) { m_startAngleB = angle; }
	double startAngleB(void) const { return m_startAngleB; }
	void setStartAngleDegB(double angle);
	double startAngleDegB(void) const;

	bool validate(void) override { updateSpecterDistribution(); return m_specterValid; };
protected:
	void updateSpecterDistribution() override;
private:
	Tube m_tubeB;
	std::unique_ptr<SpecterDistribution> m_specterDistributionB;
	double m_sddB;
	double m_fovB;
	double m_startAngleB = 0.0;
	double m_pitch = 1.0;
	double m_tubeAmas = 100.0;
	double m_tubeBmas = 100.0;
	double m_tubeBweight;
	double m_tubeAweight;
	std::shared_ptr<BeamFilter> m_bowTieFilterB;
};
