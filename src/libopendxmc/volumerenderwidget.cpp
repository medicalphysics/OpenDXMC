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
    auto renderer = vtkSmartPointer<vtkOpenGLRenderer>::New();

    openGLWidget->renderWindow()->AddRenderer(renderer);

    auto renderWindowInteractor = openGLWidget->interactor();
    auto interactorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactorStyle->SetDefaultRenderer(renderer);
    renderWindowInteractor->SetInteractorStyle(interactorStyle);

    // create tables
    auto otf = vtkSmartPointer<vtkPiecewiseFunction>::New();

    // create volume and mapper
    auto mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    mapper->LockSampleDistanceToInputSpacingOn();
    mapper->AutoAdjustSampleDistancesOn();
    auto volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);

    auto color_lut = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
    color_lut->SetIndexedLookup(false); // true for indexed lookup
    color_lut->SetDiscretize(false);
    color_lut->SetClamping(true); // map values outside range to closest value not black

    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetIndependentComponents(true);
    volumeProperty->SetScalarOpacity(otf);
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetColor(color_lut);
    volume->SetProperty(volumeProperty);
    renderer->AddVolume(volume);

    m_settings = new VolumeRenderSettings(renderer, mapper, volume, color_lut, this);
}

void VolumerenderWidget::setNewImageData(vtkSmartPointer<vtkImageData> data, bool reset_camera)
{
    if (data) {
        m_settings->setCurrentImageData(data, reset_camera);
    }
}

void VolumerenderWidget::showData(DataContainer::ImageType type)
{
    if (!m_data)
        return;
    if (m_data->hasImage(type)) {
        auto vtkimage = m_data->vtkImage(type);
        setNewImageData(vtkimage, false);
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
        } else if (data->hasImage(DataContainer::ImageType::Density)) {
            auto vtkimage = data->vtkImage(DataContainer::ImageType::Density);
            setNewImageData(vtkimage, uid_is_new);
        }
    }
    m_data = data;
}

VolumerenderSettingsWidget* VolumerenderWidget::createSettingsWidget(QWidget* parent)
{
    if (!parent)
        parent = this;

    auto wid = new VolumerenderSettingsWidget(m_settings, parent);
    return wid;
}
