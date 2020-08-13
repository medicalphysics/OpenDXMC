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

#include "opendxmc/colormap.h"

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
    std::array<double, 3> arr { q.redF(), q.greenF(), q.blueF() };
    return arr;
}

inline double interp(double x0, double x1, double y0, double y1, double x)
{
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
}

std::array<double, 768> generateStandardColorTable(const QVector<double>& colorTable)
{
    std::array<double, 768> lut;
    const int nColors = colorTable.size() / 3;
    for (int i = 0; i < 255; ++i) {
        int idx0 = i * (nColors - 1) / 255;
        int idx1 = idx0 + 1;
        for (int j = 0; j < 3; ++j) {
            lut[i * 3 + j] = interp(idx0, idx1, colorTable[idx0 * 3 + j], colorTable[idx1 * 3 + j], i * (nColors - 1) / 255.0);
        }
    }
    for (int j = 0; j < 3; ++j)
        lut[255 * 3 + j] = colorTable[(nColors - 1) * 3 + j];
    return lut;
}

QVector<QRgb> generateStandardQTColorTable(const QVector<double>& colorTable)
{
    QVector<QRgb> cmap(256);
    auto lut = generateStandardColorTable(colorTable);
    for (std::size_t i = 0; i < 256; ++i) {
        const int r = static_cast<int>(lut[i * 3] * 255.0);
        const int g = static_cast<int>(lut[i * 3 + 1] * 255.0);
        const int b = static_cast<int>(lut[i * 3 + 2] * 255.0);
        cmap[i] = qRgb(r, g, b);
    }
    return cmap;
}