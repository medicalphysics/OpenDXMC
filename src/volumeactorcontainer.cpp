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

#include "opendxmc/volumeactorcontainer.h"

#include "dxmc/vectormath.h"

#include <vtkCellData.h>
#include <vtkMatrix4x4.h>
#include <vtkNamedColors.h>
#include <vtkProperty.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkUnsignedCharArray.h>
#include <vtkXMLPolyDataReader.h>

#include <string>

VolumeActorContainer::VolumeActorContainer()
{
    m_actor = vtkSmartPointer<vtkActor>::New();
    m_userTransform = vtkSmartPointer<vtkMatrixToLinearTransform>::New();
    m_userMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    m_userTransform->SetInput(m_userMatrix);
    m_actor->SetUserTransform(m_userTransform);
    m_actor->SetDragable(true);
    m_actor->SetPickable(true);
}

void VolumeActorContainer::setOrientation(const std::array<double, 6>& directionCosines)
{
    m_userMatrix->Identity();
    double z[3];
    vectormath::cross(directionCosines.data(), z);
    for (int i = 0; i < 3; ++i) {
        m_userMatrix->SetElement(i, 0, directionCosines[i]);
        m_userMatrix->SetElement(i, 1, directionCosines[i + 3]);
        m_userMatrix->SetElement(i, 2, z[i]);
    }
    m_userMatrix->Invert();
}

OrientationActorContainer::OrientationActorContainer()
    : VolumeActorContainer()
{
    vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
    std::string filename("resources/Human.vtp");
    reader->SetFileName(filename.c_str());

    m_humanMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_humanMapper->SetInputConnection(reader->GetOutputPort());
    m_humanMapper->SetScalarModeToUsePointFieldData();
    m_humanMapper->SelectColorArray("Color");
    m_humanMapper->SetColorModeToDirectScalars();
    m_humanMapper->Update();

    vtkSmartPointer<vtkActor> humanActor = getActor();
    humanActor->SetMapper(m_humanMapper);
    humanActor->SetPosition(0, 0, 0);
    humanActor->SetScale(1.0);
}
void OrientationActorContainer::update()
{
}

void OrientationActorContainer::setOrientation(const std::array<double, 6>& directionCosines)
{
    // we need this overload to correct the reverse Y direction of our orientationmarker
    VolumeActorContainer::setOrientation(directionCosines);
    auto matrix = getMatrix();
    for (int i = 0; i < 3; ++i) {
        auto val = matrix->GetElement(i, 1);
        matrix->SetElement(i, 1, -val);
    }
}

SourceActorContainer::SourceActorContainer(Source* src)
    : VolumeActorContainer()
    , m_src(src)
{
}

void SourceActorContainer::applyActorTranslationToSource()
{
    auto pos = m_src->position();
    for (int i = 0; i < 3; ++i) {
        pos[i] += m_userMatrix->GetElement(i, 3);
        m_userMatrix->SetElement(i, 3, 0.0); // setting zeroes since the source position changes
    }
    m_src->setPosition(pos);
    update();
}

//https://vtk.org/Wiki/VTK/Examples/Cxx/VisualizationAlgorithms/TubesWithVaryingRadiusAndColors

