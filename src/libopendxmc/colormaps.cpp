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

Copyright 2023 Erlend Andersen
*/

#include <colormaps.hpp>

#include <cmath>
#include <map>
#include <numbers>

const std::vector<double> MAGMA = {
    0.001462,
    0.000466,
    0.013866,
    0.04383,
    0.03383,
    0.141886,
    0.123833,
    0.067295,
    0.295879,
    0.232077,
    0.059889,
    0.437695,
    0.341482,
    0.080564,
    0.492631,
    0.445163,
    0.122724,
    0.506901,
    0.550287,
    0.161158,
    0.505719,
    0.658483,
    0.196027,
    0.490253,
    0.767398,
    0.233705,
    0.457755,
    0.868793,
    0.287728,
    0.409303,
    0.944006,
    0.377643,
    0.365136,
    0.981,
    0.498428,
    0.369734,
    0.994738,
    0.62435,
    0.427397,
    0.997228,
    0.747981,
    0.516859,
    0.99317,
    0.870024,
    0.626189,
    0.987053,
    0.991438,
    0.749504,
};
const std::vector<double> SPRING = {
    1.0,
    0.0,
    1.0,
    1.0,
    0.0666666666667,
    0.933333333333,
    1.0,
    0.133333333333,
    0.866666666667,
    1.0,
    0.2,
    0.8,
    1.0,
    0.266666666667,
    0.733333333333,
    1.0,
    0.333333333333,
    0.666666666667,
    1.0,
    0.4,
    0.6,
    1.0,
    0.466666666667,
    0.533333333333,
    1.0,
    0.533333333333,
    0.466666666667,
    1.0,
    0.6,
    0.4,
    1.0,
    0.666666666667,
    0.333333333333,
    1.0,
    0.733333333333,
    0.266666666667,
    1.0,
    0.8,
    0.2,
    1.0,
    0.866666666667,
    0.133333333333,
    1.0,
    0.933333333333,
    0.0666666666667,
    1.0,
    1.0,
    0.0,
};
const std::vector<double> SUMMER = {
    0.0,
    0.5,
    0.4,
    0.0666666666667,
    0.533333333333,
    0.4,
    0.133333333333,
    0.566666666667,
    0.4,
    0.2,
    0.6,
    0.4,
    0.266666666667,
    0.633333333333,
    0.4,
    0.333333333333,
    0.666666666667,
    0.4,
    0.4,
    0.7,
    0.4,
    0.466666666667,
    0.733333333333,
    0.4,
    0.533333333333,
    0.766666666667,
    0.4,
    0.6,
    0.8,
    0.4,
    0.666666666667,
    0.833333333333,
    0.4,
    0.733333333333,
    0.866666666667,
    0.4,
    0.8,
    0.9,
    0.4,
    0.866666666667,
    0.933333333333,
    0.4,
    0.933333333333,
    0.966666666667,
    0.4,
    1.0,
    1.0,
    0.4,
};
const std::vector<double> COOL = {
    0.0,
    1.0,
    1.0,
    0.0666666666667,
    0.933333333333,
    1.0,
    0.133333333333,
    0.866666666667,
    1.0,
    0.2,
    0.8,
    1.0,
    0.266666666667,
    0.733333333333,
    1.0,
    0.333333333333,
    0.666666666667,
    1.0,
    0.4,
    0.6,
    1.0,
    0.466666666667,
    0.533333333333,
    1.0,
    0.533333333333,
    0.466666666667,
    1.0,
    0.6,
    0.4,
    1.0,
    0.666666666667,
    0.333333333333,
    1.0,
    0.733333333333,
    0.266666666667,
    1.0,
    0.8,
    0.2,
    1.0,
    0.866666666667,
    0.133333333333,
    1.0,
    0.933333333333,
    0.0666666666667,
    1.0,
    1.0,
    0.0,
    1.0,
};
const std::vector<double> TERRAIN = {
    0.2,
    0.2,
    0.6,
    0.111111111111,
    0.377777777778,
    0.777777777778,
    0.0222222222222,
    0.555555555556,
    0.955555555556,
    0.0,
    0.7,
    0.7,
    0.0666666666667,
    0.813333333333,
    0.413333333333,
    0.333333333333,
    0.866666666667,
    0.466666666667,
    0.6,
    0.92,
    0.52,
    0.866666666667,
    0.973333333333,
    0.573333333333,
    0.933333333333,
    0.914666666667,
    0.564,
    0.8,
    0.744,
    0.492,
    0.666666666667,
    0.573333333333,
    0.42,
    0.533333333333,
    0.402666666667,
    0.348,
    0.6,
    0.488,
    0.464,
    0.733333333333,
    0.658666666667,
    0.642666666667,
    0.866666666667,
    0.829333333333,
    0.821333333333,
    1.0,
    1.0,
    1.0,
};
const std::vector<double> BRG = {
    0.0,
    0.0,
    1.0,
    0.133333333333,
    0.0,
    0.866666666667,
    0.266666666667,
    0.0,
    0.733333333333,
    0.4,
    0.0,
    0.6,
    0.533333333333,
    0.0,
    0.466666666667,
    0.666666666667,
    0.0,
    0.333333333333,
    0.8,
    0.0,
    0.2,
    0.933333333333,
    0.0,
    0.0666666666667,
    0.933333333333,
    0.0666666666667,
    0.0,
    0.8,
    0.2,
    0.0,
    0.666666666667,
    0.333333333333,
    0.0,
    0.533333333333,
    0.466666666667,
    0.0,
    0.4,
    0.6,
    0.0,
    0.266666666667,
    0.733333333333,
    0.0,
    0.133333333333,
    0.866666666667,
    0.0,
    0.0,
    1.0,
    0.0,
};
const std::vector<double> HSV = {
    1.0,
    0.0,
    0.0,
    1.0,
    0.39375039375,
    0.0,
    1.0,
    0.787500787501,
    0.0,
    0.818748818749,
    1.0,
    0.0,
    0.424998424998,
    1.0,
    0.0,
    0.0312493437493,
    1.0,
    1.31250131245e-06,
    0.0,
    1.0,
    0.362500472497,
    0.0,
    1.0,
    0.756248385634,
    0.0,
    0.850002756253,
    1.0,
    0.0,
    0.456252362502,
    1.0,
    0.0,
    0.062501968752,
    1.0,
    0.331248424998,
    0.0,
    1.0,
    0.724998818749,
    0.0,
    1.0,
    1.0,
    0.0,
    0.881250787501,
    1.0,
    0.0,
    0.48750039375,
    1.0,
    0.0,
    0.09375,
};
const std::vector<double> BONE = {
    0.0,
    0.0,
    0.0,
    0.0583333333333,
    0.0583333105072,
    0.0811594202899,
    0.116666666667,
    0.116666621014,
    0.16231884058,
    0.175,
    0.174999931522,
    0.24347826087,
    0.233333333333,
    0.233333242029,
    0.324637681159,
    0.291666666667,
    0.291666552536,
    0.405797101449,
    0.35,
    0.361458320963,
    0.474999881875,
    0.408333333333,
    0.441666640625,
    0.533333228333,
    0.466666666667,
    0.521874960286,
    0.591666574792,
    0.525,
    0.602083279948,
    0.64999992125,
    0.583333333333,
    0.682291599609,
    0.708333267708,
    0.641666666667,
    0.762499919271,
    0.766666614167,
    0.726562401562,
    0.825,
    0.824999960625,
    0.817708267708,
    0.883333333333,
    0.883333307083,
    0.908854133854,
    0.941666666667,
    0.941666653542,
    1.0,
    1.0,
    1.0,
};
const std::vector<double> SIMPLE = { 0.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 0.0 };
const std::vector<double> GRAY = { 0.0, 0.0, 0.0, 1.0, 1.0, 1.0 };
const std::vector<double> CT = { 0, 0, 0, .55, .25, .15, .88, .60, .29, 1, .94, .95, .83, .66, 1 };
const std::vector<double> HOT_IRON = { 0, 0, 0, 0.5, 0, 0, 1, 0, 0, 1, .5, 0, 1, 1, 1 };
const std::vector<double> PET = { 0, 0, 0, 0, .5, .5, .5, 0, 1, 1, .5, 0, 1, 1, 1 };
const std::vector<double> JET = { 0, 0, .5, 0, 0, 1, 0, 0.5, 1, 0, 1, 1, 0.5, 1, 0.5, 1, 1, 0, 1, 0.5, 0, 1, 0, 0, 0.5, 0, 0 };
const std::vector<double> TURBO = { 0.18995, 0.07176, 0.23217, 0.225, 0.16354, 0.45096, 0.25107, 0.25237, 0.63374, 0.26816, 0.33825, 0.7805, 0.27628, 0.42118, 0.89123, 0.27543, 0.50115, 0.96594, 0.25862, 0.57958, 0.99876, 0.21382, 0.65886, 0.97959, 0.15844, 0.73551, 0.92305, 0.11167, 0.80569, 0.84525, 0.09267, 0.86554, 0.7623, 0.12014, 0.91193, 0.6866, 0.19659, 0.94901, 0.59466, 0.30513, 0.97697, 0.48987, 0.42778, 0.99419, 0.38575, 0.54658, 0.99907, 0.29581, 0.64362, 0.98999, 0.23356, 0.72596, 0.9647, 0.2064, 0.80473, 0.92452, 0.20459, 0.8753, 0.87267, 0.21555, 0.93301, 0.81236, 0.22667, 0.97323, 0.74682, 0.22536, 0.99314, 0.67408, 0.20348, 0.99593, 0.58703, 0.16899, 0.9836, 0.49291, 0.12849, 0.95801, 0.39958, 0.08831, 0.92105, 0.31489, 0.05475, 0.87422, 0.24526, 0.03297, 0.81608, 0.18462, 0.01809, 0.74617, 0.13098, 0.00851, 0.66449, 0.08436, 0.00424, 0.57103, 0.04474, 0.00529 };

