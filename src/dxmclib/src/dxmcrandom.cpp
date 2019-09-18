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

#include "dxmcrandom.h"
#include <numeric>
#include <random>

void randomSeed(std::uint64_t s[2])
{
	std::random_device d;
    s[0] = d() + d();
    s[1] = d() + d();

	while (s[0] == 0)
	{
		s[0] = d();
	}
	while (s[1] == 0)
	{
		s[1] = d();
	}
}




RandomDistribution::RandomDistribution(const std::vector<double>& weights) 
{
	randomSeed(m_seed); // initialize prng
	m_weights = weights;
	m_probs.resize(m_weights.size());
	m_alias.resize(m_weights.size());
	generateTable();
}

void RandomDistribution::generateTable()
{
	const std::size_t nums = m_weights.size();
	std::vector<double> norm_probs(nums);
	std::vector<std::int64_t> large_block(nums);
	std::vector<std::int64_t> small_block(nums);

	std::int64_t cur_small_block;
	std::int64_t cur_large_block;
	std::int64_t num_small_block = 0;
	std::int64_t num_large_block = 0;
	
	double sum = std::accumulate(m_weights.begin(), m_weights.end(), 0.0);
	const double scale_factor = nums / sum;

	// norm_prob
	for (std::size_t i = 0; i < m_weights.size(); i++)
	{
		norm_probs[i] = m_weights[i] * scale_factor;
	}


	for (std::int64_t i = nums - 1; i >= 0; i--) 
	{
		if (norm_probs[i] < 1.0)
		{
			small_block[num_small_block++] = i;
		}
		else
		{
			large_block[num_large_block++] = i;
		}
	}

	while (num_small_block && num_large_block) 
	{
		cur_small_block = small_block[--num_small_block];
		cur_large_block = large_block[--num_large_block];
		m_probs[cur_small_block] = norm_probs[cur_small_block];
		m_alias[cur_small_block] = cur_large_block;
		norm_probs[cur_large_block] = norm_probs[cur_large_block] + norm_probs[cur_small_block] - 1;
		if (norm_probs[cur_large_block] < 1)
		{
			small_block[num_small_block++] = cur_large_block;
		}
		else
		{
			large_block[num_large_block++] = cur_large_block;
		}
	}

	while (num_large_block)
	{
		m_probs[large_block[--num_large_block]] = 1;
	}

	while (num_small_block)
	{
		m_probs[small_block[--num_small_block]] = 1;
	}

}

std::size_t RandomDistribution::sampleIndex()
{
	return sampleIndex(m_seed);
}

std::size_t RandomDistribution::sampleIndex(std::uint64_t seed[2]) const
{
	const double r1 = randomUniform<double>(seed);
	const double r2 = randomUniform<double>(seed);
	std::size_t k = static_cast<std::size_t>(m_weights.size() * r1);
	return r2 < m_probs[k] ? k : m_alias[k];
}


SpecterDistribution::SpecterDistribution(const std::vector<double>& weights, const std::vector<double>& energies)
	: RandomDistribution(weights)
{
	m_energies = energies;
}


double SpecterDistribution::sampleValue()
{
	return m_energies[sampleIndex()];
}
double SpecterDistribution::sampleValue(std::uint64_t seed[2]) const
{
	return m_energies[sampleIndex(seed)];
}
