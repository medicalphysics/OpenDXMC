
#pragma once
#include "world.h"
#include <vector>
#include <array>

class BeamFilter
{
public:
	BeamFilter() {};
	virtual double sampleIntensityWeight(const double angle) const = 0;
};

class BowTieFilter : public BeamFilter
{
public:
	BowTieFilter(const std::vector<double>& angles, const std::vector<double>& weights);
    BowTieFilter(const std::vector<std::pair<double, double>>& angleWeightsPairs);
	BowTieFilter(const BowTieFilter& other);
	double sampleIntensityWeight(const double angle) const override;
	const std::vector<std::pair<double, double>>& data() const { return m_data; }
protected:
	void normalizeData();
private:
    std::vector<std::pair<double, double>> m_data;
};


class XCareFilter :public BeamFilter
{
public:
	XCareFilter();

	double filterAngle() const { return m_filterAngle; }
	double filterAngleDeg() const;
	void setFilterAngle(double angle);
	void setFilterAngleDeg(double angle);

	double spanAngle() const { return m_rampAngle; }
	double spanAngleDeg() const;
	void setSpanAngle(double angle);
	void setSpanAngleDeg(double angle);

	double rampAngle() const { return m_rampAngle; }
	double rampAngleDeg() const;
	void setRampAngle(double angle);
	void setRampAngleDeg(double angle);

	double lowWeight() const { return m_lowWeight; }
	void setLowWeight(double weight);
	double highWeight() const;

	double sampleIntensityWeight(const double angle) const override;

private:
	double m_filterAngle;
	double m_spanAngle;
	double m_rampAngle;
	double m_lowWeight;

};


class PositionalFilter
{
public:
	PositionalFilter() {};
	virtual double sampleIntensityWeight(const std::array<double, 3>& position) const = 0;
	virtual void updateFromWorld(const World& world) = 0;
};


class AECFilter :public PositionalFilter
{
public:
	AECFilter(const std::vector<double>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposuremapping);
	AECFilter(std::shared_ptr<std::vector<double>>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposuremapping);
	
	double sampleIntensityWeight(const std::array<double, 3>& position) const;
	void updateFromWorld(const World& world) override;
	bool isValid() const { return m_valid; }
protected:
	void setCurrentDensityImage(const std::vector<double>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::array<double, 3> &origin);
	void setCurrentDensityImage(std::shared_ptr<std::vector<double>>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::array<double, 3> &origin);
	void generateDensityWeightMap(const std::vector<double>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposuremapping);
	void generateDensityWeightMap(std::shared_ptr<std::vector<double>>& densityImage, const std::array<double, 3> spacing, const std::array<std::size_t, 3> dimensions, const std::vector<double>& exposuremapping);
	double interpolateMassWeight(double mass) const;
private:
	bool m_valid = false;
	std::vector<std::pair<double, double>> m_massWeightMap;
	std::vector<std::pair<double, double>> m_positionWeightMap;

};