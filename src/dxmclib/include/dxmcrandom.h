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

#pragma once  // include guard

#include <cstdint>
#include <vector>
#include <utility>
#include <limits>

/*
 * We roll our own random number generator */



inline std::uint64_t xoroshiro128plus(std::uint64_t s[2])
{
    std::uint64_t s0 = s[0];
    std::uint64_t s1 = s[1];
    std::uint64_t result = s0 + s1;
    s1 ^= s0;
    s[0] = ((s0 << 55) | (s0 >> 9)) ^ s1 ^ (s1 << 14);
    s[1] = (s1 << 36) | (s1 >> 28);
    return result;
}

template<typename T>
inline T randomUniform(std::uint64_t s[2])
{
    const T r = static_cast<T>(xoroshiro128plus(s));
	return r / (std::numeric_limits<std::uint64_t>::max() - 1);
}

template<typename T>
inline T randomUniform(std::uint64_t s[2], const T max)
{
	std::uint64_t rint = xoroshiro128plus(s);
	const T r = static_cast<T>(rint);
	return (r / std::numeric_limits<std::uint64_t>::max()) * max;
}


template<typename T>
inline T randomUniform(std::uint64_t s[2], const T min, const T max)
{
    std::uint64_t rint = xoroshiro128plus(s);
    const T r = static_cast<T>(rint);
    const T range = max - min;
	return min + (r / std::numeric_limits<std::uint64_t>::max()) * range;
}

void randomSeed(std::uint64_t s[2]);


class RandomDistribution
{
public:
	RandomDistribution(const std::vector<double>& weights);
	std::size_t sampleIndex();
	std::size_t sampleIndex(std::uint64_t seed[2]) const;

private:
    std::vector<double> m_weights;
    std::vector<std::uint64_t> m_alias;
    std::vector<double> m_probs;
	std::uint64_t m_seed[2];

	void generateTable();
};

class SpecterDistribution : public RandomDistribution
{
public:
	SpecterDistribution(const std::vector<double>& weights, const std::vector<double>& energies);
	SpecterDistribution(const std::vector<std::pair<double, double>>& energyWeights);
	double sampleValue();
	double sampleValue(std::uint64_t seed[2]) const ; // thread safe
	
private:
	std::vector<double> m_energies;
};
