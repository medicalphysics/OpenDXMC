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

#include "source.h"
#include "vectormath.h"
#include "material.h"
#include "dxmcrandom.h"
#include "transport.h"

#include <future>
#include <numeric>

constexpr double PI = 3.14159265358979323846;  /* pi */
constexpr double PI_2 = 2.0 * PI;
constexpr double DEG_TO_RAD = PI / 180.0;
constexpr double RAD_TO_DEG = 1.0 / DEG_TO_RAD;
constexpr double KEV_TO_MJ = 1.6021773e-13;

template<typename T>
T vectorLenght(const std::array<T, 3>& v)
{
	return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}


Source::Source()
{
	m_type = None;
	for (std::size_t i = 0; i < 3; i++)
	{
		m_position[i] = 0.0;
		m_directionCosines[i] = 0.0;
		m_directionCosines[i + 3] = 0.0;
	}
	m_directionCosines[0] = 1.0;
	m_directionCosines[4] = 1.0;
    m_historiesPerExposure = 1000;
}


void Source::setDirectionCosines(const std::array<double, 6>& cosines)
{
	m_directionCosines = cosines;
	normalizeDirectionCosines();
}

void Source::normalizeDirectionCosines()
{
	vectormath::normalize(m_directionCosines.data());
	vectormath::normalize(&m_directionCosines[3]);
}

void Source::setHistoriesPerExposure(std::uint64_t histories)
{
	m_historiesPerExposure = histories;
}

std::uint64_t Source::historiesPerExposure(void) const
{
	return m_historiesPerExposure;
}

PencilSource::PencilSource()
	:Source()
{
	m_type = Pencil;
}

bool PencilSource::getExposure(Exposure& exposure, std::uint64_t i) const
{
	exposure.setNumberOfHistories(m_historiesPerExposure);
	exposure.setPosition(m_position);
	exposure.setDirectionCosines(m_directionCosines);
	exposure.setCollimationAngles(0.0, 0.0);
	exposure.setMonoenergeticPhotonEnergy(m_photonEnergy);
	return i < m_totalExposures;
}
void PencilSource::setPhotonEnergy(double energy)
{
	if (energy > 500.0)
		m_photonEnergy = 500;
	else if (energy < 0.0)
		m_photonEnergy = 0.0;
	else
		m_photonEnergy = energy;
}

double PencilSource::getCalibrationValue(std::uint64_t nHistories)
{
	Material airMaterial("Air, Dry (near sea level)");
	const double calcOutput = nHistories * m_photonEnergy * airMaterial.getMassEnergyAbsorbtion(m_photonEnergy) * KEV_TO_MJ;
	const double factor = m_airDose / calcOutput;
	return factor;
}
DXSource::DXSource()
	:Source()
{
	m_type = DX;
	m_sdd = 1000.0;
	m_fieldSize[0] = 100.0;
	m_fieldSize[1] = 100.0;
	updateFieldSize(m_fieldSize);
	m_tube.setAlFiltration(2.0);
}
bool DXSource::getExposure(Exposure& exposure, std::uint64_t i) const
{
	exposure.setNumberOfHistories(m_historiesPerExposure);
	exposure.setPosition(tubePosition());
	exposure.setDirectionCosines(m_directionCosines);
	exposure.setCollimationAngles(m_collimationAngles.data());
	exposure.setSpecterDistribution(m_specterDistribution.get());
	return i < m_totalExposures;
}

std::uint64_t DXSource::totalExposures() const
{
	return m_totalExposures;
}
void DXSource::setTotalExposures(std::uint64_t nExposures)
{
	if (nExposures > 1)
		m_totalExposures = nExposures;
	else
		m_totalExposures = 1;
}

