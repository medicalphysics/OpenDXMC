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

#include <dxmc_specialization.hpp>

#include <vtkPolyData.h>
#include <vtkSmartPointer.h>

#include <memory>

class vtkActor;

class BeamActorContainer {
public:
    BeamActorContainer(std::shared_ptr<Beam> beam);
    void update();
    vtkSmartPointer<vtkActor> createActor();
    std::shared_ptr<Beam> getBeam();

protected:
private:
    std::shared_ptr<Beam> m_beam = nullptr;
    vtkSmartPointer<vtkPolyData> m_polydata = nullptr;
};

#include <QMetaType>
// Allow std::shared_ptr<BeamActorContainer> to be used in signal/slots
Q_DECLARE_METATYPE(std::shared_ptr<BeamActorContainer>)