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
#include <QVTKOpenGLNativeWidget.h>

// #include "vtkImageReslice.h"
#include <vtkImageActor.h>
#include <vtkImageMapToColors.h>
#include <vtkImageMapper3D.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkMatrix4x4.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>

class vtkImageInteractionCallback : public vtkCommand {
public:
    static vtkImageInteractionCallback* New() { return new vtkImageInteractionCallback; }

    vtkImageInteractionCallback()
    {
    }

    void SetImageReslice(vtkSmartPointer<vtkImageReslice> reslice) { m_reslice = reslice; }

    void SetInteractor(vtkSmartPointer<vtkRenderWindowInteractor> interactor) { m_interactor = interactor; }

    void Execute(vtkObject*, unsigned long event, void*) override
    {

        int lastPos[2];
        m_interactor->GetLastEventPosition(lastPos);
        int currPos[2];
        m_interactor->GetEventPosition(currPos);

        if (event == vtkCommand::LeftButtonPressEvent) {
            this->Slicing = true;
        } else if (event == vtkCommand::LeftButtonReleaseEvent) {
            this->Slicing = false;
        } else if (event == vtkCommand::MouseMoveEvent) {
            if (this->Slicing) {

                // Increment slice position by deltaY of mouse
                int deltaY = lastPos[1] - currPos[1];

                m_reslice->Update();
                double sliceSpacing = m_reslice->GetOutput()->GetSpacing()[2];
                vtkMatrix4x4* matrix = m_reslice->GetResliceAxes();
                // move the center point that we are slicing through
                double point[4];
                double center[4];
                point[0] = 0.0;
                point[1] = 0.0;
                point[2] = sliceSpacing * deltaY;
                point[3] = 1.0;
                matrix->MultiplyPoint(point, center);
                matrix->SetElement(0, 3, center[0]);
                matrix->SetElement(1, 3, center[1]);
                matrix->SetElement(2, 3, center[2]);
                m_interactor->Render();
            } else {
                vtkInteractorStyle* style = vtkInteractorStyle::SafeDownCast(m_interactor->GetInteractorStyle());
                if (style) {
                    style->OnMouseMove();
                }
            }
        }
    }

private:
    // Actions (slicing only, for now)
    bool Slicing = false;

    // Pointer to vtkImageReslice
    vtkSmartPointer<vtkImageReslice> m_reslice = nullptr;

    // Pointer to the interactor
    vtkSmartPointer<vtkRenderWindowInteractor> m_interactor = nullptr;
};

SliceRenderWidget::SliceRenderWidget(QWidget* parent, SliceRenderWidget::Orientation orientation)
    : QWidget(parent)
{
    // https://gitlab.kitware.com/vtk/vtk/blob/master/Examples/ImageProcessing/Cxx/ImageSlicing.cxx
    auto openGLWidget = new QVTKOpenGLNativeWidget(this);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(openGLWidget);
    this->setLayout(layout);
    this->setMinimumWidth(20);

    // extract slice
    m_reslice = vtkSmartPointer<vtkImageReslice>::New();
    m_reslice->SetOutputDimensionality(2);
    m_reslice->SetResliceAxesDirectionCosines(1, 0, 0, 0, 1, 0, 0, 0, 1);
    m_reslice->SetInterpolationModeToCubic();

    // display image
    auto actor = vtkSmartPointer<vtkImageActor>::New();

    auto table = vtkSmartPointer<vtkLookupTable>::New();
    table->SetRange(-500, 500); // image intensity range
    table->SetValueRange(0.0, 1.0); // from black to white
    table->SetSaturationRange(0.0, 0.0); // no color saturation
    table->SetRampToLinear();
    table->Build();
    auto color = vtkSmartPointer<vtkImageMapToColors>::New();
    color->SetLookupTable(table);
    color->SetInputConnection(m_reslice->GetOutputPort());

    // add color handling here
    actor->GetMapper()->SetInputConnection(color->GetOutputPort());

    auto renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->SetBackground(0.0, 0.0, 0.0);
    auto renderWindow = openGLWidget->renderWindow();
    renderWindow->AddRenderer(renderer);

    // interaction
    auto imageStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
    auto interactor = openGLWidget->interactor();
    interactor->SetInteractorStyle(imageStyle);

    auto callback = vtkSmartPointer<vtkImageInteractionCallback>::New();
    callback->SetImageReslice(m_reslice);
    callback->SetInteractor(interactor);

    imageStyle->AddObserver(vtkCommand::MouseMoveEvent, callback);
    imageStyle->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
    imageStyle->AddObserver(vtkCommand::LeftButtonReleaseEvent, callback);
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
            auto vtkimage = data->vtkImage(DataContainer::ImageType::CT);
            m_reslice->SetInputData(vtkimage);
        }
    }
    m_data = data;
}
