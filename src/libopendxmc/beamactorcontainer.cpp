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
#include <vtkNamedColors.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyLine.h>
#include <vtkProperty.h>
#include <vtkSmartPointer.h>

BeamActorContainer::BeamActorContainer(std::shared_ptr<Beam> beam_ptr)
    : m_beam(beam_ptr)
{
    m_polydata = vtkSmartPointer<vtkPolyData>::New();
}

std::shared_ptr<Beam> BeamActorContainer::getBeam()
{
    return m_beam;
}

std::array<std::array<double, 3>, 5> pointsFromCollimations(
    const std::array<double, 3>& start,
    const std::array<std::array<double, 3>, 2>& cosines,
    const std::array<double, 2>& angles,
    double scale = 1)
{
    std::array<std::array<double, 3>, 5> r;
    r[0] = start;

    const auto dir = dxmc::vectormath::cross(cosines[0], cosines[1]);
    const auto sinx_r = std::sin(angles[0]);
    const auto siny_r = std::sin(angles[1]);
    const auto sinz = std::sqrt(1 - sinx_r * sinx_r - siny_r * siny_r);
    constexpr std::array<int, 4> y_sign = { 1, -1, -1, 1 };

    for (std::size_t i = 0; i < 4; ++i) {
        const auto sinx = i < 2 ? sinx_r : -sinx_r;
        const auto siny = siny_r * y_sign[i];
        const auto sinz = std::sqrt(1 - sinx * sinx - siny * siny);
        std::array raw = {
            cosines[0][0] * sinx + cosines[1][0] * siny + dir[0] * sinz,
            cosines[0][1] * sinx + cosines[1][1] * siny + dir[1] * sinz,
            cosines[0][2] * sinx + cosines[1][2] * siny + dir[2] * sinz
        };
        r[i + 1] = dxmc::vectormath::add(start, dxmc::vectormath::scale(raw, scale));
    }
    return r;
}

void BeamActorContainer::update()
{
    if (!m_beam)
        return;

    // Create points storage
    auto points = vtkSmartPointer<vtkPoints>::New();

    // Create a cell array to store the lines in and add the lines to it.
    auto cells = vtkSmartPointer<vtkCellArray>::New();

    std::visit([points, cells](auto&& arg) {
        using U = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<U, CTSpiralBeam> || std::is_same_v<U, CBCTBeam> || std::is_same_v<U, CTSequentialBeam>) {
            const auto N = arg.numberOfExposures();
            cells->InsertNextCell(N);
            const auto exp_0 = arg.exposure(0);
            points->InsertNextPoint(exp_0.position().data());
            cells->InsertCellPoint(0);
            for (std::size_t i = 1; i < N; ++i) {
                points->InsertNextPoint(arg.exposure(i).position().data());
                cells->InsertCellPoint(i);
            }
            const auto pos = exp_0.position();
            const auto angles = exp_0.collimationAngles();
            const auto cosines = exp_0.directionCosines();
            auto p = pointsFromCollimations(pos, cosines, angles, arg.sourceDetectorDistance());
            for (std::size_t i = 0; i < p.size(); ++i) {
                auto& pi = p[i];
                points->InsertNextPoint(pi.data());
                if (i > 0) {
                    cells->InsertNextCell(2);
                    cells->InsertCellPoint(N + 0);
                    cells->InsertCellPoint(N + i);
                }
            }
            cells->InsertNextCell(5);
            cells->InsertCellPoint(N + 1);
            cells->InsertCellPoint(N + 2);
            cells->InsertCellPoint(N + 3);
            cells->InsertCellPoint(N + 4);
            cells->InsertCellPoint(N + 1);

        } else if constexpr (std::is_same_v<U, CTSpiralDualEnergyBeam>) {
            auto polyLineA = vtkSmartPointer<vtkPolyLine>::New();
            auto polyLineB = vtkSmartPointer<vtkPolyLine>::New();
            const auto N = arg.numberOfExposures();
            polyLineA->GetPointIds()->SetNumberOfIds(N / 2);
            polyLineB->GetPointIds()->SetNumberOfIds(N / 2);
            for (std::size_t i = 0; i < N; ++i) {
                points->InsertNextPoint(arg.exposure(i).position().data());
                if (i % 2 == 0)
                    polyLineA->GetPointIds()->SetId(i / 2, i);
                else
                    polyLineB->GetPointIds()->SetId(i / 2, i);
            }
            cells->InsertNextCell(polyLineA);
            cells->InsertNextCell(polyLineB);

        } else if constexpr (std::is_same_v<U, DXBeam>) {
            const auto pos = arg.position();
            const auto angles = arg.collimationAngles();
            const auto cosines = arg.directionCosines();
            const auto lenght = arg.sourceDetectorDistance();
            auto p = pointsFromCollimations(pos, cosines, angles, lenght);
            for (std::size_t i = 0; i < p.size(); ++i) {
                auto& pi = p[i];
                points->InsertNextPoint(pi.data());
                if (i > 0) {
                    cells->InsertNextCell(2);
                    cells->InsertCellPoint(0);
                    cells->InsertCellPoint(i);
                }
            }
            cells->InsertNextCell(5);
            cells->InsertCellPoint(1);
            cells->InsertCellPoint(2);
            cells->InsertCellPoint(3);
            cells->InsertCellPoint(4);
            cells->InsertCellPoint(1);
        }
    },
        *m_beam);

    // Add the points to the dataset.
    m_polydata->SetPoints(points);

    // Add the lines to the dataset.
    m_polydata->SetLines(cells);

    // Color the lines.
    // SetScalars() automatically associates the values in the data array passed
    // as parameter to the elements in the same indices of the cell data array on
    // which it is called. This means the first component (red) of the colors
    // array is matched with the first component of the cell array (line 0) and
    // the second component (green) of the colors array is matched with the second
    // component of the cell array (line 1).
    // m_polydata->GetCellData()->SetScalars(colors);

    // vtkNew<vtkNamedColors> namedColors;
    //     m_actor->GetProperty()->SetColor(colors->GetColor3d("Tomato").GetData());

    // Create a vtkUnsignedCharArray container and store the colors in it.
    // vtkNew<vtkUnsignedCharArray> colors;
    // colors->SetNumberOfComponents(3);
    // colors->InsertNextTypedTuple(namedColors->GetColor3ub("Tomato").GetData());
    // colors->InsertNextTypedTuple(namedColors->GetColor3ub("Mint").GetData());
}

vtkSmartPointer<vtkActor> BeamActorContainer::createActor()
{
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    auto actor = vtkSmartPointer<vtkActor>::New();
    mapper->SetInputData(m_polydata);
    actor->SetMapper(mapper);
    return actor;
}
