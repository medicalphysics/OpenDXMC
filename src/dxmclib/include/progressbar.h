#pragma once
#include <atomic>
#include <chrono>
#include <string>
#include <sstream>
#include <iomanip> 

class ProgressBar
{
public:
	ProgressBar() {};
	ProgressBar(std::uint64_t totalExposures) { setTotalExposures(totalExposures); }
	void setTotalExposures(std::uint64_t totalExposures, const std::string& message = "")
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
	std::atomic<std::uint64_t> m_totalExposures = 0;
	std::atomic<std::uint64_t> m_currentExposures = 0;
	std::chrono::system_clock::time_point m_startTime;
	std::atomic<double> m_secondsElapsed;
	std::string m_message;
};