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
#include <QWidgetAction>
#include <QSlider>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkWindowLevelLookupTable.h>
#include <vtkDiscretizableColorTransferFunction.h>
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
		if (m_imageMapperBackground)
		{
			m_imageMapperBackground->SetSlicePlane(plane);
			m_imageMapperBackground->UpdateInformation();
		}
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
		if (m_imageMapperBackground)
		{
			m_imageMapperBackground->SetSlicePlane(plane);
			m_imageMapperBackground->UpdateInformation();
		}
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
	void setMapperBackground(vtkSmartPointer<vtkImageResliceMapper> m)
	{
		m_imageMapperBackground = m;
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
			m_text = prettyNumber(l) + ", " + prettyNumber(w);
			m_textActor->SetInput(m_text.c_str());
		}
	}
private:
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapper;
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapperBackground;
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
	m_imageSmoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
	m_imageSmoother->SetDimensionality(3);
	m_imageSmoother->SetStandardDeviations(0.0, 0.0, 0.0);

	m_imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
	m_imageMapper->StreamingOn();
	m_imageMapper->SetInputConnection(m_imageSmoother->GetOutputPort());

	m_imageMapperBackground = vtkSmartPointer<vtkImageResliceMapper>::New();
	m_imageMapperBackground->StreamingOn();

	m_imageSlice = vtkSmartPointer<vtkImageSlice>::New();
	m_imageSlice->SetMapper(m_imageMapper);

	m_imageSliceBackground = vtkSmartPointer<vtkImageSlice>::New();
	m_imageSliceBackground->SetMapper(m_imageMapperBackground);


	//renderer
	// Setup renderers
	m_renderer = vtkSmartPointer<vtkRenderer>::New();
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
	style->setMapperBackground(m_imageMapperBackground);
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
	m_imageSmoother->SetInputData(dummyData);
	m_imageMapperBackground->SetInputData(dummyData);

	//other
	m_imageMapper->SliceFacesCameraOn();
	m_imageMapperBackground->SliceFacesCameraOn();
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
	
	auto smoothSlider = new QSlider(Qt::Horizontal, menuButton);
	smoothSlider->setMaximum(10);
	smoothSlider->setTickInterval(1);
	smoothSlider->setTracking(true);
	if (m_orientation == Axial)
		connect(smoothSlider, &QSlider::valueChanged, [=](int value) {m_imageSmoother->SetStandardDeviations(static_cast<double>(value), static_cast<double>(value), 0.0); });
	else if (m_orientation == Coronal)
		connect(smoothSlider, &QSlider::valueChanged, [=](int value) {m_imageSmoother->SetStandardDeviations(0.0, static_cast<double>(value), static_cast<double>(value)); });
	else
		connect(smoothSlider, &QSlider::valueChanged, [=](int value) {m_imageSmoother->SetStandardDeviations(static_cast<double>(value), 0.0, static_cast<double>(value)); });
	auto smoothSliderAction = new QWidgetAction(menuButton);
	smoothSliderAction->setDefaultWidget(smoothSlider);
	menu->addAction(smoothSliderAction);
	
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

