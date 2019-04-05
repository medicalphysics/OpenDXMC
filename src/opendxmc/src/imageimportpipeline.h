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
#include "material.h"
#include "imagecontainer.h"
#include "tube.h"
#include "beamfilters.h"
#include "attenuationlut.h"

#include <QObject>
#include <QStringList>
#include <QString>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageImport.h>
#include <vtkType.h>
#include <vtkDICOMReader.h>

#include <memory>
#include <vector>
#include <string>
#include <numeric>
#include <execution>
#include <future>
#include <thread>
#include <limits>

Q_DECLARE_METATYPE(std::shared_ptr<ImageContainer>) // to use ImageData in QT signal slots
Q_DECLARE_METATYPE(std::vector<Material>)
Q_DECLARE_METATYPE(std::vector<std::string>)
Q_DECLARE_METATYPE(std::shared_ptr<AECFilter>)



template<class iterT, class iterU, typename T, typename U>
void generateMaterialMapWorker(iterT CTArrayFirst, iterT CTArrayLast, iterU destination, const std::vector<std::pair<U, T>>& materialCTNumbers, unsigned int nThreads)
{
	auto len = std::distance(CTArrayFirst, CTArrayLast);
	if (materialCTNumbers.size() <= 1)
	{
		std::fill(destination, destination + len, 0);
		return;
	}

	if ((nThreads == 1) || (len < 1))
	{
		auto const  nThres = materialCTNumbers.size();
		std::vector<T> CTThres(nThres);
		for (std::size_t i = 0; i < nThres - 1; ++i)
		{
			CTThres[i] = (materialCTNumbers[i].second + materialCTNumbers[i + 1].second) * 0.5;
		}
		CTThres[nThres - 1] = std::numeric_limits<T>::infinity();
		while (CTArrayFirst != CTArrayLast)
		{
			U i = 0;
			while (*CTArrayFirst > CTThres[i])
				++i;
			*destination = materialCTNumbers[i].first;
			++CTArrayFirst;
			++destination;
		}
		return;
	}

	auto CTArrayMid = CTArrayFirst;
	std::advance(CTArrayMid, len / 2);
	auto destinationMid = destination;
	std::advance(destinationMid, len / 2);
	auto handle = std::async(std::launch::async, generateMaterialMapWorker<iterT, iterU, T, U>, CTArrayMid, CTArrayLast, destinationMid, materialCTNumbers, nThreads - 1);
	generateMaterialMapWorker(CTArrayFirst, CTArrayMid, destination, materialCTNumbers, nThreads - 1);
	handle.get();
	return;
}

template<typename S>
class CalculateCTNumberFromMaterials
{
public:
	CalculateCTNumberFromMaterials(std::vector<Material>& materialMap, const Tube& tube)
	{
		materialCTNumbers(materialMap, tube);
	}

	template<class iterT, class iterU>
	void generateMaterialMap(iterT CTArrayFirst, iterT CTArrayLast, iterU destination, unsigned int nThreads)
	{
		generateMaterialMapWorker(CTArrayFirst, CTArrayLast, destination, m_materialCTNumbers, nThreads);
	}
	template<class iterT, class iterU, class iterD>
	void generateDensityMap(iterT CTArrayFirst, iterT CTArrayLast, iterU materialIndex, iterD destination)
	{
		//calculate density based on voxel_i CT number and estimated CT number from material M in voxel_i 
		//std::fill(CTArrayFirst, CTArrayLast, 0);
		//return;
		if (m_materialCTNumbers.size() > 0)
		{
			std::vector<double> ctNumbers(m_materialCTNumbers.size());
			for (auto[index, hu] : m_materialCTNumbers)
				ctNumbers[index] = hu;
			auto constant = (m_calibrationEnergy[0] * m_calibrationDensity[0] - m_calibrationEnergy[1] * m_calibrationDensity[1]) / 1000.0;

			std::transform(std::execution::par_unseq, CTArrayFirst, CTArrayLast, materialIndex, destination,
				[constant, &ctNumbers, this](auto val, auto index) -> double {
				const double dens = (val - ctNumbers[index]) * constant / m_materialEnergy[index] + m_materialDensity[index];
				return dens > 0.0 ? dens : 0.0;
				//return dens;
			});
			
		}
	}

private:
	void materialCTNumbers(std::vector<Material>& materialMap, const Tube& tube)
	{
		// claibrationEnergy and materialEnergy are the simulated detector responce from each material for the specified tube
		//std::vector<AttenuationLutElement> calibrationLut;
		//Material waterMaterial("Water, Liquid");
		//Material airMaterial("Air, Dry (near sea level)");
		//calibrationLut.push_back(waterMaterial.generateAttenuationLut(1.0, tube.voltage()));
		//calibrationLut.push_back(airMaterial.generateAttenuationLut(1.0, tube.voltage()));
		std::vector<Material> calibrationMaterials;
		calibrationMaterials.push_back(Material("Water, Liquid"));
		calibrationMaterials.push_back(Material("Air, Dry (near sea level)"));
		AttenuationLut calibrationLut;
		calibrationLut.generate(calibrationMaterials, 1.0, tube.voltage());


		m_calibrationEnergy.clear();
		auto specterEnergy = std::vector<double>(calibrationLut.energyBegin(), calibrationLut.energyEnd());
		auto specterIntensity = tube.getSpecter(specterEnergy);
		m_calibrationDensity.clear();
		for (std::size_t i = 0; i < calibrationMaterials.size(); ++i)
		{
			m_calibrationDensity.push_back(calibrationMaterials[i].standardDensity());
			m_calibrationEnergy.push_back(std::transform_reduce(std::execution::par, calibrationLut.attenuationTotalBegin(i), calibrationLut.attenuationTotalEnd(i), specterIntensity.cbegin(), 0.0));
		}
		

		AttenuationLut attLut;
		attLut.generate(materialMap, 1.0, tube.voltage());

		//calculating CT number for each matrial based on the materials detector response 
		m_materialCTNumbers.clear();
		m_materialEnergy.clear();
		m_materialDensity.clear();
		for (std::size_t index = 0; index < materialMap.size(); ++index)
		{
			m_materialEnergy.push_back(std::transform_reduce(std::execution::par, attLut.attenuationTotalBegin(index), attLut.attenuationTotalEnd(index), specterIntensity.cbegin(), 0.0));
			m_materialDensity.push_back((materialMap[index]).standardDensity());
			double ctNumber = (m_materialEnergy[index] * m_materialDensity[index] - m_calibrationEnergy[0] * m_calibrationDensity[0]) / (m_calibrationEnergy[0] * m_calibrationDensity[0] - m_calibrationEnergy[1] * m_calibrationDensity[1]) * 1000.0; //Houndsfield units
			m_materialCTNumbers.push_back(std::make_pair(static_cast<S>(index), ctNumber));
		}
		std::sort(m_materialCTNumbers.begin(), m_materialCTNumbers.end(), [](const std::pair<std::size_t, double> &a, const std::pair<std::size_t, double> &b) {return a.second < b.second; });
	}

