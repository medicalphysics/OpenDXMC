
#include "dxmcrandom.h"
#include <iostream>
#include <vector>
#include <array>
#include <algorithm>
#include <cassert>
#include <cmath>


void testUniform()
{
	constexpr std::size_t N = 100;
	std::array<std::size_t, N> v;
	v.fill(0);

	std::uint64_t seed[2];
	randomSeed(seed);

	for (std::size_t i = 0; i < 10000000000; ++i)
	{
		const auto t = static_cast<std::size_t>(randomUniform<double>(seed, 100.0));
		++v[t];
	}
	for (std::size_t i =0;i< N;++i)
	{
		std::cout << i << " " << v[i] << std::endl;
	}
	
}

void testWeights()
{

	std::uint64_t seed[2];
	randomSeed(seed);
	//std::uint64_t u1 = xoroshiro128plus(seed);
	double r1 = randomUniform<double>(seed);
	double r2 = randomUniform<double>(seed, 0.0, 3.14);

	std::cout << seed[0] << " " << seed[1] << " " << r1 << " " << r2 << std::endl;

	std::vector<double> weights({ 0, 1, 2, 3, 4, 5, 4, 3, 2, 1 });


	RandomDistribution* rd = new RandomDistribution(weights);
	std::vector<std::size_t> res(weights.size());
	for (std::size_t i = 0; i < 10000000; ++i)
	{
		std::uint64_t ind = rd->sampleIndex();
		res[ind] += 1;
	}



	for (auto k : weights)
	{
		std::cout << k << " ";
	}
	std::cout << std::endl;

	double max_res = static_cast<double>(*(std::max_element(res.begin(), res.end())));
	double max_weights = *(std::max_element(weights.begin(), weights.end()));

	std::vector<double> res_norm;
	std::transform(res.begin(), res.end(), std::back_inserter(res_norm), [max_res, max_weights](auto d) {return d * max_weights / max_res; });

	for (auto k : res_norm)
	{
		std::cout << k << " ";
	}
	std::cout << std::endl;

	std::vector<bool> is_equal;
	std::transform(weights.begin(), weights.end(), res_norm.begin(), std::back_inserter(is_equal), [](double w, double r) {return std::abs(w - r) < 0.01; });

	auto success = std::all_of(is_equal.begin(), is_equal.end(), [](auto v) {return v == true; });
	assert(success);
}


int main(int argc, char *argv[])
{
	testUniform();
	return 0;
}
