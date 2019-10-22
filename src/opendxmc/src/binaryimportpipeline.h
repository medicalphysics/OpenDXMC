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

#include <QObject>

#include "imagecontainer.h"
#include "material.h"
#include "beamfilters.h"

#include <memory>
#include <vector>

class BinaryImportPipeline : public QObject
{
	Q_OBJECT
public:
	BinaryImportPipeline(QObject* parent = nullptr);
signals:
	void processingDataStarted();
	void processingDataEnded();
	void imageDataChanged(std::shared_ptr<ImageContainer> imageData);
	void materialDataChanged(std::vector<Material>& materials);
	void organDataChanged(std::vector<std::string>& organs);
	void aecFilterChanged(QString name, std::shared_ptr<AECFilter> filter);
};
