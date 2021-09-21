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

#include "opendxmc/binaryimportpipeline.h"
#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/stringmanipulation.h"

#include <fstream>
#include <numeric>
#include <sstream>
#include <string>
#include <vector>

BinaryImportPipeline::BinaryImportPipeline(QObject* parent)
    : QObject(parent)
{
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
    qRegisterMetaType<std::vector<Material>>();
    qRegisterMetaType<std::vector<std::string>>();
}

void BinaryImportPipeline::setDimension(const std::array<std::size_t, 3>& dimensions)
{
    for (int i = 0; i < 3; ++i) {
        if (dimensions[i] == 0 || dimensions[i] > 2048)
            return;
    }
    m_dimensions = dimensions;
    validate();
}

void BinaryImportPipeline::setDimension(int position, int value)
{
    if (value <= 0 || value > 2048)
        return;
    m_dimensions[position] = static_cast<std::size_t>(value);
    validate();
}

void BinaryImportPipeline::setSpacing(const std::array<double, 3>& spacing)
{
    for (int i = 0; i < 3; ++i) {
        if (spacing[i] <= 0.0)
            return;
    }
    m_spacing = spacing;
    validate();
}

void BinaryImportPipeline::setSpacing(int position, double value)
{
    if (value <= 0)
        return;
    m_spacing[position] = value;
    validate();
}

template <typename T>
std::shared_ptr<std::vector<T>> BinaryImportPipeline::readBinaryArray(const QString& path) const
{
    auto std_path = path.toStdString();
    std::ifstream ifs(std_path, std::ios::binary | std::ios::ate);

    if (!ifs) {
        auto msq = QString(tr("Error opening file: ")) + path;
        emit errorMessage(msq);
        return nullptr;
    }

    auto end = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    auto size = std::size_t(end - ifs.tellg());

    auto dim_size = m_dimensions[0] * m_dimensions[1] * m_dimensions[2] * sizeof(T);
    if (dim_size != size) {
        auto msq = QString(tr("Image dimensions and file size do not match for: ")) + path;
        emit errorMessage(msq);
        return nullptr;
    }

    if (size == 0) // avoid undefined behavior
    {
        auto msq = QString(tr("Error reading file: ")) + path;
        emit errorMessage(msq);
        return nullptr;
    }

    const auto buffer_size = m_dimensions[0] * m_dimensions[1] * m_dimensions[2];
    auto buffer = std::make_shared<std::vector<T>>(buffer_size);

    if (!ifs.read(reinterpret_cast<char*>(buffer->data()), size)) {
        auto msq = QString(tr("Error reading file: ")) + path;
        emit errorMessage(msq);
        return nullptr;
    }
    return buffer;
}

void BinaryImportPipeline::setMaterialArrayPath(const QString& path)
{
    emit processingDataStarted();
    emit resultsReady(false);
    m_materialArray = readBinaryArray<unsigned char>(path);
    validate();
    emit processingDataEnded();
}

void BinaryImportPipeline::setDensityArrayPath(const QString& path)
{
    emit processingDataStarted();
    emit resultsReady(false);
    m_densityArray = readBinaryArray<float>(path);
    validate();
    emit processingDataEnded();
}

