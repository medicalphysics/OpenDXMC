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
#include "slicerenderwidget.h"
#include "vectormath.h"
#include "colormap.h"

#include <QVBoxLayout>
#include <QColor>
#include <QPushButton>
#include <QIcon>
#include <QMenu>
#include <QColorDialog>
#include <QFileDialog>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkWindowLevelLookupTable.h>
#include <vtkLookupTable.h>
#include <vtkRendererCollection.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCamera.h>
#include <vtkPlane.h>
#include <vtkObjectFactory.h>
#include <vtkTextProperty.h>
#include <vtkImageProperty.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>
//#include <vtkFFMPEGWriter.h>

// Define interaction style
class customMouseInteractorStyle : public vtkInteractorStyleImage
{
public:
	static customMouseInteractorStyle* New();
	vtkTypeMacro(customMouseInteractorStyle, vtkInteractorStyleImage);

	virtual void OnLeftButtonDown()
	{
		std::cout << "Pressed left mouse button." << std::endl;
		// Forward events
		vtkInteractorStyleImage::OnLeftButtonDown();
	}

	virtual void OnMiddleButtonDown()
	{
		std::cout << "Pressed middle mouse button." << std::endl;
		// Forward events
		vtkInteractorStyleImage::OnMiddleButtonDown();
	}

	virtual void OnRightButtonDown()
	{
		std::cout << "Pressed right mouse button." << std::endl;
		// Forward events
		vtkInteractorStyleImage::OnRightButtonDown();
	}

	virtual void OnMouseWheelForward()
	{
		m_imageMapper->UpdateInformation();
		auto plane = m_imageMapper->GetSlicePlane();
		
		plane->Push(0.5);
		double* imbounds = m_imageMapper->GetBounds();
		double* origin = plane->GetOrigin();
		for (int i = 0; i < 3; ++i)
		{
			if (origin[i] > imbounds[i * 2 + 1])
				origin[i] = imbounds[i * 2];
			if (origin[i] < imbounds[i * 2])
				origin[i] = imbounds[i * 2 + 1];
		}
		plane->SetOrigin(origin);

		m_imageMapper->SetSlicePlane(plane);
		m_imageMapper->UpdateInformation();
		m_renderWindow->Render();

	}
	virtual void OnMouseWheelBackward()
	{

		m_imageMapper->UpdateInformation();
		auto plane = m_imageMapper->GetSlicePlane();

		plane->Push(-0.5);
		double* imbounds = m_imageMapper->GetBounds();
		double* origin = plane->GetOrigin();
		for (int i = 0; i < 3; ++i)
		{
			if (origin[i] > imbounds[i * 2 + 1])
				origin[i] = imbounds[i * 2];
			if (origin[i] < imbounds[i * 2])
				origin[i] = imbounds[i * 2 + 1];
		}
		plane->SetOrigin(origin);

		m_imageMapper->SetSlicePlane(plane);
		m_imageMapper->UpdateInformation();
		m_renderWindow->Render();

	}

	void OnMouseMove() override
	{
		vtkInteractorStyleImage::OnMouseMove();

		updateWLText();
	}

	void setMapper(vtkSmartPointer<vtkImageResliceMapper> m)
	{
		m_imageMapper = m;
	}
	void setRenderWindow(vtkSmartPointer<vtkRenderWindow> m)
	{
		m_renderWindow = m;
	}
	void setTextActor(vtkSmartPointer<vtkTextActor> textActor)
	{
		m_textActor = textActor;
	}
	static std::string prettyNumber(double number)
	{
		constexpr std::int32_t k = 3; // number decimals
		std::int32_t b = number * std::pow(10, k);
		auto bs = std::to_string(b);
		int c = static_cast<int>(bs.size() - k);
		if (c < 0)
			bs.insert(0, -c, '0');
		bs.insert(bs.size() - k, 1, '.');
		return bs;
	}
	void updateWLText()
	{
		auto prop = GetCurrentImageProperty();
		if (prop)
		{
			double l = prop->GetColorLevel();
			double w = prop->GetColorWindow();
			//m_text = std::to_string(l) + ", " + std::to_string(w);
			m_text = prettyNumber(l) + ", " + prettyNumber(w);
			m_textActor->SetInput(m_text.c_str());
		}
	}
private:
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapper;
	vtkSmartPointer<vtkRenderWindow> m_renderWindow;
	vtkSmartPointer<vtkTextActor> m_textActor;
	std::string m_text;
};
vtkStandardNewMacro(customMouseInteractorStyle);



