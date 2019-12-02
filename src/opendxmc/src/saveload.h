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

#include "imagecontainer.h"
#include "material.h"
#include "source.h"
#include "dosereportcontainer.h"

#include <QObject>
#include <QString>


#ifndef Q_DECLARE_METATYPE_IMAGECONTAINER
#define Q_DECLARE_METATYPE_IMAGECONTAINER
Q_DECLARE_METATYPE(std::shared_ptr<ImageContainer>)
#endif 
#ifndef Q_DECLARE_METATYPE_MATERIALVECTOR
#define Q_DECLARE_METATYPE_MATERIALVECTOR
Q_DECLARE_METATYPE(std::vector<Material>)
#endif 
#ifndef Q_DECLARE_METATYPE_STRINGVECTOR
#define Q_DECLARE_METATYPE_STRINGVECTOR
Q_DECLARE_METATYPE(std::vector<std::string>)
#endif 
#ifndef Q_DECLARE_METATYPE_DOSEREPORTCONTAINER
#define Q_DECLARE_METATYPE_DOSEREPORTCONTAINER
Q_DECLARE_METATYPE(DoseReportContainer)
#endif 

class SaveLoad : public QObject
{
	Q_OBJECT
public:
	SaveLoad(QObject* parent = nullptr);

	void loadFromFile();
	void saveToFile();
	void setImageData(std::shared_ptr<ImageContainer> image);
	void setMaterials(const std::vector<Material>& materials);
	void setOrganList(const std::vector<std::string>& organList) { m_organList = organList; }
	void clear(void);
signals:
	void processingDataStarted();
	void processingDataEnded();
	void imageDataChanged(std::shared_ptr<ImageContainer> image);
	void materialDataChanged(const std::vector<Material>& materials);
	void organDataChanged(const std::vector<std::string>& organs);
	void doseDataChanged(const DoseReportContainer& doses);
	//void aecFilterChanged(QString name, std::shared_ptr<AECFilter> filter);
private:
	std::uint64_t m_currentImageID = 0;
	std::shared_ptr<ImageContainer> m_densityImage = nullptr;
	std::shared_ptr<ImageContainer> m_materialImage = nullptr;
	std::shared_ptr<ImageContainer> m_organImage = nullptr;
	std::shared_ptr<ImageContainer> m_ctImage = nullptr;
	std::shared_ptr<ImageContainer> m_doseImage = nullptr;
	std::vector<std::string> m_organList;
	std::vector<std::string> m_materialList;

};
