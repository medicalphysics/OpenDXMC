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

Copyright 2024 Erlend Andersen
*/

#pragma once

#include <basepipeline.hpp>

#include <QStringList>

class CTImageImportPipeline : public BasePipeline {
public:
    CTImageImportPipeline(QObject* parent = nullptr);
    void updateImageData(std::shared_ptr<DataContainer>) override;
    void readImages(const QStringList& images);
    void setBlurRadius(const double*);
    void setOutputSpacing(const double*);
    void setUseOutputSpacing(bool);

private:
    std::array<double, 3> m_outputSpacing = { 0, 0, 0 };
    std::array<double, 3> m_blurRadius = { 0, 0, 0 };
    bool m_useOutputSpacing = false;
};