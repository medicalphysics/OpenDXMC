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

#include <beamactorcontainer.hpp>
#include <volumerendersettingswidget.hpp>
#include <volumerenderwidget.hpp>

#include <QChartView>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QString>
#include <QVBoxLayout>

#include <QVTKInteractor.h>
#include <vtkCamera.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkPNGWriter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkRenderWindow.h>
#include <vtkVolumeProperty.h>
#include <vtkWindowToImageFilter.h>

vtkSmartPointer<vtkImageData> generateSampleDataVolume()
{
    auto data = std::make_shared<DataContainer>();
    data->setDimensions({ 8, 8, 8 });
    data->setSpacing({ 1, 1, 1 });
    std::vector<double> im(8 * 8 * 8, 0);

    data->setImageArray(DataContainer::ImageType::CT, im);
    auto image = data->vtkImage(DataContainer::ImageType::CT);
    return image;
}

VolumerenderWidget::VolumerenderWidget(QWidget* parent)
    : QWidget(parent)
{

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    openGLWidget = new QVTKOpenGLNativeWidget(this);
    auto window = openGLWidget->renderWindow();
    window->SetLineSmoothing(true);
    layout->addWidget(openGLWidget);

    this->setLayout(layout);

    setupRenderingPipeline();
    setNewImageData(generateSampleDataVolume());

    // adding settingsbutton
    auto settingsButton = new QPushButton(QIcon(":icons/settings.png"), QString {}, openGLWidget);
    settingsButton->setFlat(true);
    settingsButton->setIconSize(QSize(24, 24));
    settingsButton->setStyleSheet("QPushButton {background-color:transparent;}");
    auto menu = new QMenu(settingsButton);
    settingsButton->setMenu(menu);

    // adding settingsactions
    menu->addAction(QString(tr("Save image")), [this]() {
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        auto dirpath_str = settings.value("saveload/path", ".").value<QString>();
        QDir dirpath(dirpath_str);
        auto filename = dirpath.absoluteFilePath("volume.png");
        filename = QFileDialog::getSaveFileName(this, tr("Save File"), filename, tr("Images (*.png)"));

        if (!filename.isEmpty()) {
            auto fileinfo = QFileInfo(filename);
            dirpath_str = fileinfo.absolutePath();
            settings.setValue("saveload/path", dirpath_str);
            auto renderWindow = this->openGLWidget->renderWindow();
            vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
            windowToImageFilter->SetInput(renderWindow);
            auto size = renderWindow->GetSize();
            int N = 1;
            while (size[0] * N < 2048 || size[1] * N < 2048)
                N++;
            windowToImageFilter->SetScale(N, N); // set the resolution of the output image (3 times the current resolution of vtk render window)
            windowToImageFilter->SetFixBoundary(true);
            windowToImageFilter->ShouldRerenderOn();
            windowToImageFilter->SetInputBufferTypeToRGBA(); // also record the alpha (transparency) channel
            windowToImageFilter->ReadFrontBufferOn(); // read from the front buffer
            windowToImageFilter->Update();
            vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
            writer->SetFileName(filename.toLatin1().data());
            writer->SetInputConnection(windowToImageFilter->GetOutputPort());
            writer->Write();
            renderWindow->Render();
        }
    });
}

void VolumerenderWidget::setupRenderingPipeline()
{
    auto renderer = vtkSmartPointer<vtkRenderer>::New();

    openGLWidget->renderWindow()->AddRenderer(renderer);

    auto renderWindowInteractor = openGLWidget->interactor();
    auto interactorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactorStyle->SetDefaultRenderer(renderer);
    renderWindowInteractor->SetInteractorStyle(interactorStyle);

    // create tables
    auto otf = vtkSmartPointer<vtkPiecewiseFunction>::New();

    // create volume and mapper
    auto mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    // mapper->LockSampleDistanceToInputSpacingOff();
    mapper->AutoAdjustSampleDistancesOff();
    mapper->SetSampleDistance(0.2);

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

void VolumerenderWidget::addActor(vtkSmartPointer<vtkActor> actor)
{
    m_settings->renderer()->AddActor(actor);
    m_settings->render();
}

void VolumerenderWidget::removeActor(vtkSmartPointer<vtkActor> actor)
{
    m_settings->renderer()->RemoveActor(actor);
    m_settings->render();
}

void VolumerenderWidget::setNewImageData(vtkSmartPointer<vtkImageData> data, bool reset_camera)
{
    if (data) {
        m_settings->setCurrentImageData(data, reset_camera);
    }
}

void VolumerenderWidget::setBackgroundColor(double r, double g, double b)
{
    m_settings->renderer()->SetBackground(r, g, b);
    Render();
}

void VolumerenderWidget::Render()
{
    m_settings->render();
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
