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
#include <vtkVariant.h>
#include <vtkRendererCollection.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCamera.h>
#include <vtkObjectFactory.h>
#include <vtkTextProperty.h>
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
		int cim = m_imageSliceMapper->GetSliceNumber() + 1;
		int max = m_imageSliceMapper->GetSliceNumberMaxValue();
		int min = m_imageSliceMapper->GetSliceNumberMinValue();
		if (cim > max)
			cim = min;
		else if (cim < min)
			cim = max;
		m_imageSliceMapper->SetSliceNumber(cim);
		m_renderWindow->Render();
		
	}
	virtual void OnMouseWheelBackward()
	{
		int cim = m_imageSliceMapper->GetSliceNumber() - 1;
		int max = m_imageSliceMapper->GetSliceNumberMaxValue();
		int min = m_imageSliceMapper->GetSliceNumberMinValue();
		if (cim > max)
			cim = min;
		else if (cim < min)
			cim = max;
		m_imageSliceMapper->SetSliceNumber(cim);
		
		m_renderWindow->Render();
	}

	void OnMouseMove() override
	{
		vtkInteractorStyleImage::OnMouseMove();
		updateWLText();
	}

	void setMapper(vtkSmartPointer<vtkImageSliceMapper> m)
	{
		m_imageSliceMapper = m;
	}
	void setRenderWindow(vtkSmartPointer<vtkRenderWindow> m)
	{
		m_renderWindow = m;
	}
	void setTextActor(vtkSmartPointer<vtkTextActor> textActor)
	{
		m_textActor = textActor;
	}
	void updateWLText()
	{
		auto prop = GetCurrentImageProperty();
		if (prop)
		{
			double l = prop->GetColorLevel();
			double w = prop->GetColorWindow();

			m_text = std::to_string(l) + ", " + std::to_string(w);
			m_textActor->SetInput(m_text.c_str());
		}
	}
