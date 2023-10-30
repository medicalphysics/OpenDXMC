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

#include <slicerenderwidget.hpp>

#include <QTimer>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkCamera.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkInteractorStyleImage.h>
#include <vtkOpenGLImageSliceMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>

#include <vtkImageResliceMapper.h>

// #include <vtkRenderer.h>
// #include <vtkResliceCursor.h>
// #include <vtkResliceCursorActor.h>
// #include <vtkResliceCursorLineRepresentation.h>
// #include <vtkResliceCursorPolyDataAlgorithm.h>
// #include <vtkResliceCursorWidget.h>
//  #include <vtkResliceImageViewer.h>
// #include <vtkScalarBarActor.h>
// #include <vtkTextProperty.h>
//  #include <vtkCellPicker.h>
//  #include <vtkCornerAnnotation.h>
//  #include <vtkImageActor.h>
//  #include <vtkPlane.h>
//  #include <vtkPlaneSource.h>
//  #include <vtkLookupTable.h>
//  #include <vtkMatrix4x4.h>
//  #include <vtkImageMapToColors.h>
//  #include <vtkImageMapper3D.h>
//  #include <vtkImagePlaneWidget.h>

/*
class vtkResliceCursorCallback : public vtkCommand {
public:
    static vtkResliceCursorCallback* New()
    {
        return new vtkResliceCursorCallback;
    }

    void Execute(vtkObject* caller, unsigned long ev, void* callData)
    {
        if (ev == vtkResliceCursorWidget::WindowLevelEvent || ev == vtkCommand::WindowLevelEvent || ev == vtkResliceCursorWidget::ResliceThicknessChangedEvent) {
            // Render everything
            for (int i = 0; i < RIW.size(); i++) {
                auto RCW = RIW[i]->GetResliceCursorWidget();
                RCW->Render();
            }
            // this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
            // return;
        }

        for (int i = 0; i < RIW.size(); i++) {
            vtkPlaneSource* ps = static_cast<vtkPlaneSource*>(this->IPW[i]->GetPolyDataAlgorithm());
            auto RCW = RIW[i]->GetResliceCursorWidget();
            ps->SetOrigin(RCW->GetResliceCursorRepresentation()->GetPlaneSource()->GetOrigin());
            ps->SetPoint1(RCW->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint1());
            ps->SetPoint2(RCW->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint2());

            // If the reslice plane has modified, update it on the 3D widget
            this->IPW[i]->UpdatePlacement();
        }

        // Render everything
        for (int i = 0; i < RIW.size(); i++) {
            auto RCW = RIW[i]->GetResliceCursorWidget();
            RCW->Render();
        }
        // this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();

        if (ev == vtkCommand::MouseMoveEvent) {
            // Get the event position from the interactor
            vtkInteractorStyleImage* style = dynamic_cast<vtkInteractorStyleImage*>(caller);
            vtkResliceImageViewer* riw = nullptr;
            if (style) {
                // Figure out the current viewer
                vtkCornerAnnotation* ca = nullptr;
                for (int i = 0; i < RIW.size(); ++i) {
                    if (RIW[i]->GetInteractorStyle() == style) {
                        riw = RIW[i];
                        ca = CA[i];
                        break;
                    }
                }

                if (riw) {
                    // Get event position in display coordinates
                    int* eventPos = style->GetInteractor()->GetEventPosition();
                    vtkRenderer* curRen = style->GetInteractor()->FindPokedRenderer(eventPos[0], eventPos[1]);

                    vtkNew<vtkCellPicker> cellPicker;
                    cellPicker->SetTolerance(0.005);
                    cellPicker->AddPickList(riw->GetImageActor());
                    cellPicker->PickFromListOn();
                    cellPicker->Pick(eventPos[0], eventPos[1], 0, curRen);
                    double* worldPtReslice = cellPicker->GetPickPosition();

                    // Get the (i,j,k) indices of the point in the original data
                    double origin[3], spacing[3];
                    data->GetOrigin(origin);
                    data->GetSpacing(spacing);
                    int pt[3];
                    int extent[6];
                    data->GetExtent(extent);

                    int iq[3];
                    int iqtemp;
                    for (int i = 0; i < 3; i++) {
                        // compute world to image coords
                        iqtemp = vtkMath::Round((worldPtReslice[i] - origin[i]) / spacing[i]);

                        // we have a valid pick already, just enforce bounds check
                        iq[i] = (iqtemp < extent[2 * i]) ? extent[2 * i] : ((iqtemp > extent[2 * i + 1]) ? extent[2 * i + 1] : iqtemp);

                        pt[i] = iq[i];
                    }

                    short* val = static_cast<short*>(data->GetScalarPointer(pt));
                    if (ca) {
                        auto wl = riw->GetColorLevel();
                        auto ww = riw->GetColorWindow();
                        std::string msg = "WL: " + std::to_string(wl) + "\nWL: " + std::to_string(ww);

                        ca->SetText(0, msg.c_str());
                    }
                    riw->Render();
                }

                style->OnMouseMove();
            }
        }

        vtkImagePlaneWidget* ipw = dynamic_cast<vtkImagePlaneWidget*>(caller);
        if (ipw) {
            double* wl = static_cast<double*>(callData);
            for (auto& ipw_cand : IPW) {
                if (ipw != ipw_cand) {
                    ipw_cand->SetWindowLevel(wl[0], wl[1], 1);
                }
            }
        }
    }

    vtkResliceCursorCallback() { }
    std::vector<vtkSmartPointer<vtkImagePlaneWidget>> IPW;
    std::vector<vtkSmartPointer<vtkResliceImageViewer>> RIW;
    std::vector<vtkSmartPointer<vtkCornerAnnotation>> CA;
    vtkImageData* data = nullptr;
};
*/

