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

#include <QObject>
#include <QString>

class SaveLoad : public QObject
{
	Q_OBJECT
public:
	SaveLoad(QObject* parent = nullptr);

	void loadFromFile(const QString& path);
	void saveToFile(const QString& path);
	void setImageData(std::shared_ptr<ImageContainer> image);
	void setMaterials(const std::vector<Material>& materials);
	void setOrganList(const std::vector<std::string>& organList) { m_organList = organList; }
signals:
	void processingDataStarted();
	void processingDataEnded();
	void imageDataChanged(std::shared_ptr<ImageContainer> image);
	void materialDataChanged(std::vector<Material>& materials);
	void organDataChanged(std::vector<std::string>& organs);
	void aecFilterChanged(QString name, std::shared_ptr<AECFilter> filter);
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