private:
	vtkSmartPointer<vtkImageSliceMapper> m_imageSliceMapper;
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
	m_imageSliceMapper = vtkSmartPointer<vtkImageSliceMapper>::New();
	m_imageSliceMapper->StreamingOn();
	//m_imageSliceMapper->SetSliceAtFocalPoint(true);
	//imageSliceMapper->SetInputData(colorImage);

	vtkSmartPointer<vtkImageSlice> imageSlice = vtkSmartPointer<vtkImageSlice>::New();
	imageSlice->SetMapper(m_imageSliceMapper);

	m_lut = vtkSmartPointer<vtkLookupTable>::New();
	m_dctf = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
	m_imageProperty = imageSlice->GetProperty();
	//m_imageProperty->SetLookupTable(m_lut);
	m_imageProperty->SetLookupTable(m_dctf);
	
	
	m_windowLevelMaps[static_cast<int>(ImageContainer::CTImage)] = std::make_pair(100, 500);
	m_windowLevelMaps[static_cast<int>(ImageContainer::DensityImage)] = std::make_pair(1, 1);
	m_windowLevelMaps[static_cast<int>(ImageContainer::DoseImage)] = std::make_pair(1, 1);
	

	//renderer
	// Setup renderers
	m_renderer = vtkSmartPointer<vtkRenderer>::New();
	m_renderer->AddViewProp(imageSlice);
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
	style->setMapper(m_imageSliceMapper);
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
	m_imageSliceMapper->SetInputData(dummyData);

	//other
	
	if (auto cam = m_renderer->GetActiveCamera(); m_orientation == Axial)
	{
		m_imageSliceMapper->SetOrientation(2);
		cam->SetFocalPoint(0, 0, 0);
		cam->SetPosition(0, 0, -1);
		cam->SetViewUp(0, -1, 0);
	}
	else if (m_orientation == Coronal)
	{
		m_imageSliceMapper->SetOrientation(1);
		cam->SetFocalPoint(0, 0, 0);
		cam->SetPosition(0, -1, 0);
		cam->SetViewUp(0, 0, 1);
	}
	else
	{
		m_imageSliceMapper->SetOrientation(0);
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
	auto oldImageID = 0;
	if (m_image)
	{
		if (volume->image == m_image->image)
			return;

		if (m_image->image)
		{
			if (int key = static_cast<int>(m_image->imageType); m_windowLevelMaps.contains(key))
			{
				m_windowLevelMaps[key].first = m_imageProperty->GetColorLevel();
				m_windowLevelMaps[key].second = m_imageProperty->GetColorWindow();
			}
		}
		oldImageID = m_image->ID;
	}
	
	m_image = volume;
	m_imageSliceMapper->SetInputData(m_image->image);
	//m_imageSliceMapper->Update();
	
	//lut stuff colortransfer function
	if ((m_image->imageType == ImageContainer::MaterialImage) ||
		(m_image->imageType == ImageContainer::OrganImage))
	{
		int nColors = static_cast<int>(m_image->minMax[1]) + 1;
		
		m_dctf->DiscretizeOn();

		m_dctf->SetNumberOfValues(nColors);
		m_dctf->RemoveAllPoints();
		m_lut->SetNumberOfTableValues(nColors);
		for (int i = 0; i < nColors; ++i)
		{
			auto arr = getColor(i);
			m_lut->SetTableValue(i, arr[0], arr[1], arr[2]);
			m_lut->SetAnnotation(vtkVariant(static_cast<unsigned char>(i)), "");
			m_dctf->AddRGBPoint(static_cast<double>(i), arr[0], arr[1], arr[2]);
		}
		m_dctf->Build();
		m_lut->IndexedLookupOn();
		m_imageProperty->SetLookupTable(m_dctf);
		m_imageProperty->UseLookupTableScalarRangeOn();	

	}
	else if (m_image->imageType==ImageContainer::DoseImage)
	{
		m_dctf->DiscretizeOn();
		m_dctf->RemoveAllPoints();
		m_dctf->AddRGBPoint(0, 0, 0, 0);
		m_dctf->AddRGBPoint(255, 1, 1, 1);
		m_dctf->SetNumberOfValues(256);
		m_dctf->Build();

		//using pet LUT
		m_lut->IndexedLookupOff();
		m_lut->SetSaturationRange(0, 0);
		m_lut->SetValueRange(0, 1);
		auto lut = generateStandardColorTable(JET);
		m_lut->SetNumberOfTableValues(lut.size() / 3);
		m_lut->ForceBuild();
		for (int i = 0; i < lut.size(); i += 3)
		{
			m_lut->SetTableValue(i / 3, lut[i], lut[i + 1], lut[i + 2]);
		}

		m_imageProperty->SetLookupTable(m_lut);
		m_imageProperty->UseLookupTableScalarRangeOff();

		if (int key = static_cast<int>(m_image->imageType); !m_windowLevelMaps.contains(key))
		{
			auto window = m_image->minMax[1] - m_image->minMax[0];
			auto level = 0.5 * (m_image->minMax[1] + m_image->minMax[0]);
			m_windowLevelMaps[key].first = level * 0.01;
			m_windowLevelMaps[key].second = window;
		}
		m_imageProperty->SetColorLevel(m_windowLevelMaps[static_cast<int>(m_image->imageType)].first);
		m_imageProperty->SetColorWindow(m_windowLevelMaps[static_cast<int>(m_image->imageType)].second);
	}
	else
	{
		m_dctf->DiscretizeOn();
		m_dctf->RemoveAllPoints();
		m_dctf->AddRGBPoint(0, 0, 0, 0);
		m_dctf->AddRGBPoint(255, 1, 1, 1);
		m_dctf->SetNumberOfValues(256);
		m_dctf->Build();

		m_lut->IndexedLookupOff();
		m_lut->SetSaturationRange(0, 0);
		m_lut->SetValueRange(0, 1);
		m_lut->SetNumberOfTableValues(256);
		m_lut->ForceBuild();
		m_imageProperty->SetLookupTable(m_lut);
		m_imageProperty->UseLookupTableScalarRangeOff();

		if (int key = static_cast<int>(m_image->imageType); !m_windowLevelMaps.contains(key))
		{
			auto window = m_image->minMax[1] - m_image->minMax[0];
			auto level = 0.5 * (m_image->minMax[1] + m_image->minMax[0]);
			m_windowLevelMaps[key].first = level;
			m_windowLevelMaps[key].second = window;
		}
		m_imageProperty->SetColorLevel(m_windowLevelMaps[static_cast<int>(m_image->imageType)].first);
		m_imageProperty->SetColorWindow(m_windowLevelMaps[static_cast<int>(m_image->imageType)].second);
	}
	m_imageSliceMapper->Update();


	
	if (oldImageID != m_image->ID)
	{
		int sliceMin = m_imageSliceMapper->GetSliceNumberMinValue();
		int sliceMax = m_imageSliceMapper->GetSliceNumberMaxValue();
		m_imageSliceMapper->SetSliceNumber((sliceMax - sliceMin) / 2);
	}
	
	//orienting cameras
	/*{
		int maxArg;
		if (m_orientation == Axial)
		{
			double z[3];
			vectormath::cross(m_image.directionCosines.data(), z);
			for (int i = 0; i < 3; ++i) z[i] = z[i] * z[i];
			maxArg = vectormath::argmax3<int>(z);
		}
		else if (m_orientation == Sagittal)
		{
			double z[3];
			for (int i = 0; i < 3; ++i) z[i] = m_image.directionCosines[i] * m_image.directionCosines[i];
			maxArg = vectormath::argmax3<int>(z);
		}
		else
		{
			double z[3];
			for (int i = 0; i < 3; ++i) z[i] = m_image.directionCosines[i + 3] * m_image.directionCosines[i + 3];
			maxArg = vectormath::argmax3<int>(z);
		}

		m_imageSliceMapper->SetOrientation(maxArg);
		auto cam = m_renderer->GetActiveCamera();
		if (maxArg == 2)
		{
			cam->SetFocalPoint(0, 0, 0);
			cam->SetPosition(0, 0, 1);
			cam->SetViewUp(0, -1, 0);
		}
		else if (maxArg == 1)
		{
			cam->SetFocalPoint(0, 0, 0);
			cam->SetPosition(0, 1, 0);
			cam->SetViewUp(0, 0, 1);
		}
		else if (maxArg == 0)
		{
			//m_imageSliceMapper->SetOrientation(0);
			cam->SetFocalPoint(0, 0, 0);
			cam->SetPosition(1, 0, 0);
			cam->SetViewUp(0, 0, 1);
		}
	}*/
	

	m_renderer->ResetCamera();
	updateRendering();
	
}
