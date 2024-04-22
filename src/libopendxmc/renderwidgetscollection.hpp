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

#include <beamactorcontainer.hpp>
#include <datacontainer.hpp>
#include <slicerenderwidget.hpp>
#include <volumerenderwidget.hpp>

#include <QColor>
#include <QComboBox>
#include <QWidget>

#include <vtkActor.h>
#include <vtkSmartPointer.h>

#include <array>
#include <memory>
#include <vector>

class VolumerenderSettingsWidget;
class BeamActorContainer;
class RendersettingsWidget;

class RenderWidgetsCollection : public QWidget {
    Q_OBJECT

    struct BeamActorMonitor {
        BeamActorMonitor(std::shared_ptr<BeamActorContainer> v)
            : beamActorContainer(v)
        {
        }
        std::shared_ptr<BeamActorContainer> beamActorContainer = nullptr;
        std::vector<vtkSmartPointer<vtkActor>> actors;
        vtkSmartPointer<vtkActor> getNewActor()
        {
            auto actor = beamActorContainer->createActor();
            actors.push_back(actor);
            return actors.back();
        }
    };

public:
    RenderWidgetsCollection(QWidget* parent = nullptr);
    void updateImageData(std::shared_ptr<DataContainer>);
    void useFXAA(bool on);
    void setMultisampleAA(int samples);
    void setInteractionStyleToSlicing();
    void setInteractionStyleTo3D();
    void setInterpolationType(int type = 1);
    void addActor(std::shared_ptr<BeamActorContainer> actor);
    void removeActor(std::shared_ptr<BeamActorContainer> actor);
    void setBeamActorsVisible(int);
    void setBackgroundColor(const QColor& color);

    void Render();

    QWidget* createRendersettingsWidget(QWidget* parent = nullptr);

    void showData(DataContainer::ImageType type);

protected:
    VolumerenderSettingsWidget* volumerenderSettingsWidget(QWidget* parent = nullptr);

private:
    std::array<SliceRenderWidget*, 3> m_slice_widgets = { nullptr, nullptr, nullptr };
    VolumerenderWidget* m_volume_widget = nullptr;
    QComboBox* m_data_type_selector = nullptr;
    std::vector<BeamActorMonitor> m_beamActorsBuffer;
    bool m_show_beam_actors = true;
};
