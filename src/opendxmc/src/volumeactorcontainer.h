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

#pragma once

#include "vectormath.h"
#include "source.h"

#include <vtkSmartPointer.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkLine.h>
#include <vtkActor.h>
#include <vtkMatrixToLinearTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkTubeFilter.h>
#include <vtkPolyLine.h>
#include <vtkAppendPolyData.h>

#include <array>
#include <memory>

class VolumeActorContainer
{
public:
	VolumeActorContainer();
	vtkSmartPointer<vtkActor> getActor() { return m_actor; }
	vtkMatrixToLinearTransform* getTransform() { return m_userTransform.GetPointer(); }
	vtkMatrix4x4* getMatrix() { return m_userMatrix.GetPointer(); }
	virtual void setOrientation(const std::array<double, 6> &directionCosines);
	virtual void update() = 0;
protected:

private:
	vtkSmartPointer<vtkActor> m_actor;
	vtkSmartPointer<vtkMatrixToLinearTransform> m_userTransform;
	vtkSmartPointer<vtkMatrix4x4> m_userMatrix;
};

class OrientationActorContainer :public VolumeActorContainer
{
public:
	OrientationActorContainer();
	virtual ~OrientationActorContainer() = default;
	void setOrientation(const std::array<double, 6> &directionCosines) override;
	void update() override;
protected:
private:
	vtkSmartPointer<vtkPolyDataMapper> m_humanMapper;
};

class DXSourceContainer :public VolumeActorContainer
{
public:
	DXSourceContainer(std::shared_ptr<DXSource> src);
	virtual ~DXSourceContainer() = default;
	//void setOrientation(const std::array<double, 6> &directionCosines) override;
	void update() override;
private:
	std::shared_ptr<DXSource> m_src;
	vtkSmartPointer<vtkPolyData> m_linesPolyData;
	vtkSmartPointer<vtkPoints> m_points;
	vtkSmartPointer<vtkLine> m_line1;
	vtkSmartPointer<vtkLine> m_line2;
	vtkSmartPointer<vtkLine> m_line3;
	vtkSmartPointer<vtkLine> m_line4;
	vtkSmartPointer<vtkCellArray> m_lines;
	vtkSmartPointer<vtkUnsignedCharArray> m_colors;
	vtkSmartPointer<vtkTubeFilter> m_tubeFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
};

class CTSpiralSourceContainer :public VolumeActorContainer
{
public:
	CTSpiralSourceContainer(std::shared_ptr<CTSpiralSource> src);
	virtual ~CTSpiralSourceContainer() = default;
	void update() override;
private:
	std::shared_ptr<CTSpiralSource> m_src;
	vtkSmartPointer<vtkPolyData> m_linesPolyData;
	vtkSmartPointer<vtkPoints> m_points;
	vtkSmartPointer<vtkPolyLine> m_polyLine;
	vtkSmartPointer<vtkLine> m_line1;
	vtkSmartPointer<vtkLine> m_line2;
	vtkSmartPointer<vtkLine> m_line3;
	vtkSmartPointer<vtkLine> m_line4;
	vtkSmartPointer<vtkTubeFilter> m_tubeFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
};
class CTAxialSourceContainer :public VolumeActorContainer
{
public:
	CTAxialSourceContainer(std::shared_ptr<CTAxialSource> src);
	virtual ~CTAxialSourceContainer() = default;
	void update() override;
private:
	std::shared_ptr<CTAxialSource> m_src;
	vtkSmartPointer<vtkPolyData> m_linesPolyData;
	vtkSmartPointer<vtkPoints> m_points;
	vtkSmartPointer<vtkPolyLine> m_polyLine;
	vtkSmartPointer<vtkLine> m_line1;
	vtkSmartPointer<vtkLine> m_line2;
	vtkSmartPointer<vtkLine> m_line3;
	vtkSmartPointer<vtkLine> m_line4;
	vtkSmartPointer<vtkTubeFilter> m_tubeFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
};


class CTDualSourceContainer :public VolumeActorContainer
{
public:
	CTDualSourceContainer(std::shared_ptr<CTDualSource> src);
	virtual ~CTDualSourceContainer() = default;
	void update() override;
private:
	void updateTubeA();
	void updateTubeB();
	std::shared_ptr<CTDualSource> m_src;
	vtkSmartPointer<vtkPolyData> m_linesPolyDataA;
	vtkSmartPointer<vtkPoints> m_pointsA;
	vtkSmartPointer<vtkPolyLine> m_polyLineA;
	vtkSmartPointer<vtkLine> m_line1A;
	vtkSmartPointer<vtkLine> m_line2A;
	vtkSmartPointer<vtkLine> m_line3A;
	vtkSmartPointer<vtkLine> m_line4A;
	vtkSmartPointer<vtkPolyData> m_linesPolyDataB;
	vtkSmartPointer<vtkPoints> m_pointsB;
	vtkSmartPointer<vtkPolyLine> m_polyLineB;
	vtkSmartPointer<vtkLine> m_line1B;
	vtkSmartPointer<vtkLine> m_line2B;
	vtkSmartPointer<vtkLine> m_line3B;
	vtkSmartPointer<vtkLine> m_line4B;
	vtkSmartPointer<vtkTubeFilter> m_tubeFilter;
	vtkSmartPointer<vtkPolyDataMapper> m_mapper;
	vtkSmartPointer<vtkAppendPolyData> m_appendFilter;
};
