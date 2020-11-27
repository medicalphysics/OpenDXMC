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

Copyright 2020 Erlend Andersen
*/

#pragma once

#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/imagecontainer.h"

#include <QObject>
#include <QString>

#include <memory>
#include <string>
#include <vector>

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


class PhantomImportPipeline : public QObject {
    Q_OBJECT
public:
    PhantomImportPipeline(QObject* parent = nullptr);

    void importICRUMalePhantom(bool ignoreArms = false);
    void importICRUFemalePhantom(bool ignoreArms = false);
    void importCTDIPhantom(int mm, bool force_interaction_measurements = false);
    void importAWSPhantom(const QString& name);

signals:
    void processingDataStarted();
    void processingDataEnded();
    void imageDataChanged(std::shared_ptr<ImageContainer> imageData);
    void materialDataChanged(const std::vector<Material>& materials);
    void organDataChanged(const std::vector<std::string>& organs);
};