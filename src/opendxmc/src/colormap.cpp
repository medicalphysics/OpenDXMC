
#include "colormap.h"
#include <cmath>
QColor getQColor(int index)
{
	if (index < 1)
		return QColor(0, 0, 0);
	double golden_ratio_conjugate = 0.618033988749895;
	double h = golden_ratio_conjugate * index;
	return QColor::fromHsvF(std::fmod(h, 1.0), 0.65, 0.95);
}

std::array<double, 3> getColor(int index)
{
	auto q = getQColor(index);
	std::array<double, 3> arr{ q.redF(), q.greenF(), q.blueF() };
	return arr;
}

inline double interp(double x0, double x1, double y0, double y1, double x)
{
	return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

std::array<double, 768> generateStandardColorTable(const QVector<double>& colorTable)
{
	std::array<double, 768> lut;
	int nColors = colorTable.count() / 3;
	int cStep = 256 / (nColors - 1);
	for (int i = 0; i < 256; ++i)
	{
		int cIdx = i / cStep;
		for (int j = 0; j < 3; ++j)
			lut[i * 3 + j] = interp(cIdx, cIdx + 1, colorTable[cIdx * 3 + j], colorTable[cIdx * 3 + 3 + j], i / (double)cStep);
	}
	return lut;
}
