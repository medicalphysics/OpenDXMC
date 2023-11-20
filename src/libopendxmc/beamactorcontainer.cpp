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

#include <beamactorcontainer.hpp>

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkLine.h>
#include <vtkNamedColors.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>

BeamActorContainer::BeamActorContainer(std::shared_ptr<Beam> beam_ptr)
    : m_beam(beam_ptr)
{
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

    m_actor = vtkSmartPointer<vtkActor>::New();
    m_actor->SetMapper(m_mapper);
    vtkNew<vtkNamedColors> colors;
    m_actor->GetProperty()->SetColor(colors->GetColor3d("Tomato").GetData());
    updateActor();
}
void BeamActorContainer::updateActor()
{
    if (!m_beam)
        return;
    auto points = vtkSmartPointer<vtkPoints>::New();

    std::visit([=](auto&& arg) {
        const auto N = arg.numberOfExposures();

        for (std::size_t i = 0; i < N; ++i) {
            points->InsertNextPoint(arg.exposure(i).position().data());
        }
    },
        *m_beam);

    auto polyLine = vtkSmartPointer<vtkPolyLine>::New();
    polyLine->GetPointIds()->SetNumberOfIds(points->GetNumberOfPoints());
    for (int i = 0; i < points->GetNumberOfPoints(); ++i) {
        polyLine->GetPointIds()->SetId(i, i);
    }

    // Create a cell array to store the lines in and add the lines to it.
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(polyLine);

    // Create a polydata to store everything in.
    m_polydata = vtkSmartPointer<vtkPolyData>::New();

    // Add the points to the dataset.
    m_polydata->SetPoints(points);

    // Add the lines to the dataset.
    m_polydata->SetLines(cells);

    m_mapper->SetInputData(m_polydata);
}