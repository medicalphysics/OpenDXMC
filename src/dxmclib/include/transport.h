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

#pragma once
#include "world.h"
#include "exposure.h"
#include "source.h"
#include "dxmcrandom.h"
#include "vectormath.h"

#include <memory>
#include <algorithm>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip> 

namespace transport {
	
	class ProgressBar
	{
	public:
		ProgressBar();
		ProgressBar(std::uint64_t totalExposures) { setTotalExposures(totalExposures); }
		void setTotalExposures(std::uint64_t totalExposures, const std::string& message="") 
		{
			m_totalExposures = totalExposures; 
			m_currentExposures = 0; 
			m_message = message;
			m_startTime = std::chrono::system_clock::now();
		} //not thread safe
		void exposureCompleted() // threadsafe
		{
			m_currentExposures++;
			auto duration = std::chrono::system_clock::now() - m_startTime;
			auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count();
			m_secondsElapsed.exchange(static_cast<double>(seconds));
		}
		std::string getETA() const
		{
			auto secondsRemaining = m_secondsElapsed.load() / m_currentExposures.load() * (m_totalExposures.load() - m_currentExposures.load());
			return makePrettyTime(secondsRemaining);
		}
	protected:
		std::string makePrettyTime(double seconds) const { 
			std::stringstream ss;
			ss << std::fixed << std::setprecision(0);
			ss << m_message << " ETA: ";
			if (seconds > 120.0)
				ss << seconds / 60.0 << " minutes";
			else
				ss << seconds << " seconds";
			return ss.str();
		}
	private:
		std::atomic<std::uint64_t> m_totalExposures=0;
		std::atomic<std::uint64_t> m_currentExposures=0;
		std::chrono::system_clock::time_point m_startTime;
		std::atomic<double> m_secondsElapsed;
		std::string m_message;
	};

	double comptonScatter(Particle& particle, std::uint64_t seed[2], double& cosAngle);
	double comptonScatterGeant(Particle& particle, std::uint64_t seed[2], double& cosAngle);
	double comptonScatterEGS(Particle& particle, std::uint64_t seed[2], double& cosAngle);
	void rayleightScatter(Particle& particle, unsigned char materialIdx, const AttenuationLut& attLut, std::uint64_t seed[2], double& cosAngle);
	std::vector<double> run(const World& world, Source* source, ProgressBar* progressBar = nullptr);
	std::vector<double> run(const CTDIPhantom& world, CTSource* source, ProgressBar* progressBar = nullptr);
}