void BinaryImportPipeline::setMaterialMapPath(const QString& path)
{
    emit processingDataStarted();
    emit resultsReady(false);
    m_materialMap.clear();
    auto std_path = path.toStdString();
    std::ifstream ifs(std_path);

    if (!ifs) {
        auto msq = QString(tr("Error opening material map file: ")) + path;
        emit errorMessage(msq);
        emit processingDataEnded();
        return;
    }
    std::string str;
    while (std::getline(ifs, str)) {
        if (str.size() > 0) {
            auto strings = string_split(str, ';');
            for (std::size_t i = 0; i < strings.size(); ++i)
                strings[i] = string_trim(strings[i]);
            if (strings.size() >= 3) {
                std::uint8_t ind = 0;
                try {
                    ind = static_cast<std::uint8_t>(std::stoi(strings[0]));
                } catch (std::invalid_argument& e) {
                    auto msq = QString(tr("Error in material map file: ")) + path + QString(tr(" when parsing material number: ")) + QString::fromStdString(strings[0]);
                    msq += QString::fromStdString(e.what());
                    emit errorMessage(msq);
                    emit processingDataEnded();
                    return;
                } catch (std::out_of_range& e) {
                    auto msq = QString(tr("Error in material map file: ")) + path + QString(tr(" when parsing material number: ")) + QString::fromStdString(strings[0]);
                    msq += QString::fromStdString(e.what());
                    emit errorMessage(msq);
                    emit processingDataEnded();
                    return;
                }
                auto name = strings[1];
                auto composition = strings[2];
                Material material(composition, name);
                material.setStandardDensity(1.0);
                if (material.isValid()) {
                    // testing if index is taken
                    for (auto& [i, m] : m_materialMap) {
                        if (i == ind) {
                            auto msq = QString(tr("Error in material map file: ")) + path + QString(tr(" index is already occupied: ")) + QString::fromStdString(strings[0]);
                            emit errorMessage(msq);
                            emit processingDataEnded();
                            return;
                        }
                    }
                    m_materialMap.push_back(std::pair(ind, material));
                } else {
                    auto msg = QString(tr("Error in material map file: ")) + path + QString(tr(" Not able to parse material definition ")) + QString::fromStdString(strings[2]);
                    emit errorMessage(msg);
                    emit processingDataEnded();
                    return;
                }
            }
        }
    }
    std::sort(m_materialMap.begin(), m_materialMap.end(), [](auto& a, auto& b) { return a.first < b.first; });
    validate();
    emit processingDataEnded();
}

void BinaryImportPipeline::validate()
{
    emit resultsReady(false);
    emit errorMessage(QString(tr("")));
    //test for valid pointers
    if (!m_densityArray || !m_materialArray)
        return;
    //test for size matching
    if (m_densityArray->size() != m_materialArray->size())
        return;
    //test for dimensions matching
    std::size_t dim = std::accumulate(m_dimensions.begin(), m_dimensions.end(), static_cast<std::size_t>(1), std::multiplies<std::size_t>());
    if (m_materialArray->size() != dim)
        return;

    //finding uniqe elements in material array
    // we must make a copy :(
    std::vector<unsigned char> matInd(m_materialArray->cbegin(), m_materialArray->cend());
    std::sort(matInd.begin(), matInd.end());
    auto last = std::unique(matInd.begin(), matInd.end());
    matInd.erase(last, matInd.end());
    //making sure all indices have a corresponding material
    std::vector<unsigned char> matMapInd;
    matMapInd.reserve(matInd.size());
    for (auto& [ind, m] : m_materialMap) {
        matMapInd.push_back(ind);
    }
    std::vector<unsigned char> matInter;
    std::set_intersection(matInd.begin(), matInd.end(), matMapInd.begin(), matMapInd.end(), std::back_inserter(matInter));
    std::vector<unsigned char> matDiff;
    std::set_difference(matInd.begin(), matInd.end(), matInter.begin(), matInter.end(), std::back_inserter(matDiff));
    if (matDiff.size() != 0) {
        auto msg = QString(tr("Error: There is a mismatch between values in material array and material IDs in the material map file."));
        emit errorMessage(msg);
        return; // this means that there are material array values that does not have a corresponding material definition
    }
    //making sure all indices are consecutive
    for (std::uint8_t i = 0; i < matInd.size(); ++i) {
        if (i != matInd[i]) {
            //changing indices
            std::replace(m_materialArray->begin(), m_materialArray->end(), matInd[i], i);
            for (auto& [ind, m] : m_materialMap) {
                if (ind == matInd[i])
                    ind = i;
            }
        }
    }
    std::array<double, 3> origin;
    for (std::size_t i = 0; i < 3; ++i)
        origin[i] = -(m_dimensions[i] * m_spacing[i] * 0.5);
    //generating image containers

    auto densArray = std::make_shared<std::vector<floating>>(m_densityArray->cbegin(), m_densityArray->cend());
    auto densImage = std::make_shared<DensityImageContainer>(m_densityArray, m_dimensions, m_spacing, origin);
    auto matImage = std::make_shared<MaterialImageContainer>(m_materialArray, m_dimensions, m_spacing, origin);
    densImage->ID = ImageContainer::generateID();
    matImage->ID = densImage->ID;

    //make material definition array
    std::vector<Material> materials;
    materials.reserve(matInd.size());
    for (auto& [ind, m] : m_materialMap) {
        materials.push_back(m);
    }

    //emitting data
    emit errorMessage(QString(""));
    emit materialDataChanged(materials);
    emit imageDataChanged(densImage);
    emit imageDataChanged(matImage);
}