std::shared_ptr<DataContainer> generateSampleData()
{
    auto data = std::make_shared<DataContainer>();
    data->setDimensions({ 8, 8, 8 });
    data->setSpacing({ 1, 1, 1 });
    std::vector<double> im(8 * 8 * 8, 0);
    for (int i = 0; i < im.size(); ++i)
        im[i] = i;
    data->setImageArray(DataContainer::ImageType::CT, im);
    return data;
}

SliceRenderWidget::SliceRenderWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumWidth(200);

    QTimer::singleShot(0, [=]() { this->setupPipeline(); });
}

void SliceRenderWidget::setupPipeline()
{
    // https://github.com/sankhesh/FourPaneViewer/blob/master/QtVTKRenderWindows.cxx

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    // gaussian smooth mapper
    imageSmoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    imageSmoother->SetDimensionality(3);
    imageSmoother->SetStandardDeviations(0.0, 0.0, 0.0);

    // lut
    lut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();

    for (int i = 0; i < 3; ++i) {
        openGLWidget[i] = new QVTKOpenGLNativeWidget(this);

        layout->addWidget(openGLWidget[i]);

        // renderers
        renderer[i] = vtkSmartPointer<vtkRenderer>::New();
        renderer[i]->GetActiveCamera()->ParallelProjectionOn();
        renderer[i]->SetBackground(0, 0, 0);

        // interaction style
        auto interactorStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
        interactorStyle->SetDefaultRenderer(renderer[i]);
        interactorStyle->SetInteractionModeToImageSlicing();
        // interactionStyle->SetInteractionModeToImage3D();

        auto renderWindowInteractor = openGLWidget[i]->interactor();
        renderWindowInteractor->SetInteractorStyle(interactorStyle);
        auto renWin = openGLWidget[i]->renderWindow();
        renWin->AddRenderer(renderer[i]);

        // auto textActorCorners = vtkSmartPointer<vtkCornerAnnotation>::New();
        // textActorCorners->SetText(1, "");
        // textActorCorners->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
        //  interactionStyle->setCornerAnnotation(textActorCorners);

        // colorbar
        // auto scalarColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
        // scalarColorBar->SetMaximumWidthInPixels(200);
        // scalarColorBar->AnnotationTextScalingOff();

        // reslice mapper
        // auto imageMapper = vtkSmartPointer<vtkOpenGLImageSliceMapper>::New();
        auto imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
        imageMapper->SetInputConnection(imageSmoother->GetOutputPort());
        imageMapper->SliceFacesCameraOn();
        imageMapper->SetSliceAtFocalPoint(true);

        // image slice
        imageSlice[i] = vtkSmartPointer<vtkImageSlice>::New();
        imageSlice[i]->SetMapper(imageMapper);
        renderer[i]->AddActor(imageSlice[i]);

        if (auto cam = renderer[i]->GetActiveCamera(); i == 0) {
            cam->SetFocalPoint(0, 0, 0);
            cam->SetPosition(0, 0, -1);
            cam->SetViewUp(0, -1, 0);
        } else if (i == 1) {
            cam->SetFocalPoint(0, 0, 0);
            cam->SetPosition(0, -1, 0);
            cam->SetViewUp(0, 0, 1);
        } else {
            cam->SetFocalPoint(0, 0, 0);
            cam->SetPosition(1, 0, 0);
            cam->SetViewUp(0, 0, 1);
        }

        auto sliceProperty = imageSlice[i]->GetProperty();
        sliceProperty->SetLookupTable(lut);
        lut->SetMinimumTableValue(0, 0, 0, 1);
        lut->SetMaximumTableValue(1, 1, 1, 1);
        lut->Build();
        sliceProperty->SetUseLookupTableScalarRange(false);
    }

    // setting dummy data to avoid pipeline errors
    updateImageData(generateSampleData());
}

