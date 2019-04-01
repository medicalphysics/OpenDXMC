#pragma once

#include "imageimportpipeline.h"
#include "imagecontainer.h"
#include "dosereportcontainer.h"
#include "material.h"
#include "world.h"
#include "tube.h"
#include "source.h"

#include <QObject>

#include <vtkSmartPointer.h>
#include <vtkImageData.h>

#include <map>
#include <memory>
#include <vector>

Q_DECLARE_METATYPE(DoseReportContainer)


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