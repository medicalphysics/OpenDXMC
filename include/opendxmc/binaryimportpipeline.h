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

#include <array>
#include <memory>
#include <vector>

#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/imagecontainer.h"

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

class BinaryImportPipeline : public QObject {
    Q_OBJECT
public:
    BinaryImportPipeline(QObject* parent = nullptr);
    void setDimension(const std::array<std::size_t, 3>& dimensions);
    void setDimension(int position, int value);
    void setSpacing(const std::array<double, 3>& spacing);
    void setSpacing(int position, double value);
    void setMaterialArrayPath(const QString& path);
    void setDensityArrayPath(const QString& path);
    void setMaterialMapPath(const QString& path);

signals:
    void processingDataStarted();
    void processingDataEnded();
    void imageDataChanged(std::shared_ptr<ImageContainer> imageData);
    void materialDataChanged(const std::vector<Material>& materials);
    void organDataChanged(const std::vector<std::string>& organs);
    void aecFilterChanged(const QString& name, std::shared_ptr<AECFilter> filter);
    void errorMessage(const QString& errorMsg) const;
    void resultsReady(bool ready);

protected:
    template <typename T>
    std::shared_ptr<std::vector<T>> readBinaryArray(const QString& path) const;
    void validate();

private:
    std::array<std::size_t, 3> m_dimensions = { 64, 64, 64 };
    std::array<double, 3> m_spacing = { 1, 1, 1 };
    std::shared_ptr<std::vector<float>> m_densityArray = nullptr;
    std::shared_ptr<std::vector<std::uint8_t>> m_materialArray = nullptr;
    std::vector<std::pair<std::uint8_t, Material>> m_materialMap;
};
