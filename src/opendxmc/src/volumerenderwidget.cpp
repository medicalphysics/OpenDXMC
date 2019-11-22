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

#include "volumerenderwidget.h"
#include "vectormath.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMenu>
#include <QIcon>
#include <QColorDialog>
#include <QFileDialog>
#include <QWidgetAction>

#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkDataArray.h>
#include <vtkLinearTransform.h>
#include <vtkWindowToImageFilter.h>
#include <vtkPNGWriter.h>


#include  <cmath>



VolumeRenderWidget::VolumeRenderWidget(QWidget *parent)
	:QWidget(parent), m_renderMode(0)
{
	
	m_openGLWidget = new QVTKOpenGLNativeWidget(this);
	vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
	m_openGLWidget->setRenderWindow(renderWindow);
	m_renderer = vtkSmartPointer<vtkOpenGLRenderer>::New();
	m_renderer->BackingStoreOn();
	renderWindow->AddRenderer(m_renderer);
	renderWindow->Render(); // making opengl context

	vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
	vtkSmartPointer<vtkColorTransferFunction> colorFun = vtkSmartPointer<vtkColorTransferFunction>::New();
	vtkSmartPointer<vtkPiecewiseFunction> opacityFun = vtkSmartPointer<vtkPiecewiseFunction>::New();
	vtkSmartPointer<vtkPiecewiseFunction> gradientFun = vtkSmartPointer<vtkPiecewiseFunction>::New();
	volumeProperty->SetColor(colorFun);
	volumeProperty->SetScalarOpacity(opacityFun);
	volumeProperty->SetGradientOpacity(gradientFun);
	volumeProperty->ShadeOn();
	volumeProperty->SetInterpolationTypeToLinear();
	volumeProperty->SetAmbient(0.6);
	volumeProperty->SetDiffuse(0.9);
	volumeProperty->SetSpecular(0.5);
	volumeProperty->SetSpecularPower(10.0);

	//smoother
	m_imageSmoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
	m_imageSmoother->SetDimensionality(3);
	m_imageSmoother->SetStandardDeviations(0.0, 0.0, 0.0);

	m_settingsWidget = new VolumeRenderSettingsWidget(volumeProperty, this);
	connect(this, &VolumeRenderWidget::imageDataChanged, m_settingsWidget, &VolumeRenderSettingsWidget::setImage);
	connect(m_settingsWidget, &VolumeRenderSettingsWidget::propertyChanged, this, &VolumeRenderWidget::updateRendering);
	connect(m_settingsWidget, &VolumeRenderSettingsWidget::renderModeChanged, this, &VolumeRenderWidget::setRenderMode);
	connect(m_settingsWidget, &VolumeRenderSettingsWidget::cropPlanesChanged, this, &VolumeRenderWidget::setCropPlanes);

	auto layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addWidget(m_openGLWidget);
	this->setLayout(layout);


	//adding orientation prop
	m_orientationProp = std::make_shared<OrientationActorContainer>();
	auto orientationPropPtr = std::static_pointer_cast<VolumeActorContainer>(m_orientationProp).get();
	m_volumeProps.push_back(orientationPropPtr);
	m_renderer->AddActor(m_orientationProp->getActor());	



	//window settings
	m_renderer->SetBackground(0, 0, 0);
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
	connect(smoothSlider, &QSlider::valueChanged, [=](int value) {m_imageSmoother->SetStandardDeviations(static_cast<double>(value), static_cast<double>(value), static_cast<double>(value)); });
	auto smoothSliderAction = new QWidgetAction(menuButton);
	smoothSliderAction->setDefaultWidget(smoothSlider);
	menu->addAction(smoothSliderAction);


	auto showAdvancedAction = menu->addAction(tr("Advanced"));
	connect(showAdvancedAction, &QAction::triggered, m_settingsWidget, &VolumeRenderSettingsWidget::toggleVisibility);


	auto showGrapicsAction = menu->addAction(tr("Show graphics"));
	showGrapicsAction->setCheckable(true);
	showGrapicsAction->setChecked(true);
	connect(showGrapicsAction, &QAction::toggled, this, &VolumeRenderWidget::setActorsVisible);

	menu->addAction(QString(tr("Set background color")), [=]() {
		auto color = QColorDialog::getColor(Qt::black, this);
		if (color.isValid())
			m_renderer->SetBackground(color.redF(), color.greenF(), color.blueF());
		updateRendering();
	});
	menu->addAction(QString(tr("Save image to file")), [=]() {
		auto filename = QFileDialog::getSaveFileName(this, tr("Save File"), "untitled.png", tr("Images (*.png)"));
		if (!filename.isEmpty())
		{
			auto renderWindow = m_openGLWidget->renderWindow();
			vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
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

VolumeRenderWidget::~VolumeRenderWidget()
{
	if (m_settingsWidget)
	{
		delete m_settingsWidget;
		m_settingsWidget = nullptr;
	}
}


void VolumeRenderWidget::updateRendering(void)
{
	if (m_volume)
		m_volume->Update();
	m_openGLWidget->update();
	m_openGLWidget->renderWindow()->Render();	
}
void VolumeRenderWidget::setImageData(std::shared_ptr<ImageContainer> image)
{
	if (!image)
		return;
	if (!image->image)
		return;
	int* dim = image->image->GetDimensions();
	for (int i = 0; i < 3; ++i)
		if (dim[i] < 2)
			return;
	if (m_imageData)
	{
		if (image->image == m_imageData->image)
		{
			return;
		}
	}
	m_imageData = image;
	m_imageSmoother->SetInputData(m_imageData->image);
	updateVolumeRendering();
}


void VolumeRenderWidget::updateVolumeProps()
{
	//we are simply computing new euler angles to the orientation marker based on the image
	//recalculate position for orientation marker
	double *bounds = m_imageData->image->GetBounds();
	m_orientationProp->getActor()->SetPosition(bounds[0], bounds[2], bounds[4]);
	for (auto prop:m_volumeProps)
		prop->setOrientation(m_imageData->directionCosines);
}

void VolumeRenderWidget::updateVolumeRendering()
{
	if (m_volume)
		m_renderer->RemoveVolume(m_volume);
	
	if (!m_imageData->image)
		return;
	
	m_volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
	m_volume = vtkSmartPointer<vtkVolume>::New();
	auto volumeProperty = m_settingsWidget->getVolumeProperty();
	auto spacing = m_imageData->image->GetSpacing();
	double meanSpacing = (spacing[0]+spacing[1]+spacing[2]) / 3.0;
	double minSpacing = std::min(std::min(spacing[0], spacing[1]), spacing[2]);
	volumeProperty->SetScalarOpacityUnitDistance(meanSpacing);
	m_volumeMapper->SetInteractiveUpdateRate(0.00001);
	m_volumeMapper->SetSampleDistance(minSpacing*1.2);
	m_volumeMapper->CroppingOn();
	int *extent = m_imageData->image->GetExtent();
	setCropPlanes(extent);
	
	if (m_renderMode == 0)
		m_volumeMapper->SetRequestedRenderModeToRayCast();
	else if (m_renderMode == 1)
		m_volumeMapper->SetRequestedRenderModeToGPU();
#ifdef ENABLE_OSPRAY
	else if (m_renderMode == 2)
		m_volumeMapper->SetRequestedRenderModeToOSPRay();
#endif // ENABLE_OSPRAY
	m_volumeMapper->SetBlendModeToComposite();
	m_volumeMapper->SetInputConnection(m_imageSmoother->GetOutputPort());
	m_volumeMapper->Update();
	emit imageDataChanged(m_imageData);
	
	m_volume->SetProperty(volumeProperty);
	m_volume->SetMapper(m_volumeMapper);
	m_volume->Update();

	updateVolumeProps();

	m_renderer->AddViewProp(m_volume);
	m_renderer->ResetCamera();
	
	updateRendering();
}

void VolumeRenderWidget::setRenderMode(int mode)
{
	m_renderMode = mode;
	if (m_volumeMapper)
	{
		if (mode == 0)
		{
			m_volumeMapper->SetRequestedRenderModeToRayCast();
		}
		if (mode == 1)
		{
			m_volumeMapper->SetRequestedRenderModeToGPU();
		}
#ifdef ENABLE_OSPRAY
		if (mode == 2)
		{
			m_volumeMapper->SetRequestedRenderModeToOSPRay();
		}
#endif // ENABLE_OSPRAY

		updateRendering();
	}
}

void VolumeRenderWidget::setCropPlanes(int planes[6])
{
	double planepos[6];
	auto spacing = m_imageData->image->GetSpacing();
	auto origo = m_imageData->image->GetOrigin();
	for (int i = 0; i < 6; ++i)
		planepos[i] = planes[i] * spacing[i / 2] + origo[i / 2];
	m_volumeMapper->SetCroppingRegionPlanes(planepos);
	this->updateRendering();
}

void VolumeRenderWidget::addActorContainer(VolumeActorContainer* actorContainer)
{
	auto actor = actorContainer->getActor();
	auto present = m_renderer->GetActors()->IsItemPresent(actor);
	if (present == 0)
	{
		if (m_imageData)
			actorContainer->setOrientation(m_imageData->directionCosines);
		m_renderer->AddActor(actor);
		m_volumeProps.push_back(actorContainer);
	}
	updateRendering();
}

void VolumeRenderWidget::removeActorContainer(VolumeActorContainer* actorContainer)
{
	auto actor = actorContainer->getActor();
	if (m_renderer->GetActors()->IsItemPresent(actor) != 0)
		m_renderer->RemoveActor(actor);
	auto pos = std::find(m_volumeProps.begin(), m_volumeProps.end(), actorContainer);
	if (pos != m_volumeProps.end())
		m_volumeProps.erase(pos);
	updateRendering();
}

void VolumeRenderWidget::setActorsVisible(int visible)
{
	for (auto m : m_volumeProps)
	{
		auto actor = m->getActor();
		if (visible == 0)
			actor->VisibilityOff();
		else
			actor->VisibilityOn();
	}
	updateRendering();
}
