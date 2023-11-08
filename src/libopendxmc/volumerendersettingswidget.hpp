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

Copyright 2023 Erlend Andersen
*/
#pragma once

#include <datacontainer.hpp>
#include <volumelutwidget.hpp>
#include <volumerendersettings.hpp>

#include <QWidget>

#include <string>

class vtkImageData;

class VolumerenderSettingsWidget : public QWidget {
    Q_OBJECT
public:
    VolumerenderSettingsWidget(const VolumeRenderSettings& settings, QWidget* parent = nullptr);

    void setImageData(vtkImageData*);
    void setColorTable(const std::string&);

private:
    VolumeRenderSettings m_settings;
    VolumeLUTWidget* m_lut_opacity_widget = nullptr;
    VolumeLUTWidget* m_lut_gradient_widget = nullptr;
};