const std::array<double, 2>& DXSource::collimationAngles() const
{
	return m_collimationAngles;
}
void DXSource::setCollimationAngles(const std::array<double, 2>& angles)
{
	std::array<double, 2> ang = { std::abs(angles[0]), std::abs(angles[1]) };
	updateCollimationAngles(ang);
}
const std::array<double, 2> DXSource::collimationAnglesDeg() const
{
	std::array<double, 2> a{ m_collimationAngles[0] * RAD_TO_DEG,
		m_collimationAngles[1] * RAD_TO_DEG };
	return a;
}
void DXSource::setCollimationAnglesDeg(const std::array<double, 2>& angles)
{
	std::array<double, 2> ang = { 
		std::abs(angles[0]) * DEG_TO_RAD, 
		std::abs(angles[1]) * DEG_TO_RAD 
	};
	updateCollimationAngles(ang);
}

void DXSource::setFieldSize(const std::array<double, 2>& mm)
{
	std::array<double, 2> amm = { std::abs(mm[0]), std::abs(mm[1]) };
	updateFieldSize(amm);
}

const std::array<double, 2>& DXSource::fieldSize(void) const
{
	return m_fieldSize;
}

void DXSource::updateSpecterDistribution()
{
	if (!m_specterValid)
	{
		auto energies = m_tube.getEnergy();
		auto n_obs = m_tube.getSpecter(energies);
		m_specterDistribution = std::make_unique<SpecterDistribution>(n_obs, energies);
		m_specterValid = true;
	}
}


void DXSource::setSourceDetectorDistance(double mm)
{
	m_sdd = std::abs(mm);
	updateFieldSize(m_fieldSize);
}

double DXSource::sourceDetectorDistance() const
{
	return m_sdd;
}

double DXSource::getCalibrationValue(std::uint64_t nHistories)
{
	
	auto specter = tube().getSpecter();
	std::vector<double> massAbsorb(specter.size(), 0.0);
	Material airMaterial("Air, Dry (near sea level)");
	std::transform(specter.begin(), specter.end(), massAbsorb.begin(), [&](const auto &el)->double {return airMaterial.getMassEnergyAbsorbtion(el.first); });
	
	double nHist = static_cast<double>(nHistories);
	
	double calcOutput = 0.0; // Air KERMA [keV/g] 
	for (std::size_t i = 0; i < specter.size(); ++i)
	{
		auto &[keV, weight] = specter[i];
		calcOutput += keV * weight * nHist * massAbsorb[i];
	}
	calcOutput *= KEV_TO_MJ; // Air KERMA [J / kg] = [Gy]

	double output = m_dap / (m_fieldSize[0] * m_fieldSize[1] * 0.01); //mm->cm
	double factor = output / calcOutput;
	return factor;
}

void DXSource::updateCollimationAngles(const std::array<double, 2>& collimationAngles)
{
	for (std::size_t i = 0; i < 2; ++i)
	{
		m_collimationAngles[i] = collimationAngles[i];
		m_fieldSize[i] = std::tan(m_collimationAngles[i] * 0.5) * m_sdd * 2.0;
	}
}
void DXSource::updateFieldSize(const std::array<double, 2>& fieldSize)
{
	for (std::size_t i = 0; i < 2; ++i)
	{
		m_fieldSize[i] = fieldSize[i];
		m_collimationAngles[i] = std::atan(m_fieldSize[i] * 0.5 / m_sdd) * 2.0;
	}
}

const std::array<double, 3> DXSource::tubePosition(void) const
{
	std::array<double, 3> beamDirection;
	vectormath::cross(m_directionCosines.data(), beamDirection.data());
	std::array<double, 3> pos;
	for (std::size_t i = 0; i < 3; ++i)
	{
		pos[i] = m_position[i] - beamDirection[i] * m_sdd;
	}
	return pos;
}

CTSource::CTSource()
	:Source()
{
	m_type = None;
	m_sdd = 1190.0;
	m_collimation = 38.4;
	m_fov = 500.0;
    m_startAngle = 0.0;
	m_exposureAngleStep = DEG_TO_RAD;
    m_scanLenght = 100.0;
	auto &t = tube();
	t.setAlFiltration(7.0);

}





void CTSource::setSourceDetectorDistance(double sdd)
{
	m_sdd = std::abs(sdd);
}

double CTSource::sourceDetectorDistance(void) const
{
	return m_sdd;
}

void CTSource::setCollimation(double collimation)
{
	m_collimation = std::abs(collimation);
}

double CTSource::collimation(void) const
{
	return m_collimation;
}