DXSourceContainer::DXSourceContainer(std::shared_ptr<DXSource> src)
    : SourceActorContainer(src.get())
    , m_src(src)
{
    m_linesPolyData = vtkSmartPointer<vtkPolyData>::New();

    //test points
    const auto origin = m_src->tubePosition();
    std::array<double, 3> direction;
    const auto& cosines = m_src->directionCosines();
    vectormath::cross(cosines.data(), direction.data());

    const double lenght = src->sourceDetectorDistance();
    std::array<double, 3> p0, p1, p2, p3;
    const auto& angles = m_src->collimationAngles();
    for (int i = 0; i < 3; ++i) {
        p0[i] = origin[i] + lenght * (direction[i] + std::tan(angles[0] * .5) * cosines[i] + std::tan(angles[1] * .5) * cosines[i + 3]);
        p1[i] = origin[i] + lenght * (direction[i] + std::tan(angles[0] * .5) * cosines[i] - std::tan(angles[1] * .5) * cosines[i + 3]);
        p2[i] = origin[i] + lenght * (direction[i] - std::tan(angles[0] * .5) * cosines[i] + std::tan(angles[1] * .5) * cosines[i + 3]);
        p3[i] = origin[i] + lenght * (direction[i] - std::tan(angles[0] * .5) * cosines[i] - std::tan(angles[1] * .5) * cosines[i + 3]);
    }

    // Create a vtkPoints container and store the points in it
    m_points = vtkSmartPointer<vtkPoints>::New();
    m_points->InsertNextPoint(origin.data());
    m_points->InsertNextPoint(p0.data());
    m_points->InsertNextPoint(p1.data());
    m_points->InsertNextPoint(p2.data());
    m_points->InsertNextPoint(p3.data());
    m_linesPolyData->SetPoints(m_points);

    m_line1 = vtkSmartPointer<vtkLine>::New();
    m_line1->GetPointIds()->SetId(0, 0);
    m_line1->GetPointIds()->SetId(1, 1);

    m_line2 = vtkSmartPointer<vtkLine>::New();
    m_line2->GetPointIds()->SetId(0, 0);
    m_line2->GetPointIds()->SetId(1, 2);

    m_line3 = vtkSmartPointer<vtkLine>::New();
    m_line3->GetPointIds()->SetId(0, 0);
    m_line3->GetPointIds()->SetId(1, 3);

    m_line4 = vtkSmartPointer<vtkLine>::New();
    m_line4->GetPointIds()->SetId(0, 0);
    m_line4->GetPointIds()->SetId(1, 4);

    m_lines = vtkSmartPointer<vtkCellArray>::New();
    m_lines->InsertNextCell(m_line1);
    m_lines->InsertNextCell(m_line2);
    m_lines->InsertNextCell(m_line3);
    m_lines->InsertNextCell(m_line4);

    m_linesPolyData->SetLines(m_lines);

    vtkSmartPointer<vtkNamedColors> namedColors = vtkSmartPointer<vtkNamedColors>::New();
    auto m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    m_colors->SetNumberOfComponents(4);
    m_colors->InsertNextTypedTuple(namedColors->GetColor4ub("Tomato").GetData());
    m_colors->InsertNextTypedTuple(namedColors->GetColor4ub("Mint").GetData());
    m_colors->InsertNextTypedTuple(namedColors->GetColor4ub("Tomato").GetData());
    m_colors->InsertNextTypedTuple(namedColors->GetColor4ub("Mint").GetData());

    m_linesPolyData->GetCellData()->SetScalars(m_colors);

    m_tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    m_tubeFilter->SetRadius(8.0);
    m_tubeFilter->SetNumberOfSides(16);

    m_tubeFilter->SetInputData(m_linesPolyData);

    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_mapper->SetInputConnection(m_tubeFilter->GetOutputPort());
    getActor()->SetMapper(m_mapper);
}

void DXSourceContainer::update()
{

    const auto& origin = m_src->tubePosition();
    std::array<double, 3> direction;
    const auto& cosines = m_src->directionCosines();
    vectormath::cross(cosines.data(), direction.data());

    double lenght = m_src->sourceDetectorDistance();
    std::array<double, 3> p0, p1, p2, p3;
    const auto& angles = m_src->collimationAngles();
    for (int i = 0; i < 3; ++i) {
        p0[i] = origin[i] + lenght * (direction[i] + std::tan(angles[0] * .5) * cosines[i] + std::tan(angles[1] * .5) * cosines[i + 3]);
        p1[i] = origin[i] + lenght * (direction[i] + std::tan(angles[0] * .5) * cosines[i] - std::tan(angles[1] * .5) * cosines[i + 3]);
        p2[i] = origin[i] + lenght * (direction[i] - std::tan(angles[0] * .5) * cosines[i] + std::tan(angles[1] * .5) * cosines[i + 3]);
        p3[i] = origin[i] + lenght * (direction[i] - std::tan(angles[0] * .5) * cosines[i] - std::tan(angles[1] * .5) * cosines[i + 3]);
    }

    // Create a vtkPoints container and store the points in it
    m_points->Reset();
    m_points->InsertNextPoint(origin.data());
    m_points->InsertNextPoint(p0.data());
    m_points->InsertNextPoint(p1.data());
    m_points->InsertNextPoint(p2.data());
    m_points->InsertNextPoint(p3.data());
    m_linesPolyData->SetPoints(m_points);
    m_tubeFilter->Update();
}

