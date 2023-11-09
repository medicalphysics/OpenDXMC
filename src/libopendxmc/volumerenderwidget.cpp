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

Copyright 2023 Erlend Andersen
*/

#include <volumerendersettingswidget.hpp>
#include <volumerenderwidget.hpp>

#include <QChartView>
#include <QVBoxLayout>

#include <vtkCamera.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolumeProperty.h>

VolumerenderWidget::VolumerenderWidget(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    openGLWidget = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(openGLWidget);

    this->setLayout(layout);

    setupRenderingPipeline();
}

void VolumerenderWidget::setupRenderingPipeline()
{
    m_settings.renderer = vtkSmartPointer<vtkOpenGLRenderer>::New();
    openGLWidget->renderWindow()->AddRenderer(m_settings.renderer);

    auto renderWindowInteractor = openGLWidget->interactor();
    auto interactorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactorStyle->SetDefaultRenderer(m_settings.renderer);
    renderWindowInteractor->SetInteractorStyle(interactorStyle);

    // create tables
    auto otf = vtkSmartPointer<vtkPiecewiseFunction>::New();

    // create volume and mapper
    m_settings.mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    m_settings.mapper->LockSampleDistanceToInputSpacingOn();
    m_settings.mapper->AutoAdjustSampleDistancesOn();
    m_settings.volume = vtkSmartPointer<vtkVolume>::New();
    m_settings.volume->SetMapper(m_settings.mapper);

    m_settings.color_lut = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
    m_settings.color_lut->SetIndexedLookup(false); // true for indexed lookup
    m_settings.color_lut->SetDiscretize(false);
    m_settings.color_lut->SetClamping(true); // map values outside range to closest value not black

    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetIndependentComponents(true);
    volumeProperty->SetScalarOpacity(otf);
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetColor(m_settings.color_lut);
    m_settings.volume->SetProperty(volumeProperty);
    m_settings.renderer->AddVolume(m_settings.volume);
    m_settings.renderer->ResetCamera();

    auto camera = m_settings.renderer->GetActiveCamera();

    m_settings.color_lut->AddRGBPoint(-3024, 0, 0, 0, 0.5, 0.0);
    m_settings.color_lut->AddRGBPoint(-1000, .62, .36, .18, 0.5, 0.0);
    m_settings.color_lut->AddRGBPoint(-500, .88, .60, .29, 0.33, 0.45);
    m_settings.color_lut->AddRGBPoint(3071, .83, .66, 1, 0.5, 0.0);

    otf->AddPoint(-1.0, 0.0, 0.0, 0.0);
    otf->AddPoint(-0.5, 0.0, 0.0, 0.0);
    otf->AddPoint(0.0, 0.8, 0.0, 0.0);
    otf->AddPoint(0.5, 0.0, 0.0, 0.0);
    otf->AddPoint(1.0, 0.0, 0.0, 0.0);
}

void VolumerenderWidget::Render(bool rezoom_camera)
{
    if (rezoom_camera)
        m_settings.renderer->ResetCamera();
    openGLWidget->renderWindow()->Render();
}

void VolumerenderWidget::setNewImageData(vtkSmartPointer<vtkImageData> data, bool rezoom_camera)
{
    if (data) {
        m_settings.mapper->SetInputData(data);
        m_settings.mapper->Update();
        m_settings.currentImageData = data;
        data->GetScalarRange(m_settings.currentImageDataScalarRange.data());
        emit imageDataChanged();
        Render(rezoom_camera);
    }
}

void VolumerenderWidget::updateImageData(std::shared_ptr<DataContainer> data)
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
            setNewImageData(vtkimage, uid_is_new);
        }
    }
    m_data = data;
}

VolumerenderSettingsWidget* VolumerenderWidget::createSettingsWidget(QWidget* parent)
{
    if (!parent)
        parent = this;

    auto wid = new VolumerenderSettingsWidget(&m_settings, parent);
    connect(this, &VolumerenderWidget::imageDataChanged, [=]() { wid->imageDataUpdated(); });
    return wid;
}
