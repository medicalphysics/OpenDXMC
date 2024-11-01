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
#include <vtkProperty.h>
#include <vtkSmartPointer.h>
#include <vtkTubeFilter.h>

BeamActorContainer::BeamActorContainer(std::shared_ptr<Beam> beam_ptr)
    : m_beam(beam_ptr)
{
    m_polydata = vtkSmartPointer<vtkPolyData>::New();
    static unsigned int color = 0;
    m_colorIdx = color;
    ++color;
}

std::shared_ptr<Beam> BeamActorContainer::getBeam()
{
    return m_beam;
}

std::array<std::array<double, 3>, 4> pointsFromCollimations(
    const std::array<double, 3>& start,
    const std::array<std::array<double, 3>, 2>& cosines,
    const std::array<double, 2>& angles,
    double scale = 1)
{
    std::array<std::array<double, 3>, 4> r;

    const auto dir = dxmc::vectormath::cross(cosines[0], cosines[1]);
    const auto sinx_r = std::sin(angles[0]);
    const auto siny_r = std::sin(angles[1]);
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
        r[i] = dxmc::vectormath::add(start, dxmc::vectormath::scale(raw, scale));
    }
    return r;
}

std::array<double, 3> add(const std::array<double, 3>& v1, const std::array<double, 3>& v2)
{
    std::array<double, 3> v = {
        v1[0] + v2[0],
        v1[1] + v2[1],
        v1[2] + v2[2]
    };
    return v;
}

void BeamActorContainer::translate(const std::array<double, 3> dist)
{

    std::visit([&dist](auto&& arg) {
        using U = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<U, CBCTBeam>) {
            arg.setIsocenter(add(arg.isocenter(), dist));
        } else if constexpr (std::is_same_v<U, CTSpiralBeam> || std::is_same_v<U, CTSpiralDualEnergyBeam>) {
            arg.setStartPosition(add(arg.startPosition(), dist));
            arg.setStopPosition(add(arg.stopPosition(), dist));
        } else if constexpr (std::is_same_v<U, DXBeam>) {
            arg.setRotationCenter(add(dist, arg.rotationCenter()));
        } else if constexpr (std::is_same_v<U, CTSequentialBeam> || std::is_same_v<U, PencilBeam>) {
            arg.setPosition(add(dist, arg.position()));
        }
    },
        *m_beam);
    update();
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
            for (std::size_t i = 0; i < N; ++i) {
                points->InsertNextPoint(arg.exposure(i).position().data());
                cells->InsertCellPoint(i);
            }
            const auto exp = arg.exposure(0);
            const auto p = pointsFromCollimations(exp.position(), exp.directionCosines(), exp.collimationAngles(), arg.sourceDetectorDistance());
            for (std::size_t i = 0; i < p.size(); ++i) {
                points->InsertNextPoint(p[i].data());
                cells->InsertNextCell(2);
                cells->InsertCellPoint(0);
                cells->InsertCellPoint(N + i);
            }
            cells->InsertNextCell(5);
            cells->InsertCellPoint(N);
            cells->InsertCellPoint(N + 1);
            cells->InsertCellPoint(N + 2);
            cells->InsertCellPoint(N + 3);
            cells->InsertCellPoint(N);
        } else if constexpr (std::is_same_v<U, CTSpiralDualEnergyBeam>) {
            const auto ND = arg.numberOfExposures();
            const auto N = ND / 2;
            cells->InsertNextCell(N);
            for (std::size_t i = 0; i < N; ++i) {
                points->InsertNextPoint(arg.exposure(i * 2).position().data());
                cells->InsertCellPoint(i);
            }
            cells->InsertNextCell(N);
            for (std::size_t i = 0; i < N; ++i) {
                points->InsertNextPoint(arg.exposure(i * 2 + 1).position().data());
                cells->InsertCellPoint(i + N);
            }

            for (std::size_t t = 0; t < 2; ++t) {
                const auto exp = arg.exposure(t);
                const auto p = pointsFromCollimations(exp.position(), exp.directionCosines(), exp.collimationAngles(), arg.sourceDetectorDistance());
                const auto offset = t * p.size();
                for (std::size_t i = 0; i < p.size(); ++i) {
                    points->InsertNextPoint(p[i].data());
                    cells->InsertNextCell(2);
                    cells->InsertCellPoint(t * N);
                    cells->InsertCellPoint(ND + i + offset);
                }
                cells->InsertNextCell(5);
                cells->InsertCellPoint(ND + offset);
                cells->InsertCellPoint(ND + offset + 1);
                cells->InsertCellPoint(ND + offset + 2);
                cells->InsertCellPoint(ND + offset + 3);
                cells->InsertCellPoint(ND + offset);
            }
        } else if constexpr (std::is_same_v<U, DXBeam>) {
            const auto pos = arg.position();
            const auto angles = arg.collimationAngles();
            const auto cosines = arg.directionCosines();
            const auto lenght = arg.sourceDetectorDistance();
            const auto p = pointsFromCollimations(pos, cosines, angles, lenght);
            points->InsertNextPoint(pos.data());
            for (std::size_t i = 0; i < p.size(); ++i) {
                points->InsertNextPoint(p[i].data());
                cells->InsertNextCell(2);
                cells->InsertCellPoint(0);
                cells->InsertCellPoint(i + 1);
            }
            cells->InsertNextCell(5);
            cells->InsertCellPoint(1);
            cells->InsertCellPoint(2);
            cells->InsertCellPoint(3);
            cells->InsertCellPoint(4);
            cells->InsertCellPoint(1);
        } else if constexpr (std::is_same_v<U, PencilBeam>) {
            const auto& start = arg.position();
            const auto& dir = arg.direction();
            constexpr double lenght = 20;
            const auto stop = dxmc::vectormath::add(start, dxmc::vectormath::scale(dir, lenght));
            points->InsertNextPoint(start.data());
            points->InsertNextPoint(stop.data());
            cells->InsertNextCell(2);
            cells->InsertCellPoint(0);
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
/*
vtkSmartPointer<vtkActor> BeamActorContainer::createActor()
{
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    auto actor = vtkSmartPointer<vtkActor>::New();
    mapper->SetInputData(m_polydata);
    actor->SetMapper(mapper);
    actor->GetProperty()->SetLineWidth(m_lineThickness);
    actor->GetProperty()->RenderLinesAsTubesOn();
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    actor->GetProperty()->SetColor(colors->GetColor3d("Peacock").GetData());
    actor->SetDragable(true);
    return actor;
}*/

vtkSmartPointer<vtkActor> BeamActorContainer::createActor()
{
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    auto tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    auto actor = vtkSmartPointer<vtkActor>::New();
    tubeFilter->SetInputData(m_polydata);
    tubeFilter->SetRadius(m_lineThickness);
    tubeFilter->SetNumberOfSides(16);
    mapper->SetInputConnection(tubeFilter->GetOutputPort());
    actor->SetMapper(mapper);
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    if (const auto cind = m_colorIdx % 3; cind == 0)
        actor->GetProperty()->SetColor(colors->GetColor3d("Mint").GetData());
    else if (cind == 1)
        actor->GetProperty()->SetColor(colors->GetColor3d("Peacock").GetData());
    else
        actor->GetProperty()->SetColor(colors->GetColor3d("Tomato").GetData());

    actor->GetProperty()->SetOpacity(1.0);
    actor->SetDragable(true);

    return actor;
}
