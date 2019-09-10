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

#include "material.h"
#include "tube.h"
#include "attenuationlut.h"

#include <algorithm>
#include <vector>
#include <array>
#include <map>
#include <string>
#include <memory>

class World {
public:
    World();
	void setDimensions(std::size_t x, std::size_t y, std::size_t z);
	void setDimensions(const std::array<std::size_t, 3>& dimensions);
	void setSpacing(double dx, double dy, double dz);
	void setSpacing(double spacing[3]);
	void setSpacing(const std::array<double, 3>& spacing);
	void setOrigin(double x, double y, double z);
	void setOrigin(double origin[3]);
	void setOrigin(const std::array<double, 3>& origin);
	void setDirectionCosines(double cosines[6]);
	void setDirectionCosines(const std::array<double, 6>& cosines);
	void setDirectionCosines(double x1, double x2, double x3, double y1, double y2, double y3);

    std::size_t size(void) const {return m_dimensions[0] * m_dimensions[1] * m_dimensions[2];}
    const std::array<std::size_t, 3>& dimensions(void) const {return m_dimensions;}
    const std::array<double, 3>& spacing(void) const {return m_spacing;}
	const std::array<double, 3>& origin(void) const {return m_origin;}
	const std::array<double, 6>& directionCosines(void) const {return m_directionCosines;}
	const std::array<double, 3>& depthDirection(void) const {return m_depthDirectionCosine;}

	void setDensityArray(std::shared_ptr<std::vector<double>> densityArray);
	const double* densityBuffer() const { return m_density->data(); }
    std::shared_ptr<std::vector<double>> densityArray(void) {return m_density;}
	const std::shared_ptr<std::vector<double>> densityArray(void) const { return m_density; }

	void setMaterialIndexArray(std::shared_ptr<std::vector<unsigned char>> materialIndex);
	const unsigned char* materialIndexBuffer() const { return m_materialIndex->data(); }
	std::shared_ptr<std::vector<unsigned char>> materialIndexArray(void){return m_materialIndex;}
    const std::shared_ptr<std::vector<unsigned char>> materialIndexArray(void) const {return m_materialIndex;}

    const std::vector<Material>& materialMap(void) const {return m_materialMap;}
    bool addMaterialToMap(const Material& material);
	bool addMaterialToMap(Material&& material);
    void clearMaterialMap(void){m_materialMap.clear();}

	const AttenuationLut& attenuationLut() const { return m_attLut; }

	void setAttenuationLutMaxEnergy(double max_keV);
	double attenuationLutMaxEnergy(void) const { return m_attenuationLutMaxEnergy; }

    const std::array<double, 6>& matrixExtent(void) const {return m_worldExtent;}

    bool isValid(void) const { return m_valid; }
	bool validate(void);

protected:
    void updateWorldMatrixExtent(void);

private:
	bool m_valid = false;

    std::array<double, 3> m_spacing = { 1.0, 1.0, 1.0 };
	std::array<double, 3> m_origin = { 0.0, 0.0, 0.0 };
	std::array<double, 6> m_directionCosines = { 1.0 , 0.0, 0.0, 0.0, 1.0, 0.0 };
	std::array<double, 3> m_depthDirectionCosine = {0.0, 0.0, 1.0};
	std::array<std::size_t, 3> m_dimensions = { 0, 0, 0 };

	std::array<double, 6> m_worldExtent; // {x0, x1, y0, y1, z0, z1}

	// m_density and m_materialIndex must outlive member variables of this class, i.e shared pointers
    std::shared_ptr<std::vector<double>> m_density;
    std::shared_ptr<std::vector<unsigned char>> m_materialIndex;
    
	std::vector<Material> m_materialMap;
	AttenuationLut m_attLut;
   
	double m_attenuationLutMaxEnergy = 150.0;
};

class CTDIPhantom : public World
{
public:
	enum HolePosition {Center, West, East, South, North};
	CTDIPhantom(std::size_t size = 320); // size im mm
	const std::vector<std::size_t>& holeIndices(HolePosition position = Center);
	double airDensity() { return m_airDensity; }
private:
	std::array<std::vector<std::size_t>, 5> m_holePositions;
	double m_airDensity = 0.0;
};