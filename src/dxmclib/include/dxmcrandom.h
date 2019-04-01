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
