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

#include "beamfilters.h"
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

constexpr double PI = 3.14159265359;
constexpr double PI_2 = PI + PI;
constexpr double DEG_TO_RAD = PI / 180.0;
constexpr double RAD_TO_DEG = 180.0 / PI;

BowTieFilter::BowTieFilter(const std::vector<double>& angles, const std::vector<double>& weights)
	:BeamFilter()
{
    if (angles.size() == weights.size())
    {
        m_data.resize(angles.size());
        for (std::size_t i = 0; i < angles.size(); i++)
        {
            m_data[i].first = std::abs(angles[i]);
            m_data[i].second = weights[i];
        }
    }
    std::sort(m_data.begin(), m_data.end());
	normalizeData();
}

BowTieFilter::BowTieFilter(const std::vector<std::pair<double, double> > & angleWeightsPairs)
    :BeamFilter(), m_data(angleWeightsPairs)
{
    for (auto& p : m_data)
    {
        p.first = std::abs(p.first);
    }
    std::sort(m_data.begin(), m_data.end());
	normalizeData();
}

BowTieFilter::BowTieFilter(const BowTieFilter& other)
	:BeamFilter()
{
	m_data = other.data();
}

double BowTieFilter::sampleIntensityWeight(const double anglePlusAndMinus) const
{

	const double angle = std::abs(anglePlusAndMinus);

    auto first = m_data.begin();
    auto last = m_data.end();
    std::advance(last, -1);

    //binary search for angle
    auto mid = std::distance(first, last) / 2;
    auto it = first;
    std::advance(it, mid);
    while (it != first)
    {

        if (angle < it->first)
        {
            last = it;
            it = first;
        }
        else
        {
            first = it;
        }
        mid = std::distance(first, last) / 2;
        std::advance(it, mid);
    }
    // end search


    if (angle < first->first)
    {
        return m_data[0].second;
    }
    if (angle > last->first)
    {
        return m_data.back().second;
    }

    //linear interpolation

    const double x0 = first->first;
    const double x1 = last->first;
    const double y0 = first->second;
    const double y1 = last->second;

    return y0 + (angle - x0) * (y1 - y0) / (x1 - x0);
}

void BowTieFilter::normalizeData()
{

	auto max = std::max_element(m_data.begin(), m_data.end(), [=](const auto &a, const auto &b) {return a.second < b.second; });
	const auto maxVal = (*max).second;
	std::transform(m_data.begin(), m_data.end(), m_data.begin(), [=](const auto &el) {return std::make_pair(el.first, el.second / maxVal); });
	
}


XCareFilter::XCareFilter()
	:BeamFilter()
{
	m_filterAngle = 180.0 * DEG_TO_RAD;
	m_spanAngle = 120.0 * DEG_TO_RAD;
	m_rampAngle = 20.0 * DEG_TO_RAD;
	m_lowWeight = 0.6;
}

double XCareFilter::filterAngleDeg() const
{
	return m_filterAngle * RAD_TO_DEG;
}

void XCareFilter::setFilterAngle(double angle)
{
	m_filterAngle = std::fmod(angle, PI_2);
	if (m_filterAngle < 0.0)
		m_filterAngle += PI_2;
}

void XCareFilter::setFilterAngleDeg(double angle)
{
	setFilterAngle(angle*DEG_TO_RAD);
}

double XCareFilter::spanAngleDeg() const
{
	return m_spanAngle * RAD_TO_DEG;
}
void XCareFilter::setSpanAngle(double angle)
{
	constexpr double smallestDegree = 5.0 * DEG_TO_RAD;
	if ((angle > smallestDegree) && (angle < PI))
	{
		m_spanAngle = angle;
	}
}

void XCareFilter::setSpanAngleDeg(double angle)
{
	setSpanAngle(angle * DEG_TO_RAD);
}

double XCareFilter::rampAngleDeg() const
{
	return m_rampAngle * RAD_TO_DEG;
}

void XCareFilter::setRampAngle(double angle)
{
	if ((angle >= 0.0) && (angle <= 0.5*m_spanAngle))
	{
		m_rampAngle = angle;
	}
}

void XCareFilter::setRampAngleDeg(double angle)
{
	setRampAngle(angle * DEG_TO_RAD);
}


