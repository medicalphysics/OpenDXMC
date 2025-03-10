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

class CTSegmentationPipeline : public BasePipeline {
    Q_OBJECT
public:
    CTSegmentationPipeline(QObject* parent = nullptr);

    void setAqusitionVoltage(double kv);
    void setAlFiltration(double mm);
    void setSnFiltration(double mm);
    void updateImageData(std::shared_ptr<DataContainer>) override;

private:
    double m_kv = 120;
    double m_Al_filt_mm = 0.9;
    double m_Sn_filt_mm = 0;
};