void CTSource::setFieldOfView(double fov)
{
	m_fov = std::abs(fov);
}

double CTSource::fieldOfView(void) const
{
	return m_fov;
}

void CTSource::setStartAngle(double angle)
{
    m_startAngle = angle;
}

double CTSource::startAngle(void) const
{
    return m_startAngle;
}
void CTSource::setStartAngleDeg(double angle)
{
	m_startAngle = DEG_TO_RAD *angle;
}

double CTSource::startAngleDeg(void) const
{
	return RAD_TO_DEG * m_startAngle;
}

void CTSource::setExposureAngleStepDeg(double angleStep)
{
	setExposureAngleStep(angleStep * DEG_TO_RAD);
}

double CTSource::exposureAngleStepDeg(void) const
{
	return m_exposureAngleStep * RAD_TO_DEG;
}

void CTSource::setExposureAngleStep(double angleStep)
{
	const double absAngle = std::abs(angleStep);
	if (absAngle < PI)
		m_exposureAngleStep = absAngle;
}

double CTSource::exposureAngleStep(void) const
{
	return m_exposureAngleStep;
}


void CTSource::setScanLenght(double scanLenght)
{
	m_scanLenght = std::abs(scanLenght);
}
double CTSource::scanLenght(void) const
{
	return m_scanLenght;
}


double ctdiStatIndex(const std::array<double, 5>& measurements)
{
	double mean = 0.0;
	for (std::size_t i = 1; i < 5; ++i)
		mean += measurements[i];
	mean /= 4.0;
	if (mean <= 0.0)
		return 1000.0;
	double stddev = 0;
	for (std::size_t i = 1; i < 5; ++i)
		stddev += (measurements[i] - mean) * (measurements[i] - mean);
	stddev = std::sqrt(stddev / 3.0);

	return stddev / mean;
}

void CTSource::updateSpecterDistribution()
{
	if (!m_specterValid)
	{
		auto energies = m_tube.getEnergy();
		auto n_obs = m_tube.getSpecter(energies);
		m_specterDistribution = std::make_unique<SpecterDistribution>(n_obs, energies);
		m_specterValid = true;
	}
}


double CTSource::getCalibrationValue(std::uint64_t nHistories)
{

	double meanWeight = 0;
	for (std::size_t i = 0; i < totalExposures(); ++i)
	{
		Exposure dummy;
		getExposure(dummy, i);
		meanWeight += dummy.beamIntensityWeight();
	}
	meanWeight /= static_cast<double>(totalExposures());

	auto world = CTDIPhantom(m_ctdiPhantomDiameter);
	world.setAttenuationLutMaxEnergy(m_tube.voltage());
	world.validate();

	updateFromWorld(world);
	validate();

	std::array<double, 5> measureDoseTotal;
	measureDoseTotal.fill(0.0);
	std::array<CTDIPhantom::HolePosition, 5> position = { CTDIPhantom::Center, CTDIPhantom::West, CTDIPhantom::East, CTDIPhantom::South, CTDIPhantom::North };

	auto spacing = world.spacing();
	auto dim = world.dimensions();

	const double voxelVolume = spacing[0] * spacing[1] * spacing[2] / 1000.0; // cm3
	const double voxelMass = world.airDensity() * voxelVolume / 1000.0; //kg

	bool usingXCare = m_useXCareFilter;
	m_useXCareFilter = false; // we need to disable organ aec for ctdi statistics, this should be ok 

	std::size_t statCounter = 0;
	do {
		std::array<double, 5> measureDose;
		measureDose.fill(0.0);
		auto dose = transport::run(world, this);
		for (std::size_t i = 0; i < 5; ++i)
		{
			auto holeIndices = world.holeIndices(position[i]);
			for (auto idx : holeIndices)
				measureDose[i] += dose[idx];
			measureDose[i] /= static_cast<double>(holeIndices.size());
			measureDoseTotal[i] += measureDose[i];
		}
		++statCounter;
	} while ((ctdiStatIndex(measureDoseTotal) > 0.05) && (statCounter < 20)); // we allow 5% normal error in ctdi dose calibration

	const double ctdiPher = (measureDoseTotal[1] + measureDoseTotal[2] + measureDoseTotal[3] + measureDoseTotal[4]) / 4.0;
	const double ctdiCent = measureDoseTotal[0];
	const double ctdivol = (ctdiCent + 2.0 * ctdiPher) / 3.0 / static_cast<double>(statCounter);
	const double factor = m_ctdivol / ctdivol / meanWeight;
	m_useXCareFilter = usingXCare; // re-enable organ aec if it was used
	return factor;

}

