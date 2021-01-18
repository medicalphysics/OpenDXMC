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

#include "opendxmc/dosereportcontainer.h"
#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/imagecontainer.h"
#include "opendxmc/imageimportpipeline.h"

#include <QObject>

#include <vtkImageData.h>
#include <vtkSmartPointer.h>

#include <map>
#include <memory>
#include <vector>

#ifndef Q_DECLARE_METATYPE_IMAGECONTAINER
#define Q_DECLARE_METATYPE_IMAGECONTAINER
Q_DECLARE_METATYPE(std::shared_ptr<ImageContainer>)
#endif
#ifndef Q_DECLARE_METATYPE_PROGRESSBAR
#define Q_DECLARE_METATYPE_PROGRESSBAR
Q_DECLARE_METATYPE(std::shared_ptr<ProgressBar>)
#endif
#ifndef Q_DECLARE_METATYPE_DOSEREPORTCONTAINER
#define Q_DECLARE_METATYPE_DOSEREPORTCONTAINER
Q_DECLARE_METATYPE(DoseReportContainer)
#endif

class SimulationPipeline : public QObject {
    Q_OBJECT
public:
    SimulationPipeline(QObject* parent = nullptr);
    void setImageData(std::shared_ptr<ImageContainer> image);
    void setMaterials(const std::vector<Material>& materials);
    void setOrganList(const std::vector<std::string>& organList) { m_organList = organList; }
    void runSimulation(const std::vector<std::shared_ptr<Source>> sources);
    void setLowEnergyCorrection(int value) { m_lowEnergyCorrection = std::max(std::min(value, 2), 0); }
    bool ignoreAirDose() const { return m_ignoreAirDose; }
    void setIgnoreAirDose(bool on) { m_ignoreAirDose = on; }

signals:
    void processingDataStarted();
    void processingDataEnded();
    void progressBarChanged(ProgressBar* progressBar);
    void imageDataChanged(std::shared_ptr<ImageContainer> image);
    void doseDataChanged(const DoseReportContainer& doses);

private:
    bool m_ignoreAirDose = true;
    int m_lowEnergyCorrection = 1;
    std::uint64_t m_currentImageID = 0;
    std::shared_ptr<DensityImageContainer> m_densityImage = nullptr;
    std::shared_ptr<MaterialImageContainer> m_materialImage = nullptr;
    std::shared_ptr<OrganImageContainer> m_organImage = nullptr;
    std::shared_ptr<MeasurementImageContainer> m_measurementImage = nullptr;
    std::vector<std::string> m_organList;
    std::vector<Material> m_materialList;
};