void SliceRenderWidget::Render()
{
    for (auto& slic : imageSlice)
        slic->Update();
    for (auto& ren : renderer)
        ren->ResetCamera();
    for (auto& w : openGLWidget)
        w->renderWindow()->Render();
}

void SliceRenderWidget::setInteractionStyleTo3D()
{
}

void SliceRenderWidget::setInteractionStyleToSlicing()
{
}

void SliceRenderWidget::useFXAA(bool use)
{
    for (auto& ren : renderer)
        ren->SetUseFXAA(use);
}

void SliceRenderWidget::setMultisampleAA(int samples)
{
    int s = std::max(samples, int { 0 });
    for (auto& wid : openGLWidget) {
        auto renWin = wid->renderWindow();
        renWin->SetMultiSamples(s);
    }
}

void SliceRenderWidget::setNewImageData(vtkImageData* data)
{
    if (data) {
        imageSmoother->SetInputData(data);
        // imageSmoother->Update();
        // imageSlice[0]->Update();
        // renderer[0]->ResetCamera();
        // openGLWidget[0]->renderWindow()->Render();
        Render();
    }
}

void SliceRenderWidget::updateImageData(std::shared_ptr<DataContainer> data)
{
    if (data && m_data) {
        if (data->ID() == m_data->ID()) {
            // no changes
            return;
        }
    }
    bool uid_is_new = true;
    if (m_data && data)
        uid_is_new = data->ID() != m_data->ID();

    // updating images before replacing old buffer
    if (data) {
        if (data->hasImage(DataContainer::ImageType::CT) && uid_is_new) {
            setNewImageData(data->vtkImage(DataContainer::ImageType::CT));

            /* auto ps = riw->GetRenderer()->GetActiveCamera()->GetParallelScale();
            riw->SetInputData(m_data->vtkImage(DataContainer::ImageType::CT));
            // planeWidget->SetInputData(m_data->vtkImage(DataContainer::ImageType::CT));
            planeWidget->SetSliceIndex(0);

            riw->SetSlice(0);
            riw->GetRenderer()->ResetCamera();
            riw->GetRenderer()->GetActiveCamera()->SetParallelScale(ps);
            riw->Render();
            */
        }
    }
    m_data = data;
}