std::uint64_t CTSource::exposuresPerRotatition() const 
{
	return static_cast<std::size_t>(PI_2 / m_exposureAngleStep);
}




CTSpiralSource::CTSpiralSource()
	:CTSource()
{
	m_type = CTSpiral;
	m_pitch = 1.0;
}
void CTSpiralSource::setPitch(double pitch)
{
	m_pitch = std::max(0.01, pitch);
}

double CTSpiralSource::pitch(void) const
{
	return m_pitch;
}
bool CTSpiralSource::getExposure(Exposure& exposure, std::uint64_t exposureIndex) const
{
	std::array<double, 3> pos = { 0, m_sdd / 2.0,0 };

	const double angle = m_startAngle + m_exposureAngleStep * exposureIndex;

	std::array<double, 3> rotationAxis, beamAxis, otherAxis;
	for (std::size_t i = 0; i < 3; ++i)
	{
		rotationAxis[i] = m_directionCosines[i + 3];
		otherAxis[i] = m_directionCosines[i];
	}
	vectormath::cross(rotationAxis.data(), otherAxis.data(), beamAxis.data());
	vectormath::rotate(pos.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += rotationAxis[i] * (exposureIndex * m_exposureAngleStep) * m_collimation * m_pitch / PI_2;

	vectormath::rotate(otherAxis.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
	{
		pos[i] += m_position[i];
		otherAxis[i] = -otherAxis[i];
	}
	//vectormath::rotate(otherAxis.data(), rotationAxis.data(), angle);


	exposure.setPosition(pos);
	exposure.setDirectionCosines(otherAxis, rotationAxis);
	exposure.setCollimationAngles(std::atan(m_fov / m_sdd) * 2.0, std::atan(m_collimation / (m_sdd / 2.0)) * 2.0);
	exposure.setBeamFilter(m_bowTieFilter.get());
	exposure.setSpecterDistribution(m_specterDistribution.get());
	exposure.setNumberOfHistories(m_historiesPerExposure);
	double weight = 1.0;
	if (m_positionalFilter)
		weight *= m_positionalFilter->sampleIntensityWeight(pos);
	if (m_useXCareFilter)
		weight *= m_xcareFilter.sampleIntensityWeight(angle);
	exposure.setBeamIntensityWeight(weight);
	return true;
}

std::array<double, 3> CTSpiralSource::getExposurePosition(std::uint64_t exposureIndex) const
{

	std::array<double, 3> pos = { 0,m_sdd / 2.0,0 };

	const double angle = m_startAngle + m_exposureAngleStep * exposureIndex;

	std::array<double, 3> rotationAxis, beamAxis, otherAxis;
	for (std::size_t i = 0; i < 3; ++i)
	{
		rotationAxis[i] = m_directionCosines[i + 3];
		otherAxis[i] = m_directionCosines[i];
	}
	vectormath::cross(rotationAxis.data(), otherAxis.data(), beamAxis.data());
	vectormath::rotate(pos.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += rotationAxis[i] * (exposureIndex * m_exposureAngleStep) * m_collimation * m_pitch / PI_2;

	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += m_position[i];
	return pos;
}

std::uint64_t CTSpiralSource::totalExposures() const
{
	return static_cast<std::uint64_t>(m_scanLenght * PI_2 / (m_collimation * m_pitch * m_exposureAngleStep));
}

double CTSpiralSource::getCalibrationValue(std::uint64_t nHistories)
{

	return CTSource::getCalibrationValue(nHistories) * m_pitch;

}




CTAxialSource::CTAxialSource()
	:CTSource()
{
	m_type = CTAxial;
	m_step = m_collimation;
	m_scanLenght = m_step;
}
void CTAxialSource::setStep(double step)
{
	const double absStep = std::abs(step);
	auto n_steps = m_scanLenght / m_step;
	m_step = absStep > 0.01 ? absStep : 0.01;
	setScanLenght(m_step*n_steps);
}

double CTAxialSource::step(void) const
{
	return m_step;
}

void CTAxialSource::setScanLenght(double scanLenght)
{
	auto test = std::floor(scanLenght / m_step);
	m_scanLenght = m_step * std::floor(scanLenght / m_step);
}

bool CTAxialSource::getExposure(Exposure& exposure, std::uint64_t exposureIndex) const
{
	//calculating position
	std::array<double, 3> pos = { 0,m_sdd / 2.0,0 };
	

	const std::uint64_t anglesPerRotation = static_cast<std::uint64_t>(PI_2 / m_exposureAngleStep);
	const std::uint64_t rotationNumber = exposureIndex / anglesPerRotation;


	const double angle = m_startAngle + m_exposureAngleStep * (exposureIndex - (rotationNumber*anglesPerRotation));

	std::array<double, 3> rotationAxis, beamAxis, otherAxis;
	for (std::size_t i = 0; i < 3; ++i)
	{
		rotationAxis[i] = m_directionCosines[i + 3];
		otherAxis[i] = m_directionCosines[i];
	}
	vectormath::cross(rotationAxis.data(), otherAxis.data(), beamAxis.data());
	vectormath::rotate(pos.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += rotationAxis[i] * m_step * rotationNumber;
	vectormath::rotate(otherAxis.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
	{
		pos[i] += m_position[i];
		otherAxis[i] = -otherAxis[i];
	}
	exposure.setPosition(pos);
	exposure.setDirectionCosines(otherAxis, rotationAxis);
	exposure.setCollimationAngles(std::atan(m_fov / m_sdd) * 2.0, std::atan(m_collimation / (m_sdd / 2.0)) * 2.0);
	exposure.setBeamFilter(m_bowTieFilter.get());
	exposure.setSpecterDistribution(m_specterDistribution.get());
	exposure.setNumberOfHistories(m_historiesPerExposure);
	double weight = 1.0;
	if (m_positionalFilter)
		weight *= m_positionalFilter->sampleIntensityWeight(pos);
	if (m_useXCareFilter)
		weight *= m_xcareFilter.sampleIntensityWeight(angle);
	exposure.setBeamIntensityWeight(weight);
	return true;
}

std::array<double, 3> CTAxialSource::getExposurePosition(std::size_t exposureIndex) const
{
	std::array<double, 3> pos = { 0,m_sdd / 2.0,0 };

	const std::uint64_t anglesPerRotation = static_cast<std::uint64_t>(PI_2 / m_exposureAngleStep);
	const std::uint64_t rotationNumber = exposureIndex / anglesPerRotation;
	const double angle = m_startAngle + m_exposureAngleStep * (exposureIndex - (rotationNumber*anglesPerRotation));

	std::array<double, 3> rotationAxis, beamAxis, otherAxis;
	for (std::size_t i = 0; i < 3; ++i)
	{
		rotationAxis[i] = m_directionCosines[i + 3];
		otherAxis[i] = m_directionCosines[i];
	}
	
	vectormath::rotate(pos.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += rotationAxis[i] * m_step * rotationNumber;
	vectormath::rotate(otherAxis.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
	{
		pos[i] += m_position[i];
		otherAxis[i] = -otherAxis[i];
	}

	return pos;
}

std::uint64_t CTAxialSource::totalExposures() const
{
	const std::uint64_t anglesPerRotation = static_cast<std::uint64_t>(PI_2 / m_exposureAngleStep);
	const std::uint64_t rotationNumbers = static_cast<std::uint64_t>(std::round(m_scanLenght / m_step));
	return anglesPerRotation * (rotationNumbers + 1);
}

CTDualSource::CTDualSource()
	:CTSource()
{
	m_type = CTDual;
	
	m_sddB = m_sdd;
	m_fovB = m_fov;
	
	m_startAngleB = m_startAngle;
	
	m_pitch = 1.0;
	m_tube.setAlFiltration(7.0);
	m_tubeB.setAlFiltration(7.0);
}

std::array<double, 3 > CTDualSource::getExposurePosition(std::uint64_t exposureIndexTotal) const
{

	double sdd, startAngle;
	uint64_t exposureIndex = exposureIndexTotal / 2;
	if (exposureIndexTotal % 2 == 0)
	{
		sdd = m_sdd;
		startAngle = m_startAngle;
	}
	else
	{
		sdd = m_sddB;
		startAngle = m_startAngleB;
	}
	std::array<double, 3> pos = { 0,m_sdd / 2.0,0 };
	
	const double angle = startAngle + m_exposureAngleStep * exposureIndex;

	std::array<double, 3> rotationAxis, beamAxis, otherAxis;
	for (std::size_t i = 0; i < 3; ++i)
	{
		rotationAxis[i] = m_directionCosines[i + 3];
		otherAxis[i] = m_directionCosines[i];
	}
	vectormath::cross(rotationAxis.data(), otherAxis.data(), beamAxis.data());
	vectormath::rotate(pos.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += rotationAxis[i] * (exposureIndex * m_exposureAngleStep) * m_collimation * m_pitch / PI_2;
	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += m_position[i];
	return pos;
}

bool CTDualSource::getExposure(Exposure& exposure, std::uint64_t exposureIndexTotal) const
{
	double sdd, startAngle, fov;
	uint64_t exposureIndex = exposureIndexTotal / 2;
	BeamFilter* bowTie=nullptr;
	SpecterDistribution* specterDistribution=nullptr;
	if (exposureIndexTotal % 2 == 0)
	{
		sdd = m_sdd;
		startAngle = m_startAngle;
		fov = m_fov;
		bowTie = m_bowTieFilter.get();
		specterDistribution = m_specterDistribution.get();
	}
	else
	{
		sdd = m_sddB;
		startAngle = m_startAngleB;
		fov = m_fovB;
		bowTie = m_bowTieFilterB.get();
		specterDistribution = m_specterDistributionB.get();
	}
	std::array<double, 3> pos = { 0,m_sdd / 2.0,0 };

	const double angle = startAngle + m_exposureAngleStep * exposureIndex;

	std::array<double, 3> rotationAxis, beamAxis, otherAxis;
	for (std::size_t i = 0; i < 3; ++i)
	{
		rotationAxis[i] = m_directionCosines[i + 3];
		otherAxis[i] = m_directionCosines[i];
	}
	vectormath::cross(rotationAxis.data(), otherAxis.data(), beamAxis.data());
	vectormath::rotate(pos.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
		pos[i] += rotationAxis[i] * (exposureIndex * m_exposureAngleStep) * m_collimation * m_pitch / PI_2;
	
	vectormath::rotate(otherAxis.data(), rotationAxis.data(), angle);
	for (std::size_t i = 0; i < 3; ++i)
	{
		pos[i] += m_position[i];
		otherAxis[i] = -otherAxis[i];
	}

	exposure.setPosition(pos);
	exposure.setDirectionCosines(otherAxis, rotationAxis);
	exposure.setCollimationAngles(std::atan(fov / sdd) * 2.0, std::atan(m_collimation / (sdd / 2.0)) * 2.0);
	exposure.setBeamFilter(bowTie);
	exposure.setSpecterDistribution(specterDistribution);
	exposure.setNumberOfHistories(m_historiesPerExposure);
	double weight = 1.0;
	if (m_positionalFilter)
		weight *= m_positionalFilter->sampleIntensityWeight(pos);
	if (m_useXCareFilter)
		weight *= m_xcareFilter.sampleIntensityWeight(angle);
	exposure.setBeamIntensityWeight(weight);
	return true;
}

std::uint64_t CTDualSource::totalExposures(void) const
{

	auto singleSourceExposure =  static_cast<std::uint64_t>(m_scanLenght * PI_2 / (m_collimation * m_pitch * m_exposureAngleStep));
	return singleSourceExposure * 2;
}

std::uint64_t CTDualSource::exposuresPerRotatition() const
{
	return 2 * static_cast<std::size_t>(PI_2 / m_exposureAngleStep);
}


double CTDualSource::getCalibrationValue(std::uint64_t nHistories)
{
	double meanWeight = 0;
	for (std::size_t i = 0; i < totalExposures(); ++i)
	{
		Exposure dummy;
		getExposure(dummy, i);
		meanWeight += dummy.beamIntensityWeight();
	}
	meanWeight /= static_cast<double>(totalExposures());

	auto world = CTDIPhantom(m_ctdiPhantomDiameter);
	world.setAttenuationLutMaxEnergy(maxPhotonEnergyProduced());
	world.validate();

	updateFromWorld(world);
	validate();

	std::array<double, 5> measureDoseTotal;
	measureDoseTotal.fill(0.0);
	std::array<CTDIPhantom::HolePosition, 5> position = { CTDIPhantom::Center, CTDIPhantom::West, CTDIPhantom::East, CTDIPhantom::South, CTDIPhantom::North };

	auto spacing = world.spacing();
	auto dim = world.dimensions();

	const double voxelVolume = spacing[0] * spacing[1] * spacing[2] / 1000.0; // cm3
	const double voxelMass = world.airDensity() * voxelVolume / 1000.0; //kg

	bool usingXCare = m_useXCareFilter;
	m_useXCareFilter = false; // we need to disable organ aec for ctdi statistics, this should be ok 

	std::size_t statCounter = 0;
	do {
		std::array<double, 5> measureDose;
		measureDose.fill(0.0);
		auto dose = transport::run(world, this);
		for (std::size_t i = 0; i < 5; ++i)
		{
			auto holeIndices = world.holeIndices(position[i]);
			for (auto idx : holeIndices)
				measureDose[i] += dose[idx];
			measureDose[i] /= static_cast<double>(holeIndices.size());
			measureDoseTotal[i] += measureDose[i];
		}
		++statCounter;
	} while ((ctdiStatIndex(measureDoseTotal) > 0.05) && (statCounter < 20)); // we allow 5% normal error in ctdi dose calibration

	const double ctdiPher = (measureDoseTotal[1] + measureDoseTotal[2] + measureDoseTotal[3] + measureDoseTotal[4]) / 4.0;
	const double ctdiCent = measureDoseTotal[0];
	const double ctdivol = (ctdiCent + 2.0 * ctdiPher) / 3.0 / m_pitch / static_cast<double>(statCounter);
	const double factor = m_ctdivol / ctdivol / meanWeight;
	m_useXCareFilter = usingXCare; // re-enable organ aec if it was used
	return factor;
}


void CTDualSource::setStartAngleDegB(double angle)
{
	m_startAngleB = DEG_TO_RAD * angle;
}

double CTDualSource::startAngleDegB(void) const
{
	return RAD_TO_DEG * m_startAngleB;
}

void CTDualSource::setPitch(double pitch)
{
	m_pitch = std::max(0.01, pitch);
}

double CTDualSource::pitch(void) const
{
	return m_pitch;
}


void CTDualSource::updateSpecterDistribution()
{
	if (!m_specterValid)
	{
		auto energyA = m_tube.getEnergy();
		auto energyB = m_tubeB.getEnergy();
		auto specterA = m_tube.getSpecter(energyA, false);
		auto specterB = m_tubeB.getSpecter(energyB, false);

		double sumA = std::accumulate(specterA.begin(), specterA.end(), 0.0);
		double sumB = std::accumulate(specterB.begin(), specterB.end(), 0.0);
		double weightA = m_tubeAmas * sumA;
		double weightB = m_tubeBmas * sumB;

		std::for_each(specterA.begin(), specterA.end(), [=](double& val) {val /= sumA; });
		std::for_each(specterB.begin(), specterB.end(), [=](double& val) {val /= sumB; });

		m_tubeAweight = 1.0;
		m_tubeBweight = weightB / weightA;

		m_specterDistribution = std::make_unique<SpecterDistribution>(specterA, energyA);
		m_specterDistributionB = std::make_unique<SpecterDistribution>(specterB, energyB);
		m_specterValid = true;
	}
}