CTSpiralSourceContainer::CTSpiralSourceContainer(std::shared_ptr<CTSpiralSource> src)
    : SourceActorContainer(src.get())
    , m_src(src)
{
    m_points = vtkSmartPointer<vtkPoints>::New();
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_linesPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    m_mapper->SetInputConnection(m_tubeFilter->GetOutputPort());
    m_polyLine = vtkSmartPointer<vtkPolyLine>::New();
    m_tubeFilter->SetRadius(8.0);
    m_tubeFilter->SetNumberOfSides(16);
    m_tubeFilter->CappingOn();
    update();
}

void CTSpiralSourceContainer::update()
{

    //generate points
    m_points->Reset();
    m_linesPolyData->Reset();
    const int nPoints = static_cast<int>(m_src->totalExposures());
    m_points->SetNumberOfPoints(nPoints + 4);

    Exposure exp;
    for (int i = 0; i < nPoints; ++i) {
        m_src->getExposure(exp, i);
        auto pos = exp.position();
        m_points->SetPoint(i, pos[0], pos[1], pos[2]);
    }

    std::array<double, 3> p0, p1, p2, p3;
    m_src->getExposure(exp, 0);
    const auto start = exp.position();
    const auto cosines = exp.directionCosines();
    const auto direction = exp.beamDirection();

    const double sdd = m_src->sourceDetectorDistance();

    auto angles = exp.collimationAngles();
    double lenght = std::sqrt(sdd * sdd * 0.25 + m_src->fieldOfView() * m_src->fieldOfView() * 0.25);
    for (int i = 0; i < 3; ++i) {
        p0[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p1[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p2[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p3[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
    }

    m_points->SetPoint(nPoints, p0[0], p0[1], p0[2]);
    m_points->SetPoint(nPoints + 1, p1[0], p1[1], p1[2]);
    m_points->SetPoint(nPoints + 2, p2[0], p2[1], p2[2]);
    m_points->SetPoint(nPoints + 3, p3[0], p3[1], p3[2]);
    m_linesPolyData->SetPoints(m_points);

    m_polyLine->GetPointIds()->SetNumberOfIds(nPoints);
    for (int i = 0; i < nPoints; ++i)
        m_polyLine->GetPointIds()->SetId(i, i);
    m_line1 = vtkSmartPointer<vtkLine>::New();
    m_line1->GetPointIds()->SetId(0, 0);
    m_line1->GetPointIds()->SetId(1, nPoints);
    m_line2 = vtkSmartPointer<vtkLine>::New();
    m_line2->GetPointIds()->SetId(0, 0);
    m_line2->GetPointIds()->SetId(1, nPoints + 1);

    m_line3 = vtkSmartPointer<vtkLine>::New();
    m_line3->GetPointIds()->SetId(0, 0);
    m_line3->GetPointIds()->SetId(1, nPoints + 2);

    m_line4 = vtkSmartPointer<vtkLine>::New();
    m_line4->GetPointIds()->SetId(0, 0);
    m_line4->GetPointIds()->SetId(1, nPoints + 3);

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(m_polyLine);
    cells->InsertNextCell(m_line1);
    cells->InsertNextCell(m_line2);
    cells->InsertNextCell(m_line3);
    cells->InsertNextCell(m_line4);

    m_linesPolyData->SetLines(cells);

    vtkSmartPointer<vtkNamedColors> namedColors = vtkSmartPointer<vtkNamedColors>::New();
    vtkSmartPointer<vtkUnsignedCharArray> m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    m_colors->SetNumberOfComponents(3);
    m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("Tomato").GetData());
    for (int i = 0; i < 4; ++i)
        m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("Mint").GetData());

    m_linesPolyData->GetCellData()->SetScalars(m_colors);
    m_tubeFilter->SetInputData(m_linesPolyData);
    m_tubeFilter->Update();
    getActor()->SetMapper(m_mapper);
}

CTAxialSourceContainer::CTAxialSourceContainer(std::shared_ptr<CTAxialSource> src)
    : SourceActorContainer(src.get())
    , m_src(src)
{
    m_points = vtkSmartPointer<vtkPoints>::New();
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_linesPolyData = vtkSmartPointer<vtkPolyData>::New();
    m_tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    m_mapper->SetInputConnection(m_tubeFilter->GetOutputPort());
    m_polyLine = vtkSmartPointer<vtkPolyLine>::New();
    m_tubeFilter->SetRadius(8.0);
    m_tubeFilter->SetNumberOfSides(16);
    m_tubeFilter->CappingOn();
    update();
}

void CTAxialSourceContainer::update()
{
    //generate points
    m_points->Reset();
    m_linesPolyData->Reset();
    const int nPoints = static_cast<int>(m_src->totalExposures());
    m_points->SetNumberOfPoints(nPoints + 4);
    Exposure exp;
    for (int i = 0; i < nPoints; ++i) {
        m_src->getExposure(exp, i);
        auto pos = exp.position();
        m_points->SetPoint(i, pos[0], pos[1], pos[2]);
    }

    m_src->getExposure(exp, 0);
    std::array<double, 3> p0, p1, p2, p3;
    auto start = exp.position();
    auto cosines = exp.directionCosines();
    auto direction = exp.beamDirection();

    double sdd = m_src->sourceDetectorDistance();

    auto angles = exp.collimationAngles();
    double lenght = std::sqrt(sdd * sdd * 0.25 + m_src->fieldOfView() * m_src->fieldOfView() * 0.25);
    for (int i = 0; i < 3; ++i) {
        p0[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p1[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p2[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p3[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
    }

    m_points->SetPoint(nPoints, p0[0], p0[1], p0[2]);
    m_points->SetPoint(nPoints + 1, p1[0], p1[1], p1[2]);
    m_points->SetPoint(nPoints + 2, p2[0], p2[1], p2[2]);
    m_points->SetPoint(nPoints + 3, p3[0], p3[1], p3[2]);
    m_linesPolyData->SetPoints(m_points);

    m_polyLine->GetPointIds()->SetNumberOfIds(nPoints);
    for (int i = 0; i < nPoints; ++i)
        m_polyLine->GetPointIds()->SetId(i, i);
    m_line1 = vtkSmartPointer<vtkLine>::New();
    m_line1->GetPointIds()->SetId(0, 0);
    m_line1->GetPointIds()->SetId(1, nPoints);
    m_line2 = vtkSmartPointer<vtkLine>::New();
    m_line2->GetPointIds()->SetId(0, 0);
    m_line2->GetPointIds()->SetId(1, nPoints + 1);

    m_line3 = vtkSmartPointer<vtkLine>::New();
    m_line3->GetPointIds()->SetId(0, 0);
    m_line3->GetPointIds()->SetId(1, nPoints + 2);

    m_line4 = vtkSmartPointer<vtkLine>::New();
    m_line4->GetPointIds()->SetId(0, 0);
    m_line4->GetPointIds()->SetId(1, nPoints + 3);

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(m_polyLine);
    cells->InsertNextCell(m_line1);
    cells->InsertNextCell(m_line2);
    cells->InsertNextCell(m_line3);
    cells->InsertNextCell(m_line4);

    m_linesPolyData->SetLines(cells);

    vtkSmartPointer<vtkNamedColors> namedColors = vtkSmartPointer<vtkNamedColors>::New();
    auto m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    m_colors->SetNumberOfComponents(3);
    m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("Tomato").GetData());
    for (int i = 0; i < 4; ++i)
        m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("Mint").GetData());

    m_linesPolyData->GetCellData()->SetScalars(m_colors);

    m_tubeFilter->SetInputData(m_linesPolyData);
    m_tubeFilter->Update();
    getActor()->SetMapper(m_mapper);
}

CTDualSourceContainer::CTDualSourceContainer(std::shared_ptr<CTDualSource> src)
    : SourceActorContainer(src.get())
    , m_src(src)
{
    m_pointsA = vtkSmartPointer<vtkPoints>::New();
    m_pointsB = vtkSmartPointer<vtkPoints>::New();
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_linesPolyDataA = vtkSmartPointer<vtkPolyData>::New();
    m_linesPolyDataB = vtkSmartPointer<vtkPolyData>::New();
    m_tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    m_mapper->SetInputConnection(m_tubeFilter->GetOutputPort());
    m_polyLineA = vtkSmartPointer<vtkPolyLine>::New();
    m_polyLineB = vtkSmartPointer<vtkPolyLine>::New();
    m_appendFilter = vtkSmartPointer<vtkAppendPolyData>::New();
    m_tubeFilter->SetRadius(8.0);
    m_tubeFilter->SetNumberOfSides(16);
    m_tubeFilter->CappingOn();
    m_tubeFilter->SetInputConnection(m_appendFilter->GetOutputPort());
    m_appendFilter->AddInputData(m_linesPolyDataA);
    m_appendFilter->AddInputData(m_linesPolyDataB);
    getActor()->SetMapper(m_mapper);
    update();
}

void CTDualSourceContainer::update()
{
    //generate points
    updateTubeA();
    updateTubeB();

    m_tubeFilter->Update();
    getActor()->SetMapper(m_mapper);
}

void CTDualSourceContainer::updateTubeA()
{
    m_pointsA->Reset();
    m_linesPolyDataA->Reset();
    const int nPoints = static_cast<int>(m_src->totalExposures()) / 2;
    m_pointsA->SetNumberOfPoints(nPoints + 4);
    Exposure exp;
    for (int i = 0; i < nPoints; ++i) {
        m_src->getExposure(exp, i * 2);
        auto pos = exp.position();
        m_pointsA->SetPoint(i, pos[0], pos[1], pos[2]);
    }

    m_src->getExposure(exp, 0);
    std::array<double, 3> p0, p1, p2, p3;
    auto start = exp.position();
    auto cosines = exp.directionCosines();
    auto direction = exp.beamDirection();

    double sdd = m_src->sourceDetectorDistance();

    auto angles = exp.collimationAngles();

    double lenght = std::sqrt(sdd * sdd * 0.25 + m_src->fieldOfView() * m_src->fieldOfView() * 0.25);
    for (int i = 0; i < 3; ++i) {
        p0[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p1[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p2[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p3[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
    }

    m_pointsA->SetPoint(nPoints, p0[0], p0[1], p0[2]);
    m_pointsA->SetPoint(nPoints + 1, p1[0], p1[1], p1[2]);
    m_pointsA->SetPoint(nPoints + 2, p2[0], p2[1], p2[2]);
    m_pointsA->SetPoint(nPoints + 3, p3[0], p3[1], p3[2]);
    m_linesPolyDataA->SetPoints(m_pointsA);

    m_polyLineA->GetPointIds()->SetNumberOfIds(nPoints);
    for (int i = 0; i < nPoints; ++i)
        m_polyLineA->GetPointIds()->SetId(i, i);
    m_line1A = vtkSmartPointer<vtkLine>::New();
    m_line1A->GetPointIds()->SetId(0, 0);
    m_line1A->GetPointIds()->SetId(1, nPoints);
    m_line2A = vtkSmartPointer<vtkLine>::New();
    m_line2A->GetPointIds()->SetId(0, 0);
    m_line2A->GetPointIds()->SetId(1, nPoints + 1);

    m_line3A = vtkSmartPointer<vtkLine>::New();
    m_line3A->GetPointIds()->SetId(0, 0);
    m_line3A->GetPointIds()->SetId(1, nPoints + 2);

    m_line4A = vtkSmartPointer<vtkLine>::New();
    m_line4A->GetPointIds()->SetId(0, 0);
    m_line4A->GetPointIds()->SetId(1, nPoints + 3);

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(m_polyLineA);
    cells->InsertNextCell(m_line1A);
    cells->InsertNextCell(m_line2A);
    cells->InsertNextCell(m_line3A);
    cells->InsertNextCell(m_line4A);

    m_linesPolyDataA->SetLines(cells);

    vtkSmartPointer<vtkNamedColors> namedColors = vtkSmartPointer<vtkNamedColors>::New();
    vtkSmartPointer<vtkUnsignedCharArray> m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    m_colors->SetNumberOfComponents(3);
    m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("Tomato").GetData());
    for (int i = 0; i < 4; ++i)
        m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("Mint").GetData());

    m_linesPolyDataA->GetCellData()->SetScalars(m_colors);
}

void CTDualSourceContainer::updateTubeB()
{
    m_pointsB->Reset();
    m_linesPolyDataB->Reset();
    const int nPoints = static_cast<int>(m_src->totalExposures()) / 2;
    m_pointsB->SetNumberOfPoints(nPoints + 4);
    Exposure exp;
    for (int i = 0; i < nPoints; ++i) {
        m_src->getExposure(exp, 2 * i + 1);
        auto pos = exp.position();
        m_pointsB->SetPoint(i, pos[0], pos[1], pos[2]);
    }

    m_src->getExposure(exp, 1);
    std::array<double, 3> p0, p1, p2, p3;
    auto start = exp.position();
    auto direction = exp.beamDirection();
    auto cosines = exp.directionCosines();
    double sdd = m_src->sourceDetectorDistanceB();
    auto angles = exp.collimationAngles();
    double lenght = std::sqrt(sdd * sdd * 0.25 + m_src->fieldOfViewB() * m_src->fieldOfViewB() * 0.25);

    for (int i = 0; i < 3; ++i) {
        p0[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p1[i] = start[i] + lenght * (direction[i] + std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p2[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] + std::tan(angles[1] * 0.5) * cosines[i + 3]);
        p3[i] = start[i] + lenght * (direction[i] - std::tan(angles[0] * 0.5) * cosines[i] - std::tan(angles[1] * 0.5) * cosines[i + 3]);
    }

    m_pointsB->SetPoint(nPoints, p0[0], p0[1], p0[2]);
    m_pointsB->SetPoint(nPoints + 1, p1[0], p1[1], p1[2]);
    m_pointsB->SetPoint(nPoints + 2, p2[0], p2[1], p2[2]);
    m_pointsB->SetPoint(nPoints + 3, p3[0], p3[1], p3[2]);
    m_linesPolyDataB->SetPoints(m_pointsB);

    m_polyLineB->GetPointIds()->SetNumberOfIds(nPoints);
    for (int i = 0; i < nPoints; ++i)
        m_polyLineB->GetPointIds()->SetId(i, i);
    m_line1B = vtkSmartPointer<vtkLine>::New();
    m_line1B->GetPointIds()->SetId(0, 0);
    m_line1B->GetPointIds()->SetId(1, nPoints);
    m_line2B = vtkSmartPointer<vtkLine>::New();
    m_line2B->GetPointIds()->SetId(0, 0);
    m_line2B->GetPointIds()->SetId(1, nPoints + 1);

    m_line3B = vtkSmartPointer<vtkLine>::New();
    m_line3B->GetPointIds()->SetId(0, 0);
    m_line3B->GetPointIds()->SetId(1, nPoints + 2);

    m_line4B = vtkSmartPointer<vtkLine>::New();
    m_line4B->GetPointIds()->SetId(0, 0);
    m_line4B->GetPointIds()->SetId(1, nPoints + 3);

    vtkSmartPointer<vtkCellArray> cells = vtkSmartPointer<vtkCellArray>::New();
    cells->InsertNextCell(m_polyLineB);
    cells->InsertNextCell(m_line1B);
    cells->InsertNextCell(m_line2B);
    cells->InsertNextCell(m_line3B);
    cells->InsertNextCell(m_line4B);

    m_linesPolyDataB->SetLines(cells);

    vtkSmartPointer<vtkNamedColors> namedColors = vtkSmartPointer<vtkNamedColors>::New();
    vtkSmartPointer<vtkUnsignedCharArray> m_colors = vtkSmartPointer<vtkUnsignedCharArray>::New();
    m_colors->SetNumberOfComponents(3);
    m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("SpringGreen").GetData());
    for (int i = 0; i < 4; ++i)
        m_colors->InsertNextTypedTuple(namedColors->GetColor3ub("Gold").GetData());

    m_linesPolyDataB->GetCellData()->SetScalars(m_colors);
}
