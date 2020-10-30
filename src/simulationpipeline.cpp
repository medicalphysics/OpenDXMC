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

#include "opendxmc/simulationpipeline.h"
#include "opendxmc/dxmc_specialization.h"

#include "vtkType.h"

SimulationPipeline::SimulationPipeline(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<DoseReportContainer>();
    qRegisterMetaType<std::shared_ptr<ProgressBar>>();
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
}

void SimulationPipeline::setImageData(std::shared_ptr<ImageContainer> image)
{
    if (!image)
        return;

    const auto id = image->ID;

    if (id != m_currentImageID) {
        m_densityImage = nullptr;
        m_materialImage = nullptr;
        m_measurementImage = nullptr;
        m_organImage = nullptr;
        m_currentImageID = id;
    }

    if (image->imageType == ImageContainer::ImageType::DensityImage)
        m_densityImage = std::static_pointer_cast<DensityImageContainer>(image);
    else if (image->imageType == ImageContainer::ImageType::MaterialImage)
        m_materialImage = std::static_pointer_cast<MaterialImageContainer>(image);
    else if (image->imageType == ImageContainer::ImageType::OrganImage)
        m_organImage = std::static_pointer_cast<OrganImageContainer>(image);
    else if (image->imageType == ImageContainer::ImageType::MeasurementImage)
        m_measurementImage = std::static_pointer_cast<MeasurementImageContainer>(image);
}

void SimulationPipeline::setMaterials(const std::vector<Material>& materials)
{
    m_materialList = materials;
}

void SimulationPipeline::runSimulation(const std::vector<std::shared_ptr<Source>> sources)
{
    emit processingDataStarted();

    auto dummyImage = std::make_shared<DoseImageContainer>();
    if (m_densityImage)
        dummyImage->ID = m_densityImage->ID;
    emit imageDataChanged(dummyImage);

    auto dummyTally = std::make_shared<TallyImageContainer>();
    if (m_densityImage)
        dummyTally->ID = m_densityImage->ID;
    emit imageDataChanged(dummyTally);

    auto dummyVariance = std::make_shared<VarianceImageContainer>();
    if (m_densityImage)
        dummyVariance->ID = m_densityImage->ID;
    emit imageDataChanged(dummyVariance);

    DoseReportContainer dummyContainer;
    emit doseDataChanged(dummyContainer);

    if (!m_densityImage || !m_materialImage) {
        emit processingDataEnded();
        return;
    }
    if (m_densityImage->ID != m_materialImage->ID) {
        emit processingDataEnded();
        return;
    }

    std::array<double, 3> spacing;
    std::array<std::size_t, 3> dimensions;
    for (std::size_t i = 0; i < 3; ++i) {
        spacing[i] = (m_densityImage->image->GetSpacing())[i];
        dimensions[i] = static_cast<std::size_t>((m_densityImage->image->GetDimensions())[i]);
    }
    World world;
    world.setSpacing(spacing);
    world.setDimensions(dimensions);
    world.setDirectionCosines(m_densityImage->directionCosines);
    world.setMaterialIndexArray(m_materialImage->imageData());
    world.setDensityArray(m_densityImage->imageData());
    for (const Material& m : m_materialList)
        world.addMaterialToMap(m);
    if (m_measurementImage)
        world.setMeasurementMapArray(m_measurementImage->imageData());

    auto totalDose = std::make_shared<std::vector<double>>(world.size(), 0.0);
    auto totalTally = std::make_shared<std::vector<std::uint32_t>>(world.size(), 0);
    auto totalVariance = std::make_shared<std::vector<double>>(world.size(), 0.0);

    for (std::shared_ptr<Source> s : sources) {
        
        world.makeValid();
        auto progressBar = std::make_unique<ProgressBar>(s->totalExposures());
        emit progressBarChanged(progressBar.get());
        Transport transport;
        const auto result = transport(world, s.get(), progressBar.get());
        std::transform(std::execution::par_unseq, totalDose->cbegin(), totalDose->cend(), result.dose.cbegin(), totalDose->begin(), std::plus<>());
        std::transform(std::execution::par_unseq, totalTally->cbegin(), totalTally->cend(), result.nEvents.cbegin(), totalTally->begin(), std::plus<>());
        std::transform(std::execution::par_unseq, totalVariance->cbegin(), totalVariance->cend(), result.variance.cbegin(), totalVariance->begin(), std::plus<>());
        emit progressBarChanged(nullptr);
    }

    if (m_ignoreAirDose) {
        auto mat = world.materialIndexArray();
        auto mat0 = world.materialMap().at(0);
        if (mat0.name().compare("Air, Dry (near sea level)") == 0) //ignore air only if present
        {
            std::transform(std::execution::par_unseq, totalDose->cbegin(), totalDose->cend(), mat->cbegin(), totalDose->begin(),
                [](double e, unsigned char i) -> double { return i == 0 ? 0.0 : e; });
        }
    }
    std::array<double, 3> origin;
    for (int i = 0; i < 3; ++i)
        origin[i] = -(dimensions[i] * spacing[i] * 0.5);

    //scaling dosevalues to sane units
    const double maxDose = *std::max_element(std::execution::par_unseq, totalDose->cbegin(), totalDose->cend());
    std::string dataUnits = "mGy";
    if (maxDose < 1.0 / 1000.0) {
        dataUnits = "nGy";
        std::transform(std::execution::par_unseq, totalDose->cbegin(), totalDose->cend(), totalDose->begin(),
            [](double d) { return d * 1e6; });
    } else if (maxDose < 1.0) {
        dataUnits = "uGy";
        std::transform(std::execution::par_unseq, totalDose->cbegin(), totalDose->cend(), totalDose->begin(),
            [](double d) { return d * 1e3; });
    }

    auto doseContainer = std::make_shared<DoseImageContainer>(totalDose, dimensions, spacing, origin);
    doseContainer->directionCosines = m_densityImage->directionCosines;
    doseContainer->ID = m_densityImage->ID;
    doseContainer->dataUnits = dataUnits;

    auto tallyContainer = std::make_shared<TallyImageContainer>(totalTally, dimensions, spacing, origin);
    tallyContainer->directionCosines = m_densityImage->directionCosines;
    tallyContainer->ID = m_densityImage->ID;
    tallyContainer->dataUnits = "# Events";

    auto varianceContainer = std::make_shared<VarianceImageContainer>(totalVariance, dimensions, spacing, origin);
    varianceContainer->directionCosines = m_densityImage->directionCosines;
    varianceContainer->ID = m_densityImage->ID;
    varianceContainer->dataUnits = "Variance " + dataUnits;

    if (m_organImage && (m_organList.size() > 0)) {
        if (m_organImage->ID == m_materialImage->ID) {
            DoseReportContainer cont(world.materialMap(), m_organList, m_materialImage, m_organImage, m_densityImage, doseContainer, tallyContainer);
            emit doseDataChanged(cont);
        } else {
            DoseReportContainer cont(world.materialMap(), m_materialImage, m_densityImage, doseContainer, tallyContainer);
            emit doseDataChanged(cont);
        }
    } else {
        DoseReportContainer cont(world.materialMap(), m_materialImage, m_densityImage, doseContainer, tallyContainer);
        emit doseDataChanged(cont);
    }
    emit imageDataChanged(doseContainer);
    emit imageDataChanged(tallyContainer);
    emit imageDataChanged(varianceContainer);
    emit processingDataEnded();
}
