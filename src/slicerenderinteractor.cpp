
#include "opendxmc/slicerenderinteractor.h"
#include "dxmc/vectormath.h"



#include <vtkCamera.h>
#include <vtkPlane.h>
#include <vtkRendererCollection.h>
#include <vtkImageProperty.h>
#include <vtkImageData.h>
#include <vtkObjectFactory.h>

#include <array>

vtkStandardNewMacro(customMouseInteractorStyle);

void customMouseInteractorStyle::OnMouseWheelForward()
{
	scrollSlice(true);
}

void customMouseInteractorStyle::OnMouseWheelBackward()
{
	scrollSlice(false);
}

void customMouseInteractorStyle::OnMouseMove()
{
	vtkInteractorStyleImage::OnMouseMove();
	updateWLText();
}

void customMouseInteractorStyle::OnLeftButtonDown()
{
	int x = this->Interactor->GetEventPosition()[0];
	int y = this->Interactor->GetEventPosition()[1];

	this->FindPokedRenderer(x, y);
	this->FindPickedActor(x, y);

	vtkInteractorStyleImage::OnLeftButtonDown();
}

void customMouseInteractorStyle::OnLeftButtonUp()
{
	vtkInteractorStyleImage::OnLeftButtonUp();
}


void customMouseInteractorStyle::setMapper(vtkSmartPointer<vtkImageResliceMapper> m)
{
	m_imageMapper = m;
}
void customMouseInteractorStyle::setMapperBackground(vtkSmartPointer<vtkImageResliceMapper> m)
{
	m_imageMapperBackground = m;
}
void customMouseInteractorStyle::setRenderWindow(vtkSmartPointer<vtkRenderWindow> m)
{
	m_renderWindow = m;
}

void customMouseInteractorStyle::setCornerAnnotation(vtkSmartPointer<vtkCornerAnnotation> actor)
{
	m_textActorCorners = actor;
}

std::string customMouseInteractorStyle::prettyNumber(double number)
{
	constexpr std::int32_t k = 2; // number decimals
	std::int32_t b = number * std::pow(10, k);
	auto bs = std::to_string(b);
	int c = static_cast<int>(bs.size() - k);
	if (c < 0)
		bs.insert(0, -c, '0');
	if (bs.size() == k)
		bs.insert(0, "0.");
	else
		bs.insert(bs.size() - k, 1, '.');
	return bs;
}

void customMouseInteractorStyle::update()
{
	updateWLText();
	updatePlaneActors();
	m_renderWindow->Render();
}

void customMouseInteractorStyle::updateWLText()
{
	auto prop = GetCurrentImageProperty();
	if (prop)
	{
		double l = prop->GetColorLevel();
		double w = prop->GetColorWindow();
		auto text = "WC: " + prettyNumber(l) + "\n" + "WW: " + prettyNumber(w);
		if (m_textActorCorners)
			m_textActorCorners->SetText(0, text.c_str());
	}
}
void customMouseInteractorStyle::addImagePlaneActor(VolumeActorContainer* container)
{
	m_imagePlaneActors.push_back(container);
	updatePlaneActors();
	m_renderWindow->Render();
}
void customMouseInteractorStyle::removeImagePlaneActor(VolumeActorContainer* container)
{
	m_imagePlaneActors.erase(std::remove(m_imagePlaneActors.begin(), m_imagePlaneActors.end(), container), m_imagePlaneActors.end());
	auto renderCollection = m_renderWindow->GetRenderers();
	auto renderer = renderCollection->GetFirstRenderer();
	auto actor = container->getActor();
	renderer->RemoveActor(actor);
	m_renderWindow->Render();
}

void customMouseInteractorStyle::scrollSlice(bool forward)
{
	m_imageMapper->UpdateInformation();
	auto plane = m_imageMapper->GetSlicePlane();
	auto image = m_imageMapper->GetInput();
	const double direction = forward ? 1.0 : -1.0;
	if (image)
	{
		auto planeNormal = plane->GetNormal();
		auto ind = vectormath::argmax3<std::size_t, double>(planeNormal);
		auto spacing = image->GetSpacing();
		plane->Push(direction * spacing[ind]);
	}
	else
	{
		plane->Push(direction);
	}
	double* imbounds = m_imageMapper->GetBounds();
	double* origin = plane->GetOrigin();
	for (int i = 0; i < 3; ++i)
	{
		if (origin[i] > imbounds[i * 2 + 1])
			origin[i] = imbounds[i * 2];
		if (origin[i] < imbounds[i * 2])
			origin[i] = imbounds[i * 2 + 1];
	}

	auto renderCollection = m_renderWindow->GetRenderers();
	auto nRenderers = renderCollection->GetNumberOfItems();
	auto renderer = renderCollection->GetFirstRenderer();
	auto cam = renderer->GetActiveCamera();
	cam->SetFocalPoint(origin);
	updatePlaneActors();

	m_renderWindow->Render();
}
void customMouseInteractorStyle::updatePlaneActors()
{	
	auto renderCollection = m_renderWindow->GetRenderers();
	auto renderer = renderCollection->GetFirstRenderer();
	auto cam = renderer->GetActiveCamera();

	std::array<double, 3> origin;
	cam->GetFocalPoint(origin.data());

	std::array<double, 6> bounds;

	for (auto container : m_imagePlaneActors)
	{
		auto actor = container->getActor();
		renderer->RemoveActor(actor);
		actor->GetBounds(bounds.data());
		bool visible = true;
		for (std::size_t i = 0; i < 3; ++i)
		{
			visible = visible && origin[i] >= bounds[2 * i];
			visible = visible && origin[i] <= bounds[2 * i + 1];
		}
		if (visible)
		{
			renderer->AddActor(actor);
		}
	}
}

vtkActor* customMouseInteractorStyle::findPickedPlaneActor(int x, int y)
{
	this->InteractionPicker->Pick(x, y, 0.0, this->CurrentRenderer);
	vtkProp* prop = this->InteractionPicker->GetViewProp();
	if (prop != nullptr)
	{
		this->InteractionProp = vtkProp3D::SafeDownCast(prop);
	}
	else
	{
		this->InteractionProp = nullptr;
	}
}