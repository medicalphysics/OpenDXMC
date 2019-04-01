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

#include "world.h"
#include "vectormath.h"
#include "material.h"
#include <algorithm>
#include <future>
#include <execution>

World::World()
{
	vectormath::cross(m_directionCosines.data(), m_depthDirectionCosine.data());
    updateWorldMatrixExtent();
	m_attenuationLutMaxEnergy = Tube::maxVoltage();
}

void World::updateWorldMatrixExtent(void)
{
    //compute world extent
    for (std::size_t i = 0;i < 3; i++)
    {
		auto halfDist = (m_dimensions[i] * m_spacing[i]) * 0.5;
		m_worldExtent[i * 2] = m_origin[i] - halfDist;
        m_worldExtent[i * 2 + 1] = m_origin[i] + halfDist;
    }
}

void World::setDimensions(std::size_t x, std::size_t y, std::size_t z)
{   
	m_dimensions[0] = x;
	m_dimensions[1] = y;
	m_dimensions[2] = z;   
    updateWorldMatrixExtent();

	m_valid = false;
}

void World::setDimensions(const std::array<std::size_t, 3>& dimensions)
{
	m_dimensions = dimensions;
	updateWorldMatrixExtent();
	m_valid = false;
}

void World::setSpacing(double dx, double dy, double dz)
{
	m_spacing[0] = dx;
	m_spacing[1] = dy;
	m_spacing[2] = dz;
    updateWorldMatrixExtent();
}

void World::setSpacing(double spacing[3])
{
    setSpacing(spacing[0], spacing[1], spacing[2]);
}
void World::setSpacing(const std::array<double, 3> &spacing)
{
	setSpacing(spacing[0], spacing[1], spacing[2]);
}
void World::setOrigin(double x, double y, double z)
{
	m_origin[0] = x;
	m_origin[1] = y;
	m_origin[2] = z;
    updateWorldMatrixExtent();
}

void World::setOrigin(double position[3])
{
    setOrigin(position[0], position[1], position[2]);
}

void World::setOrigin(const std::array<double, 3> &position)
{
	setOrigin(position[0], position[1], position[2]);
}


void World::setDirectionCosines(const std::array<double, 6>& cosines)
{
	setDirectionCosines(
		cosines[0],
		cosines[1],
		cosines[2],
		cosines[3],
		cosines[4],
		cosines[5]
	);
}
void World::setDirectionCosines(double cosines[6])
{
    setDirectionCosines(
        cosines[0],
        cosines[1],
        cosines[2],
        cosines[3],
        cosines[4],
        cosines[5]
    );
}
void World::setDirectionCosines(double x1, double x2, double x3, double y1, double y2, double y3)
{
	m_directionCosines[0] = x1;
	m_directionCosines[1] = x2;
	m_directionCosines[2] = x3;
	m_directionCosines[3] = y1;
	m_directionCosines[4] = y2;
	m_directionCosines[5] = y3;
	vectormath::normalize(m_directionCosines.data());
	vectormath::normalize(&m_directionCosines[3]);
	vectormath::cross(m_directionCosines.data(), m_depthDirectionCosine.data());
    updateWorldMatrixExtent();
}


bool World::addMaterialToMap(const Material& material)
{
    if(material.isValid())
    {
        m_materialMap.push_back(material);
		m_valid = false;
        return true;
    }
    return false;
}
bool World::addMaterialToMap(Material&& material)
{
	if (material.isValid())
	{
		m_materialMap.push_back(material);
		m_valid = false;
		return true;
	}
	return false;
}

void World::setDensityArray(std::shared_ptr<std::vector<double>> densityArray)
{
	m_density = densityArray;
	m_valid = false;
}

void World::setMaterialIndexArray(std::shared_ptr<std::vector<unsigned char>> materialIndexArray)
{
	m_materialIndex = materialIndexArray;
	m_valid = false;
}


