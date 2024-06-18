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

#include <ctsegmentationpipeline.hpp>
#include <dxmc_specialization.hpp>

#include "dxmc/beams/tube/tube.hpp"
#include "dxmc/material/nistmaterials.hpp"

#include <execution>
#include <numeric>
#include <string>
#include <vector>

CTSegmentationPipeline::CTSegmentationPipeline(QObject* parent)
    : BasePipeline(parent)
{
}

void CTSegmentationPipeline::setAqusitionVoltage(double kv)
{
    m_kv = kv;
}

void CTSegmentationPipeline::setAlFiltration(double f)
{
    m_Al_filt_mm = std::max(0.0, f);
}

void CTSegmentationPipeline::setSnFiltration(double f)
{
    m_Sn_filt_mm = std::max(0.0, f);
}

using MatDens = std::pair<Material, double>;

struct CTNumberData {
    std::vector<double> HU;
    std::vector<double> attenuation;
    double attenuationWater = 0;
    double attenuationAir = 0;
    double water_dens = 0;
    double air_dens = 0;
};

CTNumberData getMeanCTNumbers(const std::vector<MatDens>& materials, double al_filtration, double sn_filtration, double tube_kvp)
{

    CTNumberData data;

    Tube tube(tube_kvp);
    tube.setAlFiltration(al_filtration);
    tube.setSnFiltration(sn_filtration);

    const auto spec_en = tube.getEnergy();
    const auto spec_w = tube.getSpecter(spec_en, true);

    const auto air = Material::byNistName("Air, Dry (near sea level)").value();
    const auto air_dens = NISTMaterials::density("Air, Dry (near sea level)");
    const auto water = Material::byNistName("Water, Liquid").value();
    const auto water_dens = NISTMaterials::density("Water, Liquid");

    data.HU.resize(materials.size());
    for (int i = 0; i < materials.size(); ++i) {
        const auto& mat = materials[i].first;
        const auto mat_dens = materials[i].second;
        data.HU[i] = 1000 * std::transform_reduce(std::execution::par_unseq, spec_en.cbegin(), spec_en.cend(), spec_w.cbegin(), 0.0, std::plus(), [&](const auto e, const auto w) {
            const auto uw = water.attenuationValues(e).sum() * water_dens;
            const auto ua = air.attenuationValues(e).sum() * air_dens;
            const auto um = mat.attenuationValues(e).sum() * mat_dens;

            return w * (um - uw) / (uw - ua);
        });
    }

    data.attenuationWater = std::transform_reduce(std::execution::par_unseq, spec_en.cbegin(), spec_en.cend(), spec_w.cbegin(), 0.0, std::plus(), [&](const auto e, const auto w) {
        const auto uw = water.attenuationValues(e).sum();
        return w * uw;
    });
    data.attenuationAir = std::transform_reduce(std::execution::par_unseq, spec_en.cbegin(), spec_en.cend(), spec_w.cbegin(), 0.0, std::plus(), [&](const auto e, const auto w) {
        const auto uw = air.attenuationValues(e).sum();
        return w * uw;
    });

    for (const auto& [mat, dens] : materials) {
        const auto a = std::transform_reduce(std::execution::par_unseq, spec_en.cbegin(), spec_en.cend(), spec_w.cbegin(), 0.0, std::plus(), [&](const auto e, const auto w) {
            const auto uw = mat.attenuationValues(e).sum();
            return w * uw;
        });
        data.attenuation.push_back(a);
    }
    data.water_dens = water_dens;
    data.air_dens = air_dens;

    return data;
}

void CTSegmentationPipeline::updateImageData(std::shared_ptr<DataContainer> data)
{
    if (!data)
        return;
    if (!data->hasImage(DataContainer::ImageType::CT))
        return;
    emit dataProcessingStarted(ProgressWorkType::Segmentating);
    std::vector<std::string> mat_names = { "Air, Dry (near sea level)", "Adipose Tissue (ICRP)", "Tissue, Soft (ICRP)", "Muscle, Skeletal", "Bone, Cortical (ICRP)" };

    std::vector<MatDens> materials;
    for (const auto& n : mat_names)
        materials.push_back(std::make_pair(Material::byNistName(n).value(), NISTMaterials::density(n)));

    // Density for bone is to high
    materials.back().second = 1.09;

    const auto mat_data = getMeanCTNumbers(materials, m_Al_filt_mm, m_Sn_filt_mm, m_kv);
    const auto& mat_HU = mat_data.HU;
    std::vector<double> mat_HU_sep;
    for (std::size_t i = 0; i < mat_HU.size() - 1; ++i) {
        mat_HU_sep.push_back((mat_HU[i] + mat_HU[i + 1]) / 2);
    }

    std::vector<std::uint8_t> mat_array(data->size());
    const auto& HU = data->getCTArray();
    std::transform(std::execution::par_unseq, HU.cbegin(), HU.cend(), mat_array.begin(), [&](const auto h) {
        for (std::uint8_t i = 0; i < mat_HU_sep.size(); ++i) {
            if (h < mat_HU_sep[i])
                return i;
        }
        return static_cast<std::uint8_t>(mat_HU_sep.size());
    });

    std::vector<double> dens_array(data->size());
    std::transform(std::execution::par_unseq, HU.cbegin(), HU.cend(), mat_array.cbegin(), dens_array.begin(), [&](const double hu, const std::uint8_t mIdx) {
        const auto& w_att = mat_data.attenuationWater;
        const auto& w_dens = mat_data.water_dens;
        const auto& a_att = mat_data.attenuationAir;
        const auto& a_dens = mat_data.air_dens;
        const auto& m_att = mat_data.attenuation[mIdx];

        const auto dens = (((w_att * w_dens - a_att * a_dens) * hu / 1000) + w_att * w_dens) / m_att;
        return std::max(dens, 0.0);
    });
    data->setImageArray(DataContainer::ImageType::Material, std::move(mat_array));
    data->setImageArray(DataContainer::ImageType::Density, std::move(dens_array));

    std::vector<DataContainer::Material> cont_materials(materials.size());
    for (int i = 0; i < materials.size(); ++i) {
        cont_materials[i].name = mat_names[i];
        cont_materials[i].Z = dxmc::NISTMaterials::Composition(mat_names[i]);
    }
    data->setMaterials(cont_materials);
    emit imageDataChanged(data);
    emit dataProcessingFinished(ProgressWorkType::Segmentating);
}