void XCareFilter::setLowWeight(double weight)
{
	if ((weight > 0.0) && (weight <= 1.0))
	{
		m_lowWeight = weight;
	}
}

double XCareFilter::highWeight() const
{
	return (PI_2 - m_spanAngle * m_lowWeight + m_lowWeight * m_rampAngle) / (PI_2 - m_spanAngle + m_rampAngle);
}

template<typename T>
T interp(T x0, T x1, T y0, T y1, T x)
{
	return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

double XCareFilter::sampleIntensityWeight(const double angle) const
{
	double angleMod = std::fmod(angle - m_filterAngle + PI, PI_2); // centering angle on 180 degrees
	if (angleMod < 0.0)
		angleMod += PI_2;

	const double high = highWeight();
	
	const double startFilter = PI - m_spanAngle * 0.5;
	if (angleMod < startFilter)
		return high;

	const double endRamp1 = startFilter + m_rampAngle;
	if (angleMod < endRamp1)
		return interp(startFilter, endRamp1, high, m_lowWeight, angleMod);

	const double startRamp2 = endRamp1 + m_spanAngle - m_rampAngle;
	if (angleMod < startRamp2)
		return m_lowWeight;

	const double endramp2 = startFilter + m_spanAngle;
	if (angleMod < endramp2)
		return interp(startRamp2, endramp2, m_lowWeight, high, angleMod);
	
	return high;
}

AECFilter::AECFilter(const std::vector<double>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposure)
	:PositionalFilter()
{
	generateDensityWeightMap(densityImage, spacing, dimensions, exposure);
	std::array<double, 3> origin{ 0,0,0 };
	setCurrentDensityImage(densityImage, spacing, dimensions, origin);
}

AECFilter::AECFilter(std::shared_ptr<std::vector<double>>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposure)
	:PositionalFilter()
{
	generateDensityWeightMap(densityImage, spacing, dimensions, exposure);
	std::array<double, 3> origin{ 0,0,0 };
	setCurrentDensityImage(densityImage, spacing, dimensions, origin);
}

void AECFilter::updateFromWorld(const World& world)
{
	auto dens = world.densityArray();
	auto spacing = world.spacing();
	auto dim = world.dimensions();
	auto origin = world.origin();
	setCurrentDensityImage(dens, spacing, dim, origin);

}

void AECFilter::setCurrentDensityImage(const std::vector<double>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::array<double, 3> &origin)
{
	std::size_t size = dimensions[0] * dimensions[1] * dimensions[2];
	if (densityImage.size() != size)
		return;
	m_positionWeightMap.clear();
	m_positionWeightMap.reserve(dimensions[2]);
	const double voxelVolume = spacing[0] * spacing[1] * spacing[2];
	const std::size_t step = dimensions[0] * dimensions[1];
	for (std::size_t k = 0; k < dimensions[2]; ++k)
	{
		const std::size_t offset = step * k;
		auto beg = densityImage.begin();
		auto end = densityImage.begin();
		std::advance(beg, offset);
		std::advance(end, offset + step);
		const double mass = std::accumulate(beg, end, 0.0) * voxelVolume;
		const double position = origin[2] - spacing[2] * dimensions[2] * 0.5 + spacing[2] * k;
		const double weight = interpolateMassWeight(mass);
		m_positionWeightMap.push_back(std::make_pair(position, weight));
	}
	m_valid = true;
}
void AECFilter::setCurrentDensityImage(std::shared_ptr<std::vector<double>>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::array<double, 3> &origin)
{
	std::size_t size = dimensions[0] * dimensions[1] * dimensions[2];
	if (densityImage->size() != size)
		return;
	m_positionWeightMap.clear();
	m_positionWeightMap.reserve(dimensions[2]);
	const double voxelVolume = spacing[0] * spacing[1] * spacing[2];
	const std::size_t step = dimensions[0] * dimensions[1];
	for (std::size_t k = 0; k < dimensions[2]; ++k)
	{
		const std::size_t offset = step * k;
		auto beg = densityImage->begin();
		auto end = densityImage->begin();
		std::advance(beg, offset);
		std::advance(end, offset + step);
		const double mass = std::accumulate(beg, end, 0.0) * voxelVolume;
		const double position = origin[2] - spacing[2] * dimensions[2] * 0.5 + spacing[2] * k;
		const double weight = interpolateMassWeight(mass);
		m_positionWeightMap.push_back(std::make_pair(position, weight));
	}
	m_valid = true;
}



double AECFilter::sampleIntensityWeight(const std::array<double, 3>& position) const
{
	auto it = std::upper_bound(m_positionWeightMap.begin(), m_positionWeightMap.end(), std::make_pair(position[2],0.0), [=](const auto& left, const auto& right) {return left.first < right.first; });
	if (it == m_positionWeightMap.end())
		return m_positionWeightMap.back().second;
	if (it == m_positionWeightMap.begin())
		return m_positionWeightMap.front().second;
	auto it0 = it;
	std::advance(it0, -1);
	auto p1 = *it;
	auto p0 = *it0;
	return p0.second + (p1.second - p0.second) * (position[2] - p0.first) / (p1.first - p0.first);
}


inline double arraySubIndex(const std::vector<double> arr, std::size_t otherIndex, std::size_t otherDim)
{
	auto t = static_cast<double> (otherIndex) / otherDim;
	auto index = static_cast<std::size_t> (t*arr.size());
	if (index < arr.size())
		return arr[index];
	return arr.back();
}

void AECFilter::generateDensityWeightMap(const std::vector<double>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposure)
{
	std::size_t size = dimensions[0] * dimensions[1] * dimensions[2];
	if (densityImage.size() != size)
		return;
	if (exposure.size() == 0)
		return;
	m_massWeightMap.clear();
	m_massWeightMap.reserve(dimensions[2]);
	
	const double voxelVolume = spacing[0] * spacing[1] * spacing[2];
	const std::size_t step = dimensions[0] * dimensions[1];
	double exposureMean = std::accumulate(exposure.begin(), exposure.end(), 0.0) / exposure.size();
	for (std::size_t k = 0; k < dimensions[2]; ++k)
	{
		const std::size_t offset = step * k;
		auto beg = densityImage.begin();
		auto end = densityImage.begin();
		std::advance(beg, offset);
		std::advance(end, offset + step);
		double sumDens = std::accumulate(beg, end, 0.0);
		m_massWeightMap.push_back(std::make_pair(sumDens*voxelVolume, arraySubIndex(exposure, k, dimensions[2]) / exposureMean));
	}
	std::sort(m_massWeightMap.begin(), m_massWeightMap.end());
}

void AECFilter::generateDensityWeightMap(std::shared_ptr<std::vector<double>>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposure)
{
	std::size_t size = dimensions[0] * dimensions[1] * dimensions[2];
	if (densityImage->size() != size)
		return;
	
	if (exposure.size() ==0)
		return;

	m_massWeightMap.clear();
	m_massWeightMap.reserve(dimensions[2]);

	const double voxelVolume = spacing[0] * spacing[1] * spacing[2];
	const std::size_t step = dimensions[0] * dimensions[1];
	double exposureMean = std::accumulate(exposure.begin(), exposure.end(), 0.0) / exposure.size();
	for (std::size_t k = 0; k < dimensions[2]; ++k)
	{
		const std::size_t offset = step * k;
		auto beg = densityImage->begin();
		auto end = densityImage->begin();
		std::advance(beg, offset);
		std::advance(end, offset + step);
		double sumDens = std::accumulate(beg, end, 0.0);
		m_massWeightMap.push_back(std::make_pair(sumDens*voxelVolume, arraySubIndex(exposure, k, dimensions[2]) / exposureMean));
	}
	std::sort(m_massWeightMap.begin(), m_massWeightMap.end());
}

double AECFilter::interpolateMassWeight(double mass) const
{
	auto it = std::upper_bound(m_massWeightMap.begin(), m_massWeightMap.end(), std::make_pair(mass, 0.0), [](const std::pair<double, double>& left, const std::pair<double, double>& right) {return left.first < right.first; });
	
	if (it == m_massWeightMap.end())
		return m_massWeightMap.back().second;
	if (it == m_massWeightMap.begin())
		return m_massWeightMap.front().second;
	auto it0 = it;
	std::advance(it0, -1);
	auto p1 = *it;
	auto p0 = *it0;
	return p0.second + (p1.second-p0.second) * (mass-p0.first) / (p1.first-p0.first);
}