void SliceRenderWidget::setImageData(std::shared_ptr<ImageContainer> volume, std::shared_ptr<ImageContainer> background)
{
	if (!volume)
		return;
	if (!volume->image)
		return;
	if (m_image)
	{
		if ((m_image->ID == volume->ID) && (m_image->imageType == volume->imageType) && (m_imageBackground==background))
			return;
		
		if (std::array<double, 2> wl; m_image->image)
		{
			auto props = m_imageSlice->GetProperty();
			wl[0] = props->GetColorLevel();
			wl[1] = props->GetColorWindow();
			m_windowLevels[m_image->imageType] = wl;
		}
	}

	m_image = volume;
	m_imageBackground = background;
	m_renderer->RemoveActor(m_imageSliceBackground);
	m_renderer->RemoveActor(m_imageSlice);

	if (m_windowLevels.find(m_image->imageType) == m_windowLevels.end())
	{
		std::array<double, 2> wl = presetLeveling(m_image->imageType);
		if (wl[1] < 0.0) 
		{
			const auto& mm = m_image->minMax;
			wl[0] = (mm[0] + mm[1]) * 0.5;
			wl[1] = (mm[1] - mm[0]) * 0.5;
		}
		m_windowLevels[m_image->imageType] = wl;
	}
	m_imageSmoother->SetInputData(m_image->image);
	m_imageSmoother->Update();
	//m_imageMapper->SetInputData(m_image->image);
	
	//update LUT based on image type
	if (auto prop = m_imageSlice->GetProperty(); m_image->imageType == ImageContainer::CTImage)
	{
		prop->BackingOff();
		prop->UseLookupTableScalarRangeOff();
		prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
		prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);
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
	else if (m_image->imageType == ImageContainer::DensityImage)
	{
		prop->BackingOff();
		prop->UseLookupTableScalarRangeOff();
		prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
		prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);
		auto lut = vtkSmartPointer<vtkLookupTable>::New();
		lut->SetHueRange(0.0, 1.0);
		lut->SetSaturationRange(0.5, 0.5);
		lut->SetValueRange(1.0, 1.0);
		lut->SetBelowRangeColor(0.0, 0.0, 0.0, 0.0);
		lut->UseBelowRangeColorOn();
		lut->Build();
		prop->SetLookupTable(lut);
	}
	else if (m_image->imageType == ImageContainer::MaterialImage)
	{
		prop->BackingOff();
		prop->UseLookupTableScalarRangeOn();
		auto lut = vtkSmartPointer<vtkLookupTable>::New();
		
		int nColors = static_cast<int>(m_image->minMax[1]) + 1;
		lut->SetNumberOfTableValues(nColors);
		for (int i = 0; i < nColors; ++i)
		{
			auto arr = getColor(i);
			if (i == 0)
				lut->SetTableValue(i, arr[0], arr[1], arr[2], 0.0);
			else
				lut->SetTableValue(i, arr[0], arr[1], arr[2], 1.0);
		}
		lut->SetTableRange(m_image->minMax.data());
		prop->SetLookupTable(lut);
	}
	else if (m_image->imageType == ImageContainer::OrganImage)
	{
		prop->BackingOff();
		prop->UseLookupTableScalarRangeOn();
		auto lut = vtkSmartPointer<vtkLookupTable>::New();

		int nColors = static_cast<int>(m_image->minMax[1]) + 1;
		lut->SetNumberOfTableValues(nColors);
		for (int i = 0; i < nColors; ++i)
		{
			auto arr = getColor(i);
			if (i == 0)
				lut->SetTableValue(i, arr[0], arr[1], arr[2], 0.0);
			else
				lut->SetTableValue(i, arr[0], arr[1], arr[2], 1.0);
		}
		lut->SetTableRange(m_image->minMax.data());
		prop->SetLookupTable(lut);
	}
	else if (m_image->imageType == ImageContainer::DoseImage)
	{
		prop->BackingOff();
		prop->UseLookupTableScalarRangeOff();
		prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
		prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);
		auto lut = vtkSmartPointer<vtkLookupTable>::New();
		lut->SetHueRange(0.0, 1.0);
		lut->SetSaturationRange(0.5, 0.5);
		lut->SetValueRange(1.0, 1.0);
		lut->SetBelowRangeColor(0.0, 0.0, 0.0, 0.0);
		lut->UseBelowRangeColorOn();
		lut->Build();
		prop->SetLookupTable(lut);
	}

	if (m_imageBackground)
	{
		if (m_imageBackground->image)
		{
			m_imageMapperBackground->SetInputData(m_imageBackground->image);
			
			m_renderer->AddActor(m_imageSliceBackground);

			auto prop = m_imageSliceBackground->GetProperty();
			prop->BackingOff();
			prop->UseLookupTableScalarRangeOff();
			auto wl = presetLeveling(m_imageBackground->imageType);
			prop->SetColorLevel(wl[0]);
			prop->SetColorWindow(wl[1]);
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
	}
	m_renderer->AddActor(m_imageSlice);
	m_renderer->ResetCamera();
	updateRendering();
}

std::array<double, 2> SliceRenderWidget::presetLeveling(ImageContainer::ImageType type)
{
	std::array<double, 2> wl = {1.0, -1.0};
	if (type == ImageContainer::CTImage)
	{
		wl[0] = 10.0;
		wl[1] = 500.0;
	}
	else if (type == ImageContainer::DensityImage)
	{
		wl[0] = 1.0;
		wl[1] = 0.5;
	}
	else if (type == ImageContainer::DoseImage)
	{
		wl[0] = 0.1;
		wl[1] = 0.1;
	}
	return wl;
}

/*void SliceRenderWidget::updateOrientation(void)
{
	auto argmax = vectormath::argmax3<std::size_t, double>(m_image->directionCosines.data());

	if (m_orientation == Axial)
	{
		for (std::size_t i = 0; i < 3; ++i)
			dir[i] = m_image->directionCosines[i];
	}
	else if (m_orientation == Sagittal)
	{
		for (std::size_t i = 0; i < 3; ++i)
			dir[i] = m_image->directionCosines[i + 3];
	}
	else
	{
		vectormath::cross(m_image->directionCosines.data(), dir.data());
	}

	
	std::size_t argmax = vectormath::argmax3()
	const auto& dir = m_image->directionCosines;


}
*/