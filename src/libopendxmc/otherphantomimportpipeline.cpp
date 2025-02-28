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

Copyright 2024 Erlend Andersen
*/

#include "dxmc/material/nistmaterials.hpp"
#include <otherphantomimportpipeline.hpp>

OtherPhantomImportPipeline::OtherPhantomImportPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void OtherPhantomImportPipeline::updateImageData(std::shared_ptr<DataContainer>)
{
    // we do nothing
}

std::vector<std::uint8_t> generateCylinder(const std::array<std::size_t, 3>& dim)
{
    std::vector<std::uint8_t> cyl(dim[0] * dim[1] * dim[2]);

    const auto cx = dim[0] / 2.0;
    const auto cy = dim[1] / 2.0;
    const auto r = std::min(cx, cy);
    const auto r2 = r * r;

    for (std::size_t k = 0; k < dim[2]; ++k)
        for (std::size_t j = 0; j < dim[1]; ++j)
            for (std::size_t i = 0; i < dim[0]; ++i) {
                const auto ind = i + j * dim[0] + k * dim[0] * dim[1];
                const auto x = i - cx;
                const auto y = j - cy;
                cyl[ind] = x * x + y * y <= r2 ? 1 : 0;
            }
    return cyl;
}

std::vector<std::uint8_t> generateCube(const std::array<std::size_t, 3>& dim)
{
    std::vector<std::uint8_t> cube(dim[0] * dim[1] * dim[2], std::uint8_t { 1 });
    return cube;
}

void OtherPhantomImportPipeline::importPhantom(int type, double dx, double dy, double dz, int nx, int ny, int nz)
{
    emit dataProcessingStarted(ProgressWorkType::Importing);
    auto vol = std::make_shared<DataContainer>();

    const std::array<std::size_t, 3> dims = {
        static_cast<std::size_t>(nx),
        static_cast<std::size_t>(ny),
        static_cast<std::size_t>(nz)
    };
    const auto N = dims[0] * dims[1] * dims[2];
    vol->setDimensions(dims);
    vol->setSpacing({ dx, dy, dz });

    std::vector<std::uint8_t> mat;
    if (type == 0)
        mat = generateCylinder(dims);
    else
        mat = generateCube(dims);
    vol->setImageArray(DataContainer::ImageType::Material, mat);
    vol->setImageArray(DataContainer::ImageType::Organ, mat);

    std::vector<DataContainer::Material> materials;
    std::vector<std::string> organ_names;
    organ_names.push_back("Air, Dry (near sea level)");
    organ_names.push_back("Polymethyl Methacralate (Lucite, Perspex)");
    vol->setOrganNames(organ_names);

    for (const auto& n : organ_names) {
        auto Z = dxmc::NISTMaterials::Composition(n);
        materials.push_back({ .name = n, .Z = Z });
    }
    vol->setMaterials(materials);

    const double air_dens = dxmc::NISTMaterials::density(organ_names[0]);
    const double pmma_dens = dxmc::NISTMaterials::density(organ_names[1]);
    std::vector<double> dens(N);
    std::transform(std::execution::par_unseq, mat.cbegin(), mat.cend(), dens.begin(), [air_dens, pmma_dens](const auto m) {
        return m == 1 ? pmma_dens : air_dens;
    });
    vol->setImageArray(DataContainer::ImageType::Density, dens);

    emit imageDataChanged(vol);
    emit dataProcessingFinished(ProgressWorkType::Importing);
}
void OtherPhantomImportPipeline::importHMGUPhantom(QString path)
{
    emit dataProcessingStarted(ProgressWorkType::Importing);
    // TODO
    //Identify phantom
    //import phantom
    //generate CT image?
    
    //emit imageDataChanged(vol);
    
    emit dataProcessingFinished(ProgressWorkType::Importing);
}