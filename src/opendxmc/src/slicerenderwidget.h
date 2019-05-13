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


#include "imagecontainer.h"

#include <QWidget>

#include <QVTKOpenGLWidget.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkRenderer.h>
#include <vtkTextActor.h>

#include <memory>
#include <map>
#include <array>


// https://github.com/Kitware/VTK/blob/master/Rendering/Image/Testing/Cxx/TestImageResliceMapperAlpha.cxx

class SliceRenderWidget : public QWidget
{
	Q_OBJECT
public:
	enum Orientation { Axial, Sagittal, Coronal };
	SliceRenderWidget(QWidget *parent = nullptr, Orientation orientation = Axial);

	void updateRendering();
	//void setImageData(std::shared_ptr<ImageContainer> volume);
	void setImageData(std::shared_ptr<ImageContainer> foreground, std::shared_ptr<ImageContainer> background=nullptr);
protected:
	std::array<double, 2> presetLeveling(ImageContainer::ImageType type);
	//void updateOrientation(void);
private:
	Orientation m_orientation;
	QVTKOpenGLWidget *m_openGLWidget;
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapper;
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapperBackground;
	vtkSmartPointer<vtkImageSlice> m_imageSlice;
	vtkSmartPointer<vtkImageSlice> m_imageSliceBackground;
	std::map<ImageContainer::ImageType, std::array<double, 2>> m_windowLevels;


	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkTextActor> m_textActor;
	std::shared_ptr<ImageContainer> m_image;
	std::shared_ptr<ImageContainer> m_imageBackground;
}; 


/*class SliceRenderWidget : public QWidget
{
	Q_OBJECT
public:
	enum Orientation { Axial, Sagittal, Coronal };
	SliceRenderWidget(QWidget *parent = nullptr, Orientation orientation = Axial);

	void updateRendering();
	void setImageData(ImageContainer volume);

private:
	Orientation m_orientation;
	QVTKOpenGLWidget *m_openGLWidget;
	vtkSmartPointer<vtkResliceImageViewer> m_resliceImageViewer;
	ImageContainer m_image;
	vtkSmartPointer<vtkWindowLevelLookupTable> m_lut;
};*/