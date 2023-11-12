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
#include <slicerenderwidget.hpp>
#include <volumerenderwidget.hpp>

#include <QWidget>
#include <QComboBox>

#include <array>

class VolumerenderSettingsWidget;

class RenderWidgetsCollection : public QWidget {
    Q_OBJECT

public:
    RenderWidgetsCollection(QWidget* parent = nullptr);
    void updateImageData(std::shared_ptr<DataContainer>);
    void useFXAA(bool on);
    void setMultisampleAA(int samples);
    void setInteractionStyleToSlicing();
    void setInteractionStyleTo3D();
    void setInterpolationType(int type = 1);

    VolumerenderSettingsWidget* volumerenderSettingsWidget(QWidget* parent = nullptr);
    QComboBox* getVolumeSelector();

    void showData(DataContainer::ImageType type);

private:
    std::array<SliceRenderWidget*, 3> m_slice_widgets = { nullptr, nullptr, nullptr };
    VolumerenderWidget* m_volume_widget = nullptr;
    QComboBox* m_data_type_selector = nullptr;
};