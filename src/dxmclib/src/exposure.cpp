
#include "exposure.h"
#include "vectormath.h"
Exposure::Exposure(const BeamFilter* filter, const SpecterDistribution* specter)
{
	for (std::size_t i = 0; i < 3; ++i)
	{
		m_position[i] = 0.0;
		m_directionCosines[i] = 0.0;
		m_directionCosines[i + 3] = 0.0;
	}
	m_directionCosines[0] = 1.0;
	m_directionCosines[5] = 1.0;
	calculateBeamDirection();
	m_collimation = Rectangular;
	m_collimationAngles[0] = 0.35;  // about 20 deg
	m_collimationAngles[1] = 0.35;
	m_beamIntensityWeight = 1.0;
	m_specterDistribution = specter;
	m_beamFilter = filter;
}


void Exposure::setPosition(double x, double y, double z)
{
	m_position[0] = x;
	m_position[1] = y;
	m_position[2] = z;
}


void Exposure::setPosition(const double pos[3])
{
	for (std::size_t i = 0; i < 3; i++)
	{
		m_position[i] = pos[i];
	}
}


const std::array<double, 3>& Exposure::position(void) const
{
	return m_position;
}

void Exposure::setDirectionCosines(double x1, double x2, double x3, double y1, double y2, double y3)
{
	m_directionCosines[0] = x1;
	m_directionCosines[1] = x2;
	m_directionCosines[2] = x3;
	m_directionCosines[3] = y1;
	m_directionCosines[4] = y2;
	m_directionCosines[5] = y3;
	normalizeDirectionCosines();
}

void Exposure::setDirectionCosines(const double cosines[6])
{
	for (std::size_t i = 0; i < 6; i++)
	{
		m_directionCosines[i] = cosines[i];
	}
	normalizeDirectionCosines();
}

void Exposure::setDirectionCosines(const std::array<double, 6>& cosines)
{
	m_directionCosines = cosines;
	normalizeDirectionCosines();
}

void Exposure::setDirectionCosines(const std::array<double, 3>& cosinesX, const std::array<double, 3>& cosinesY) 
{
	for (std::size_t i = 0; i < 3; i++)
	{
		m_directionCosines[i] = cosinesX[i];
		m_directionCosines[i + 3] = cosinesY[i];
	}
	normalizeDirectionCosines();
}


void Exposure::normalizeDirectionCosines()
{
	double sumx = 0;
	double sumy = 0;
	for (std::size_t i = 0; i < 3; i++)
	{
		sumx += m_directionCosines[i] * m_directionCosines[i];
		sumy += m_directionCosines[i + 3] * m_directionCosines[i + 3];
	}

	sumx = 1.0 / std::sqrt(sumx);
	sumy = 1.0 / std::sqrt(sumy);

	for (std::size_t i = 0; i < 3; i++)
	{
		m_directionCosines[i] *= sumx;
		m_directionCosines[i + 3] *= sumy;
	}

	calculateBeamDirection();
}

void Exposure::calculateBeamDirection()
{
    vectormath::cross(m_directionCosines.data(), m_beamDirection.data());
}


Exposure::CollimationType Exposure::collimationType(void)
{
	return m_collimation;
}

void Exposure::setCollimationType(Exposure::CollimationType type)
{
	m_collimation = type;
}

void Exposure::setCollimationAngles(const double angles[2])
{
	m_collimationAngles[0] = angles[0];
	m_collimationAngles[1] = angles[1];
}
void Exposure::setCollimationAngles(const std::array<double, 2>& angles)
{
	m_collimationAngles = angles;
}
void Exposure::setCollimationAngles(const double angleX, const double angleY)
{
	m_collimationAngles[0] = angleX;
	m_collimationAngles[1] = angleY;
}

double Exposure::collimationAngleX()
{
	return m_collimationAngles[0];
}
double Exposure::collimationAngleY()
{
	return m_collimationAngles[1];
}

void Exposure::setBeamFilter(const BeamFilter* filter)
{
	m_beamFilter = filter;
}

void Exposure::setSpecterDistribution(const SpecterDistribution* specter)
{
	m_specterDistribution = specter;
}
void Exposure::setMonoenergeticPhotonEnergy(double energy)
{
	if (energy > 500.0)
		energy = 500.0;
	if (energy < 0.0)
		energy = 0.0;
	m_monoenergeticPhotonEnergy = energy;
}
void Exposure::alignToDirectionCosines(const std::array<double, 6>& directionCosines)
// aligning coordinates to new basis given by cosines
{
	const double* b1 = directionCosines.data();
	const double* b2 = &b1[3];
	double b3[3];
	vectormath::cross(b1, b2, b3);
	vectormath::changeBasisInverse(b1, b2, b3, m_position.data());
	vectormath::changeBasisInverse(b1, b2, b3, m_directionCosines.data());
	vectormath::changeBasisInverse(b1, b2, b3, &m_directionCosines[3]);
	vectormath::changeBasisInverse(b1, b2, b3, m_beamDirection.data());
}

void Exposure::sampleParticle(Particle& p, std::uint64_t seed[2]) const
{

	p.pos[0] = m_position[0];
	p.pos[1] = m_position[1];
	p.pos[2] = m_position[2];


	// particle direction

	const double theta = randomUniform(seed, -m_collimationAngles[0] / 2.0, m_collimationAngles[0] / 2.0);
	const double phi = randomUniform(seed, -m_collimationAngles[1] / 2.0, m_collimationAngles[1] / 2.0);
	const double sintheta = std::sin(theta);
	const double sinphi = std::sin(phi);
	const double sin2theta = sintheta * sintheta;
	const double sin2phi = sinphi * sinphi;
	const double norm = 1.0 / std::sqrt(1.0 + sin2phi + sin2theta);
	for (std::size_t i = 0; i < 3; i++)
	{
		p.dir[i] = norm * (m_beamDirection[i] + sintheta * m_directionCosines[i] + sinphi * m_directionCosines[i + 3]);
	}
	
	if (m_specterDistribution)
	{
		p.energy = m_specterDistribution->sampleValue(seed);
	}
	else 
	{
		p.energy = m_monoenergeticPhotonEnergy;
	}
	
	p.weight = m_beamIntensityWeight;
	if (m_beamFilter)
	{
		p.weight *= m_beamFilter->sampleIntensityWeight(theta);
	}
}