static const std::map<std::string, std::vector<double>> COLORMAPS = {
    { "SIMPLE", SIMPLE },
    { "GRAY", GRAY },
    { "TURBO", TURBO },
    { "CT", CT },
    { "PET", PET },
    { "BONE", BONE },
    { "HOT IRON", HOT_IRON },
    { "MAGMA", MAGMA },
    { "COOL", COOL }
};

std::vector<std::string> Colormaps::availableColormaps()
{
    std::vector<std::string> n;
    n.reserve(COLORMAPS.size());
    for (const auto& [key, value] : COLORMAPS)
        n.push_back(key);
    return n;
}

double interpolate(double x, double x0, double x1, double y0, double y1)
{
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

std::vector<double> interpolateColormap(const std::vector<double>& map)
{
    const auto N = map.size() / 3;
    const double mstep = 1.0 / (N - 1);
    const double step = 1.0 / 255.0;
    std::vector<double> res;
    res.reserve(256 * 3);
    double x0 = 0;
    double x1 = mstep;
    int y0_ind = 0;
    for (int i = 0; i < 256; ++i) {
        const auto x = step * i;
        while (x1 <= x && y0_ind < N - 2) {
            x0 = x1;
            x1 += mstep;
            y0_ind++;
        }
        for (int j = 0; j < 3; ++j) {
            res.push_back(interpolate(x, x0, x1, map[y0_ind * 3 + j], map[(y0_ind + 1) * 3 + j]));
        }
    }
    return res;
}

const std::vector<double>& Colormaps::colormap(const std::string& name)
{
    if (!COLORMAPS.contains(name)) {
        return colormap("GRAY");
    }
    return COLORMAPS.at(name);
}
std::vector<double> Colormaps::colormapLongForm(const std::string& name)
{
    if (!COLORMAPS.contains(name)) {
        return interpolateColormap(colormap("GRAY"));
    }
    return interpolateColormap(COLORMAPS.at(name));
}

bool Colormaps::haveColormap(const std::string& name)
{
    return COLORMAPS.contains(name);
}

std::array<double, 4> HSVtoRGB(double H, double S, double V, double alpha = 1.0)
{
    if (S == 0) {
        std::array res = { V, V, V, alpha };
        return res;
    } else {
        auto var_h = H * 6;
        if (var_h == 6)
            var_h = 0; // H must be < 1
        const auto var_i = int(var_h); // Or ... var_i = floor( var_h )
        const auto var_1 = V * (1 - S);
        const auto var_2 = V * (1 - S * (var_h - var_i));
        const auto var_3 = V * (1 - S * (1 - (var_h - var_i)));
        double var_r, var_g, var_b;
        if (var_i == 0) {
            var_r = V;
            var_g = var_3;
            var_b = var_1;
        } else if (var_i == 1) {
            var_r = var_2;
            var_g = V;
            var_b = var_1;
        } else if (var_i == 2) {
            var_r = var_1;
            var_g = V;
            var_b = var_3;
        } else if (var_i == 3) {
            var_r = var_1;
            var_g = var_2;
            var_b = V;
        } else if (var_i == 4) {
            var_r = var_3;
            var_g = var_1;
            var_b = V;
        } else {
            var_r = V;
            var_g = var_1;
            var_b = var_2;
        }

        std::array res = { var_r, var_g, var_b, alpha };
        return res;
    }
}

std::array<double, 4> Colormaps::discreetColor(int idx, double alpha)
{
    std::array<double, 4> c;
    if (idx < 1)
        c = { 0, 0, 0, alpha };
    else {
        constexpr auto golden_ratio_conjugate = 1 / std::numbers::phi_v<double>;
        const double h = std::fmod(golden_ratio_conjugate * idx, 1);
        c = HSVtoRGB(h, 0.65, 0.95, alpha);
    }
    return c;
}
