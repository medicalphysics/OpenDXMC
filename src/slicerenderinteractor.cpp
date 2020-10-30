
#include "opendxmc/slicerenderinteractor.h"
#include "opendxmc/dxmc_specialization.h"

#include <vtkCamera.h>
#include <vtkImageData.h>
#include <vtkImageProperty.h>
#include <vtkObjectFactory.h>
#include <vtkPlane.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <vtkTransform.h>

#include <array>

vtkStandardNewMacro(customMouseInteractorStyle);

customMouseInteractorStyle::customMouseInteractorStyle()
{
    m_interactionPicker = vtkSmartPointer<vtkCellPicker>::New();
}

void customMouseInteractorStyle::OnMouseWheelForward()
{
    scrollSlice(false);
}

void customMouseInteractorStyle::OnMouseWheelBackward()
{
    scrollSlice(true);
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

    m_pickedPlaneActor = findPickedPlaneActor(x, y);

    if (!m_pickedPlaneActor) {
        vtkInteractorStyleImage::OnLeftButtonDown();
    } else {
        //Actor pan
        this->StartPan();
    }
}

void customMouseInteractorStyle::OnLeftButtonUp()
{
    if (!m_pickedPlaneActor) {
        vtkInteractorStyleImage::OnLeftButtonUp();
    } else {

        m_pickedPlaneActor->applyActorTranslationToSource();
        m_callback();
        //Actor pan
        this->EndPan();
    }
}

void customMouseInteractorStyle::StartPan()
{
    vtkInteractorStyleImage::StartPan();
}
void customMouseInteractorStyle::EndPan()
{
    vtkInteractorStyleImage::EndPan();
}
void customMouseInteractorStyle::Pan()
{
    if (m_pickedPlaneActor == nullptr || this->CurrentRenderer == nullptr) {
        vtkInteractorStyleImage::Pan();
        return;
    }

    auto actor = m_pickedPlaneActor->getActor();
    auto pos = actor->GetCenter();
    std::array<double, 3> displayPos;
    this->ComputeWorldToDisplay(pos[0], pos[1], pos[2], displayPos.data());

    auto rwi = this->GetInteractor();
    std::array<double, 4> newPos, oldPos;
    this->ComputeDisplayToWorld(
        rwi->GetEventPosition()[0], rwi->GetEventPosition()[1], displayPos[2], newPos.data());
    this->ComputeDisplayToWorld(
        rwi->GetLastEventPosition()[0], rwi->GetLastEventPosition()[1], displayPos[2], oldPos.data());

    std::array<double, 3> motionVector;
    for (std::size_t i = 0; i < 3; ++i)
        motionVector[i] = newPos[i] - oldPos[i];

    auto matrix = m_pickedPlaneActor->getMatrix();
    for (int i = 0; i < 3; ++i) {
        const double new_pos = motionVector[i] + matrix->GetElement(i, 3);
        matrix->SetElement(i, 3, new_pos);
    }

    if (this->AutoAdjustCameraClippingRange) {
        this->CurrentRenderer->ResetCameraClippingRange();
    }
    rwi->Render();
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
    if (prop) {
        double l = prop->GetColorLevel();
        double w = prop->GetColorWindow();
        auto text = "WC: " + prettyNumber(l) + "\n" + "WW: " + prettyNumber(w);
        if (m_textActorCorners)
            m_textActorCorners->SetText(0, text.c_str());
    }
}
void customMouseInteractorStyle::addImagePlaneActor(SourceActorContainer* container)
{
    m_imagePlaneActors.push_back(container);
    updatePlaneActors();
    m_renderWindow->Render();
}
void customMouseInteractorStyle::removeImagePlaneActor(SourceActorContainer* container)
{
    m_imagePlaneActors.erase(std::remove(m_imagePlaneActors.begin(), m_imagePlaneActors.end(), container), m_imagePlaneActors.end());
    auto renderCollection = m_renderWindow->GetRenderers();
    auto renderer = renderCollection->GetFirstRenderer();
    auto actor = container->getActor();
    renderer->RemoveActor(actor);
    m_renderWindow->Render();
}