void World::setAttenuationLutMaxEnergy(double max_keV)
{
	if (max_keV == m_attenuationLutMaxEnergy) // so we dont invalidate the world in vain
		return;

	if (max_keV < Tube::minVoltage())
		m_attenuationLutMaxEnergy = Tube::minVoltage();
	else if (max_keV > Tube::maxVoltage())
		m_attenuationLutMaxEnergy = Tube::maxVoltage();
	else
		m_attenuationLutMaxEnergy = max_keV;
	m_valid = false;
}

template<typename It, typename Value>
bool testMaterialIndex(It beg, It end, const std::vector<Value>& materialMap)
{// validating that all material indices are present in the materialMap
	auto[min, max] = std::minmax_element(std::execution::par_unseq, beg, end);
	auto n = materialMap.size();
	return (*min >= 0) && (*min < n) && (*max >= 0) && (*max < n);
}

bool World::validate()
{
	if (m_valid)
		return m_valid;

	if ((m_dimensions[0] * m_dimensions[1] * m_dimensions[2]) <= 0)
	{
		m_valid = false;
		return m_valid;
	}

	if ((m_spacing[0] * m_spacing[1] * m_spacing[2]) <= 0.0)
	{
		m_valid = false;
		return m_valid;
	}

	if (!m_density | !m_materialIndex)
	{
		m_valid = false;
		return m_valid;
	}

	auto elements = size();
	if (elements == 0)
	{
		m_valid = false;
		return m_valid;
	}

    if(m_density->size() != elements)
	{
		m_valid = false;
		return m_valid;
	}

    if (m_materialIndex->size() != elements)
	{
		m_valid = false;
		return m_valid;
	}

    //testValidMaterials
    for (auto const & mat : m_materialMap)
    {
        bool validMaterial = mat.isValid();
        if (!validMaterial)
		{
			m_valid = false;
			return m_valid;
		}
    }

    double testOrtogonality = vectormath::dot(m_depthDirectionCosine.data(), m_directionCosines.data()) +
            vectormath::dot(&m_depthDirectionCosine[0], &m_directionCosines[3]) +
            vectormath::dot(&m_directionCosines[0], &m_directionCosines[3]);

    if (std::abs(testOrtogonality) > 0.00001)
	{
		m_valid = false;
		return m_valid;
	}

    m_valid = testMaterialIndex(m_materialIndex->begin(), m_materialIndex->end(), m_materialMap);
    if (m_valid)
    {
		m_attLut.generate(m_materialMap, 1.0, m_attenuationLutMaxEnergy);
		m_attLut.generateMaxMassTotalAttenuation(m_materialIndex->begin(), m_materialIndex->end(), m_density->begin());
    }
	return m_valid;
}


std::vector<std::size_t> circleIndices2D(const std::array<std::size_t, 2>& dim, const std::array<double, 2>& spacing, const std::array<double, 2>& center, double radius)
{
	std::vector<std::size_t> indices;
	std::array<int, 4> flimits;
	//int flimits[4];
	flimits[0] = static_cast<int>((center[0] - radius) / spacing[0]);
	flimits[1] = static_cast<int>((center[0] + radius) / spacing[0]) + 1;
	flimits[2] = static_cast<int>((center[1] - radius) / spacing[1]);
	flimits[3] = static_cast<int>((center[1] + radius) / spacing[1]) + 1;
	std::array<std::size_t, 4> limits;
	//std::size_t limits[4];
	limits[0] = static_cast<std::size_t>(std::max(flimits[0], 0));
	limits[1] = static_cast<std::size_t>(std::min(flimits[1], static_cast<int>(dim[0])));
	limits[2] = static_cast<std::size_t>(std::max(flimits[2], 0));
	limits[3] = static_cast<std::size_t>(std::min(flimits[3], static_cast<int>(dim[1])));


	const double r2 = radius * radius;
	for (std::size_t i = limits[0]; i < limits[1]; ++i)
	{
		const double posX = center[0] - i * spacing[0];
		for (std::size_t j = limits[2]; j < limits[3]; ++j)
		{
			const double posY = center[1] - j * spacing[1];
			if ((posX * posX + posY * posY) <= r2)
				indices.push_back(i + j * dim[0]);
		}
	}
	return indices;
}

