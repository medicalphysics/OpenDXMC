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

#include <QVBoxLayout>

#include <vtkCamera.h>
#include <vtkCellPicker.h>
#include <vtkCornerAnnotation.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper3D.h>
#include <vtkImageResliceMapper.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkPlane.h>
#include <vtkPlaneSource.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkResliceCursor.h>
#include <vtkResliceCursorActor.h>
#include <vtkResliceCursorLineRepresentation.h>
#include <vtkResliceCursorPolyDataAlgorithm.h>
#include <vtkResliceCursorWidget.h>
#include <vtkResliceImageViewer.h>
#include <vtkTextProperty.h>

#include <vtkDICOMImageReader.h>

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
            this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
            return;
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
        this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();

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
                        // ca = CA[i];
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
                    std::cout << "worldPtReslice " << worldPtReslice[0] << " " << worldPtReslice[1] << " " << worldPtReslice[2] << std::endl;

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
                        std::ostringstream annotation;
                        annotation << "(" << pt[0] << ", " << pt[1] << ", " << pt[2] << ") " << val[0];
                        std::cout << "(" << pt[0] << ", " << pt[1] << ", " << pt[2] << ") " << val[0] << std::endl;
                        ca->SetText(0, annotation.str().c_str());
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
    // std::vector<vtkSmartPointer<vtkCornerAnnotation>> CA;
    vtkImageData* data = nullptr;
};

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
    //https://github.com/sankhesh/FourPaneViewer/blob/master/QtVTKRenderWindows.cxx
    auto reader = vtkSmartPointer<vtkDICOMImageReader>::New();
    reader->SetDirectoryName("C:/Users/ander/testbilder");
    reader->ReleaseDataFlagOn();
    reader->SetDataScalarTypeToFloat();
    reader->Update();
    int imageDims[3];
    auto image = reader->GetOutput();
    reader->GetOutput()->GetDimensions(imageDims);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    for (int i = 0; i < 3; ++i) {
        auto openGLWidget = new QVTKOpenGLNativeWidget(this);
        openGLWidgets.push_back(openGLWidget);
        layout->addWidget(openGLWidget);
        auto window = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
        auto r = vtkSmartPointer<vtkResliceImageViewer>::New();
        riw.push_back(r);
        openGLWidget->setRenderWindow(window);
        riw[i]->SetRenderWindow(openGLWidget->renderWindow());
        riw[i]->SetupInteractor(openGLWidget->renderWindow()->GetInteractor());
    }
    setLayout(layout);

    for (int i = 0; i < 3; ++i) {
        // make them all share the same reslice cursor object.
        vtkResliceCursorLineRepresentation* rep = vtkResliceCursorLineRepresentation::SafeDownCast(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());

        rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(i);

        riw[i]->SetInputData(reader->GetOutput());
        riw[i]->SetSliceOrientation(i);
        riw[i]->SetResliceModeToAxisAligned();
    }

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.005);

    vtkSmartPointer<vtkProperty> ipwProp = vtkSmartPointer<vtkProperty>::New();

    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();

    auto iren = openGLWidgets[0]->renderWindow()->GetInteractor();
    // vtkRenderWindowInteractor* iren = openGLWidgets[0]->renderWindow()->GetInteractor();

    for (int i = 0; i < 3; i++) {
        planeWidget.push_back(vtkSmartPointer<vtkImagePlaneWidget>::New());
        planeWidget[i]->SetInteractor(iren);
        planeWidget[i]->SetPicker(picker);
        planeWidget[i]->RestrictPlaneToVolumeOn();
        double color[3] = { 0, 0, 0 };
        color[i] = 1;
        planeWidget[i]->GetPlaneProperty()->SetColor(color);

        color[0] /= 4.0;
        color[1] /= 4.0;
        color[2] /= 4.0;
        riw[i]->GetRenderer()->SetBackground(color);

        planeWidget[i]->SetTexturePlaneProperty(ipwProp);
        planeWidget[i]->TextureInterpolateOff();
        planeWidget[i]->SetResliceInterpolateToLinear();
        planeWidget[i]->SetInputConnection(reader->GetOutputPort());
        planeWidget[i]->SetPlaneOrientation(i);
        planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
        riw[i]->SetSlice(imageDims[i] / 2);
        planeWidget[i]->DisplayTextOn();
        planeWidget[i]->SetDefaultRenderer(ren);
        planeWidget[i]->SetWindowLevel(1358, -27);
        planeWidget[i]->On();
        planeWidget[i]->InteractionOn();
    }

    vtkSmartPointer<vtkResliceCursorCallback> cbk = vtkSmartPointer<vtkResliceCursorCallback>::New();
    cbk->data = reader->GetOutput();

    for (int i = 0; i < 3; i++) {
        cbk->IPW.push_back(planeWidget[i]);
        cbk->RIW.push_back(riw[i]);

        riw[i]->GetResliceCursorWidget()->AddObserver(
            vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(
            vtkResliceCursorWidget::WindowLevelEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(
            vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(
            vtkResliceCursorWidget::ResetCursorEvent, cbk);
        riw[i]->GetInteractorStyle()->AddObserver(
            vtkCommand::WindowLevelEvent, cbk);
        riw[i]->GetInteractorStyle()->AddObserver(
            vtkCommand::MouseMoveEvent, cbk);

        // Make them all share the same color map.
        riw[i]->SetLookupTable(riw[0]->GetLookupTable());
        planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());
        planeWidget[i]->SetColorMap(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());

        /* vtkSmartPointer<vtkCornerAnnotation> cornerText = vtkSmartPointer<vtkCornerAnnotation>::New();
        cornerText->SetText(0, "(X, Y, Z) I");
        cornerText->GetTextProperty()->SetColor(0.6, 0.5, 0.1);
        riw[i]->GetRenderer()->AddViewProp(cornerText);
        cbk->CA.push_back(cornerText);
        */
    }
    resetViews();
}

void SliceRenderWidget::resetViews()
{
    // Reset the reslice image views
    for (int i = 0; i < 3; i++) {
        riw[i]->Reset();
    }

    // Also sync the Image plane widget on the 3D top right view with any
    // changes to the reslice cursor.
    for (int i = 0; i < 3; i++) {
        vtkPlaneSource* ps = static_cast<vtkPlaneSource*>(
            planeWidget[i]->GetPolyDataAlgorithm());
        ps->SetNormal(riw[0]->GetResliceCursor()->GetPlane(i)->GetNormal());
        ps->SetCenter(riw[0]->GetResliceCursor()->GetPlane(i)->GetOrigin());

        // If the reslice plane has modified, update it on the 3D widget
        this->planeWidget[i]->UpdatePlacement();
    }

    // Render in response to changes.
    this->Render();
}

void SliceRenderWidget::Render()
{
    for (int i = 0; i < 3; i++) {
        riw[i]->Render();
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