	std::vector<std::pair<S, double>> m_materialCTNumbers;
	std::vector<double> m_calibrationEnergy;
	std::vector<double> m_calibrationDensity;
	std::vector<double> m_materialEnergy;
	std::vector<double> m_materialDensity;
};





/*class DataExportContainer :public QObject
{
	Q_OBJECT
public:
	DataExportContainer(QObject* parent=nullptr) :QObject(parent){}
	std::shared_ptr<DensityImageContainer> m_density;
	std::shared_ptr<OrganImageContainer> m_organMap;
	std::shared_ptr<MaterialImageContainer> m_materialMap;
	std::shared_ptr<DoseImageContainer> m_dose;
	std::vector<Material> m_materials;

};*/



class ImageImportPipeline : public QObject
{
	Q_OBJECT
public:
	ImageImportPipeline(QObject *parent = nullptr);
	void setDicomData(QStringList dicomPaths);
	void setOutputSpacing(const double* spacing);
	void setUseOutputSpacing(bool value) { m_useOutputSpacing = value; };
	void setBlurRadius(const double* radius);

	void setCTImportMaterialMap(const std::vector<Material>& map) { m_ctImportMaterialMap = map; }
	void setCTImportAqusitionVoltage(double voltage) { m_tube.setVoltage(voltage); }
	void setCTImportAqusitionAlFiltration(double mm) { m_tube.setAlFiltration(mm); }
	void setCTImportAqusitionCuFiltration(double mm) { m_tube.setCuFiltration(mm); }

	void importICRUMalePhantom(void);
	void importICRUFemalePhantom(void);
	void importCTDIPhantom(int mm);

	//void importH5File(const QString& path);
	//void exportH5File(const QString& path, const DataExportContainer data);


signals:
	void processingDataStarted();
	void processingDataEnded();
	void imageDataChanged(std::shared_ptr<ImageContainer> imageData);
	void materialDataChanged(std::vector<Material>& materials);
	void organDataChanged(std::vector<std::string>& organs);
	void aecFilterChanged(QString name, std::shared_ptr<AECFilter> filter);
protected:
	template<class Iter>
	std::pair<std::shared_ptr<std::vector<unsigned char>>, std::shared_ptr<std::vector<double>>> calculateMaterialAndDensityFromCTData(Iter first, Iter last);
	void processCTData(std::shared_ptr<ImageContainer> ctImage, const std::pair<std::string,std::vector<double>>& exposureData);
	std::pair<std::string, std::vector<double>> readExposureData(vtkSmartPointer<vtkDICOMReader>& dicomReader);
	
private:
	std::array<double, 3> m_outputSpacing = { 2, 2, 2 };
	std::array<double, 3> m_blurRadius = { 1, 1, 1 };
	bool m_useOutputSpacing = false;
	Tube m_tube;
	std::vector<Material> m_ctImportMaterialMap;
	
};