CTDIPhantom::CTDIPhantom(std::size_t diameter)
	:World()
{
	setDirectionCosines(1.0, 0.0, 0.0, 0.0, 1.0, 0.0);
	setOrigin(0.0, 0.0, 0.0);
	setSpacing(1.0, 1.0, 25.0);
	setDimensions(diameter+3, diameter+3, 6);
	

	auto air = Material("Air, Dry (near sea level)");
	auto pmma = Material("Polymethyl Methacralate (Lucite, Perspex)");
	addMaterialToMap(air);
	addMaterialToMap(pmma);
	m_airDensity = air.standardDensity();


	constexpr double holeDiameter = 13.1;
	constexpr double holeRadii = holeDiameter / 2.0;
	constexpr double holeDisplacement = 10.0;
	const double radii = static_cast<double>(diameter) / 2.0;
	const double radiiSqr = radii * radii;
	const auto dim = dimensions();
	const auto sp = spacing();
	const auto ex = matrixExtent();
	//making phantom
	auto dBufferPtr = std::make_shared<std::vector<double>>(size());
	auto mBufferPtr = std::make_shared<std::vector<unsigned char>>(size());
	setDensityArray(dBufferPtr);
	setMaterialIndexArray(mBufferPtr);
	
	auto dBuffer = dBufferPtr->data();
	auto mBuffer = mBufferPtr->data();
	//filling with air

	for (std::size_t i = 0; i < size(); ++i)
	{
		dBuffer[i] = m_airDensity;
		mBuffer[i] = 0;
	}

	//air holes indices
	std::array<std::size_t, 2> fdim = { dim[0], dim[1] };
	std::array<double, 2> fspacing = { sp[0], sp[1] };
	std::array<double, 2> fcenter = { dim[0] * sp[0] * 0.5, dim[1] * sp[1] * 0.5 };

	std::array<std::array<double, 2>, 5> hcenters;
	hcenters[0][0] = 0.0; hcenters[0][1] = 0.0;
	hcenters[1][0] = 0.0; hcenters[1][1] = radii - holeDisplacement;
	hcenters[2][0] = 0.0; hcenters[2][1] = -radii + holeDisplacement;
	hcenters[3][0] = radii - holeDisplacement; hcenters[3][1] = 0.0;
	hcenters[4][0] = -radii + holeDisplacement; hcenters[4][1] = 0.0;
	for (auto& cvec : hcenters)
		for (std::size_t i = 0; i < 2; ++i)
			cvec[i] += fcenter[i];
	
	//setting up measurement indices
	for (std::size_t k = 1; k < 5; ++k) // from 2.5 to 12.5 cm into the phantom
	{
		const std::size_t offset = k * dim[0] * dim[1];
		for (std::size_t i = 0; i < 5; ++i)
		{
			auto idx = circleIndices2D(fdim, fspacing, hcenters[i], holeRadii);
			std::transform(idx.begin(), idx.end(), idx.begin(), [=](std::size_t ind) {return ind + offset; }); //adding offset
			auto &indices = m_holePositions[i];
			indices.insert(indices.end(), idx.begin(), idx.end());
		}
	}

	//making phantom
	for (std::size_t k = 0; k < dim[2]; ++k)
	{
		const std::size_t offset = k * dim[0] * dim[1];
		auto indices = circleIndices2D(fdim, fspacing, fcenter, radii);
		for (auto idx : indices)
		{
			dBuffer[idx + offset] = pmma.standardDensity();
			mBuffer[idx + offset] = 1;
		}

		//air holes
		for (std::size_t i = 0; i < 5; ++i)
		{
			auto indicesHoles = circleIndices2D(fdim, fspacing, hcenters[i], holeRadii);
			for (auto idx : indicesHoles)
			{
				dBuffer[idx + offset] = air.standardDensity();
				mBuffer[idx + offset] = 0;
			}
		}
	}
	validate();
}

const std::vector<std::size_t>& CTDIPhantom::holeIndices(HolePosition position)
{
	if (position == West)
		return m_holePositions[4];
	else if (position == East)
		return m_holePositions[3];
	else if (position == North)
		return m_holePositions[2];
	else if (position == South)
		return m_holePositions[1];
	return m_holePositions[0];
}