SliceRenderWidget::SliceRenderWidget(QWidget *parent, Orientation orientation)
	:QWidget(parent), m_orientation(orientation)
{
	m_openGLWidget = new QVTKOpenGLWidget(this);
	
	auto layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_openGLWidget);
	this->setLayout(layout);

	
	
	//mapper 
	m_imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
	m_imageMapper->StreamingOn();

	m_imageSlice = vtkSmartPointer<vtkImageSlice>::New();
	m_imageSlice->SetMapper(m_imageMapper);

	//renderer
	// Setup renderers
	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_renderer->AddViewProp(m_imageSlice);
	m_renderer->UseFXAAOn();
	


	//render window
	//https://lorensen.github.io/VTKExamples/site/Cxx/Images/ImageSliceMapper/
	//http://vtk.org/gitweb?p=VTK.git;a=blob;f=Examples/GUI/Qt/FourPaneViewer/QtVTKRenderWindows.cxx
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
	renderWindow->AddRenderer(m_renderer);
	m_openGLWidget->SetRenderWindow(renderWindow);


	// Setup render window interactor
	vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

	vtkSmartPointer<customMouseInteractorStyle> style = vtkSmartPointer<customMouseInteractorStyle>::New();
	//style->SetInteractionModeToImageSlicing();
	style->setMapper(m_imageMapper);
	style->setRenderWindow(renderWindow);
	renderWindowInteractor->SetInteractorStyle(style);

	// Create the tekst widget
	m_textActor = vtkSmartPointer<vtkTextActor>::New();
	m_textActor->SetInput("");
	m_textActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
	m_renderer->AddActor(m_textActor);
	style->setTextActor(m_textActor);
	//setup collbacks
	



	// Render and start interaction
	renderWindowInteractor->SetRenderWindow(renderWindow);
	renderWindowInteractor->Initialize();


	vtkSmartPointer<vtkImageData> dummyData = vtkSmartPointer<vtkImageData>::New();
	dummyData->SetDimensions(30, 30, 30);
	dummyData->AllocateScalars(VTK_FLOAT, 1);
	m_imageMapper->SetInputData(dummyData);

	//other
	m_imageMapper->SliceFacesCameraOn();
	//m_imageSliceMapper->SliceAtFocalPointOn();

	if (auto cam = m_renderer->GetActiveCamera(); m_orientation == Axial)
	{
		
		cam->SetFocalPoint(0, 0, 0);
		cam->SetPosition(0, 0, -1);
		cam->SetViewUp(0, -1, 0);
	}
	else if (m_orientation == Coronal)
	{
		
		cam->SetFocalPoint(0, 0, 0);
		cam->SetPosition(0, -1, 0);
		cam->SetViewUp(0, 0, 1);
	}
	else
	{
		
		cam->SetFocalPoint(0, 0, 0);
		cam->SetPosition(1, 0, 0);
		cam->SetViewUp(0, 0, 1);
	}
	
	
	//window settings
	m_renderer->SetBackground(0,0,0);
	auto menuIcon = QIcon(QString("resources/icons/settings.svg"));
	auto menuButton = new QPushButton(menuIcon, QString(), m_openGLWidget);
	menuButton->setIconSize(QSize(24, 24));
	menuButton->setStyleSheet("QPushButton {background-color:transparent;}");
	auto menu = new QMenu(menuButton);
	menuButton->setMenu(menu);
	
	menu->addAction(QString(tr("Set background color")), [=]() {
		auto color = QColorDialog::getColor(Qt::black, this);
		if (color.isValid())
			m_renderer->SetBackground(color.redF(), color.greenF(), color.blueF());
		updateRendering();
	});
	menu->addAction(QString(tr("Save to file")), [=]() {
		auto filename = QFileDialog::getSaveFileName(this, tr("Save File"), "untitled.png",	tr("Images (*.png)"));
		if (!filename.isEmpty())
		{
			auto renderWindow = m_openGLWidget->GetRenderWindow();
			vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter =
				vtkSmartPointer<vtkWindowToImageFilter>::New();
			windowToImageFilter->SetInput(renderWindow);
			windowToImageFilter->SetScale(3, 3); //set the resolution of the output image (3 times the current resolution of vtk render window)
			windowToImageFilter->SetInputBufferTypeToRGBA(); //also record the alpha (transparency) channel
			windowToImageFilter->ReadFrontBufferOff(); // read from the back buffer
			windowToImageFilter->Update();
			vtkSmartPointer<vtkPNGWriter> writer =
				vtkSmartPointer<vtkPNGWriter>::New();
			writer->SetFileName(filename.toLatin1().data());
			writer->SetInputConnection(windowToImageFilter->GetOutputPort());
			writer->Write();
			this->updateRendering();
		}
	});
}

void SliceRenderWidget::updateRendering()
{
	//might need to call Render
	//m_resliceImageViewer->Reset();
	
	auto renderWindow = m_openGLWidget->GetRenderWindow();
	auto renderCollection = renderWindow->GetRenderers();
	auto renderer = renderCollection->GetFirstRenderer();
	renderer->ResetCamera();
	renderWindow->Render();
	m_openGLWidget->update();
	return;
}

void SliceRenderWidget::setImageData(std::shared_ptr<ImageContainer> volume)
{
	if (!volume)
		return;
	if (m_image1)
	{
		if ((m_image1->ID == volume->ID) && (m_image1->imageType == volume->imageType))
		{
			return;
		}
	}
	
	m_image1 = volume;
	m_imageMapper->SetInputData(m_image1->image);
	
	//update LUT based on image type
	if (auto prop = m_imageSlice->GetProperty(); m_image1->imageType == ImageContainer::CTImage)
	{
		prop->BackingOff();
		
		
		//auto lut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();
		auto lut = vtkSmartPointer<vtkLookupTable>::New();
		lut->SetHueRange(0.0, 0.0);
		lut->SetSaturationRange(0.0, 0.0);
		lut->SetValueRange(0.0, 1.0);
		lut->SetAboveRangeColor(1.0, 1.0, 1.0, 1.0);
		lut->UseAboveRangeColorOn();
		lut->SetBelowRangeColor(0.0, 0.0, 0.0, 0.0);
		lut->UseBelowRangeColorOn();	
		lut->Build();
		prop->SetLookupTable(lut);
	}
	else if (m_image1->imageType == ImageContainer::DensityImage)
	{

	}
	else if (m_image1->imageType == ImageContainer::MaterialImage)
	{

	}
	else if (m_image1->imageType == ImageContainer::OrganImage)
	{

	}
	else if (m_image1->imageType == ImageContainer::DoseImage)
	{

	}

	m_renderer->ResetCamera();
	updateRendering();
	
}