void customMouseInteractorStyle::scrollToStart()
{
    m_imageMapper->UpdateInformation();
    auto plane = m_imageMapper->GetSlicePlane();
    auto planeNormal = plane->GetNormal();
    auto ind = dxmc::vectormath::argmax3<std::size_t, double>(planeNormal);

    double* imbounds = m_imageMapper->GetBounds();
    double* origin = plane->GetOrigin();
    origin[ind] = imbounds[ind * 2];

    scrollToPoint(origin);
}
void customMouseInteractorStyle::scrollToPoint(double point[3])
{
    auto renderCollection = m_renderWindow->GetRenderers();
    auto nRenderers = renderCollection->GetNumberOfItems();
    auto renderer = renderCollection->GetFirstRenderer();
    auto cam = renderer->GetActiveCamera();
    cam->SetFocalPoint(point);
    updatePlaneActors();
    m_renderWindow->Render();
}
void customMouseInteractorStyle::scrollSlice(bool forward)
{
    m_imageMapper->UpdateInformation();
    auto plane = m_imageMapper->GetSlicePlane();
    auto image = m_imageMapper->GetInput();
    const double direction = forward ? 1.0 : -1.0;
    if (image) {
        auto planeNormal = plane->GetNormal();
        auto ind = dxmc::vectormath::argmax3<std::size_t, double>(planeNormal);
        auto spacing = image->GetSpacing();
        plane->Push(direction * spacing[ind]);
    } else {
        plane->Push(direction);
    }
    double* imbounds = m_imageMapper->GetBounds();
    double* origin = plane->GetOrigin();
    for (int i = 0; i < 3; ++i) {
        if (origin[i] > imbounds[i * 2 + 1])
            origin[i] = imbounds[i * 2];
        if (origin[i] < imbounds[i * 2])
            origin[i] = imbounds[i * 2 + 1];
    }
    scrollToPoint(origin);
    /*
    auto renderCollection = m_renderWindow->GetRenderers();
    auto nRenderers = renderCollection->GetNumberOfItems();
    auto renderer = renderCollection->GetFirstRenderer();
    auto cam = renderer->GetActiveCamera();
    cam->SetFocalPoint(origin);
    updatePlaneActors();

    m_renderWindow->Render();*/
}

void customMouseInteractorStyle::updatePlaneActors()
{
    auto renderCollection = m_renderWindow->GetRenderers();
    auto renderer = renderCollection->GetFirstRenderer();
    auto cam = renderer->GetActiveCamera();

    auto plane = m_imageMapper->GetSlicePlane();
    std::array<double, 3> plane_normal;
    plane->GetNormal(plane_normal.data());
    std::array<double, 3> plane_origin;
    cam->GetFocalPoint(plane_origin.data());
    const auto nIdx = dxmc::vectormath::argmax3<std::size_t, double>(plane_normal.data());
    for (auto container : m_imagePlaneActors) {
        auto actor = container->getActor();
        renderer->RemoveActor(actor);
        std::array<double, 6> bounds;
        actor->GetBounds(bounds.data());
        const bool visible = plane_origin[nIdx] >= bounds[2 * nIdx] && plane_origin[nIdx] <= bounds[2 * nIdx + 1];

        if (visible && m_imagePlaneActorVisibility) {
            renderer->AddActor(actor);
        }
    }
}

SourceActorContainer* customMouseInteractorStyle::findPickedPlaneActor(int x, int y)
{
    this->FindPokedRenderer(x, y);
    auto renWinInteractor = GetInteractor();
    auto picker = renWinInteractor->GetPicker();
    m_interactionPicker->Pick(x, y, 0, this->CurrentRenderer);
    if (auto prop = m_interactionPicker->GetViewProp()) {
        for (auto cont : m_imagePlaneActors) {
            auto contActor = cont->getActor();
            if (contActor == prop)
                return cont;
        }
    }
    return nullptr;
}