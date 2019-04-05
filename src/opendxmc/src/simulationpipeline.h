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

#include "imageimportpipeline.h"
#include "imagecontainer.h"
#include "dosereportcontainer.h"
#include "material.h"
#include "world.h"
#include "tube.h"
#include "source.h"
#include "transport.h"

#include <QObject>

#include <vtkSmartPointer.h>
#include <vtkImageData.h>

#include <map>
#include <memory>
#include <vector>

Q_DECLARE_METATYPE(DoseReportContainer)
Q_DECLARE_METATYPE(std::shared_ptr<transport::ProgressBar>)

class SimulationPipeline : public QObject
{
	Q_OBJECT
public:
	SimulationPipeline(QObject *parent = nullptr);
	void setImageData(std::shared_ptr<ImageContainer> image);
	void setMaterials(const std::vector<Material>& materials);
	void setOrganList(const std::vector<std::string>& organList) { m_organList = organList; }
	void runSimulation(const std::vector<std::shared_ptr<Source>> sources);

	bool ignoreAirDose() const { return m_ignoreAirDose; }
	void setIgnoreAirDose(bool on) { m_ignoreAirDose = on; }

signals:
	void processingDataStarted();
	void processingDataEnded();
	void progressBarChanged(std::shared_ptr<transport::ProgressBar> progressBar);
	void imageDataChanged(std::shared_ptr<ImageContainer> image);
	void doseDataChanged(const DoseReportContainer& doses);

private:
	World m_world;
	bool m_ignoreAirDose = true;
	std::shared_ptr<DensityImageContainer> m_densityImage;
	std::shared_ptr<MaterialImageContainer> m_materialImage;
	std::shared_ptr<OrganImageContainer> m_organImage;
	std::vector<std::string> m_organList;
	
};