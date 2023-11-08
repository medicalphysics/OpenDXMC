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
    renderer = vtkSmartPointer<vtkOpenGLRenderer>::New();
    openGLWidget->renderWindow()->AddRenderer(renderer);

    auto renderWindowInteractor = openGLWidget->interactor();
    auto interactorStyle = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactorStyle->SetDefaultRenderer(renderer);
    renderWindowInteractor->SetInteractorStyle(interactorStyle);

    // create tables
    auto otf = vtkSmartPointer<vtkPiecewiseFunction>::New();

    // create volume and mapper
    mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    mapper->LockSampleDistanceToInputSpacingOn();
    mapper->AutoAdjustSampleDistancesOn();
    volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);

    lut = vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
    lut->SetIndexedLookup(false); // true for indexed lookup
    lut->SetDiscretize(false);
    lut->SetClamping(true); // map values outside range to closest value not black

    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetIndependentComponents(true);
    volumeProperty->SetScalarOpacity(otf);
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetColor(lut);
    volume->SetProperty(volumeProperty);
    renderer->AddVolume(volume);
    renderer->ResetCamera();

    auto camera = renderer->GetActiveCamera();
    camera->SetPosition(56.8656, -297.084, 78.913);
    camera->SetFocalPoint(109.139, 120.604, 63.5486);
    camera->SetViewUp(-0.00782421, -0.0357807, -0.999329);
    camera->SetDistance(421.227);
    camera->SetClippingRange(146.564, 767.987);

    lut->AddRGBPoint(-3024, 0, 0, 0, 0.5, 0.0);
    lut->AddRGBPoint(-1000, .62, .36, .18, 0.5, 0.0);
    lut->AddRGBPoint(-500, .88, .60, .29, 0.33, 0.45);
    lut->AddRGBPoint(3071, .83, .66, 1, 0.5, 0.0);

    otf->AddPoint(-3024, 0, 0.5, 0.0);
    otf->AddPoint(-1000, 0, 0.5, 0.0);
    otf->AddPoint(-500, 1.0, 0.33, 0.45);
    otf->AddPoint(3071, 1.0, 0.5, 0.0);

    /*

    // create volume and mapper
    auto mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    mapper->SetInputConnection(imageSmoother->GetOutputPort());
    auto volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);
    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetIndependentComponents(true);
    volumeProperty->SetColor(ctf);
    volumeProperty->SetScalarOpacity(otf);
    volumeProperty->SetInterpolationTypeToLinear();
    volume->SetProperty(volumeProperty);

    ctf->AddRGBPoint(-3024, 0, 0, 0, 0.5, 0.0);
    ctf->AddRGBPoint(-1000, .62, .36, .18, 0.5, 0.0);
    ctf->AddRGBPoint(-500, .88, .60, .29, 0.33, 0.45);
    ctf->AddRGBPoint(3071, .83, .66, 1, 0.5, 0.0);

    otf->AddPoint(-3024, 0, 0.5, 0.0);
    otf->AddPoint(-1000, 0, 0.5, 0.0);
    otf->AddPoint(-500, 1.0, 0.33, 0.45);
    otf->AddPoint(3071, 1.0, 0.5, 0.0);

    ren->AddVolume(volume);
    ren->ResetCamera();

    auto camera = ren->GetActiveCamera();
    camera->SetPosition(56.8656, -297.084, 78.913);
    camera->SetFocalPoint(109.139, 120.604, 63.5486);
    camera->SetViewUp(-0.00782421, -0.0357807, -0.999329);
    camera->SetDistance(421.227);
    camera->SetClippingRange(146.564, 767.987);
}
*/
}

void VolumerenderWidget::Render()
{
    openGLWidget->renderWindow()->Render();
}

void VolumerenderWidget::setNewImageData(vtkSmartPointer<vtkImageData> data)
{
    if (data) {
        mapper->SetInputData(data);
        mapper->Update();
        double* scalar_range = data->GetScalarRange();
        emit imageDataChanged(data.Get());
        Render();
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
            setNewImageData(vtkimage);
        }
    }

    m_data = data;
}

VolumerenderSettingsWidget* VolumerenderWidget::createSettingsWidget(QWidget* parent)
{
    if (!parent)
        parent = this;
    auto prop = volume->GetProperty();
    auto wid = new VolumerenderSettingsWidget(mapper, volume, prop->GetScalarOpacity(), lut, parent);
    connect(this, &VolumerenderWidget::imageDataChanged, [=](vtkImageData* data) { wid->setImageData(data); });
    connect(wid, &VolumerenderSettingsWidget::requestRender, this, &VolumerenderWidget::Render);
    return wid;
}

/*
#include "opendxmc/dxmc_specialization.h"
#include "opendxmc/qpathmanipulation.h"
#include "opendxmc/volumerenderwidget.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <vtkColorTransferFunction.h>
#include <vtkDataArray.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkLinearTransform.h>
#include <vtkPNGWriter.h>
#include <vtkPiecewiseFunction.h>
#include <vtkPointData.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkVolume.h>
#include <vtkWindowToImageFilter.h>

#include <cmath>

VolumeRenderWidget::VolumeRenderWidget(QWidget* parent)
    : QWidget(parent)
    , m_renderMode(0)
{
    qRegisterMetaType<std::shared_ptr<ImageContainer>>();
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
    m_volumeProps.push_back(m_orientationProp.get());
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
    connect(smoothSlider, &QSlider::valueChanged, [=](int value) { m_imageSmoother->SetStandardDeviations(static_cast<double>(value), static_cast<double>(value), static_cast<double>(value)); });
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
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        auto dirname = directoryPath(settings.value("saveload/path", ".").value<QString>());

        auto filename = filePath(dirname, QString("volume.png"));
        filename = QFileDialog::getSaveFileName(this, tr("Save File"), filename, tr("Images (*.png)"));

        if (!filename.isEmpty()) {
            dirname = directoryPath(filename);
            settings.setValue("saveload/path", dirname);
            auto renderWindow = m_openGLWidget->renderWindow();
            vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
            windowToImageFilter->SetInput(renderWindow);
            //this is somehow broken
            windowToImageFilter->SetScale(3, 3); //set the resolution of the output image (3 times the current resolution of vtk render window)
            windowToImageFilter->SetInputBufferTypeToRGBA(); //also record the alpha (transparency) channel
            windowToImageFilter->ReadFrontBufferOn(); // read from the front buffer
            windowToImageFilter->SetFixBoundary(true);
            windowToImageFilter->ShouldRerenderOn();
            windowToImageFilter->Update();
            vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
            writer->SetFileName(filename.toLatin1().data());
            writer->SetInputConnection(windowToImageFilter->GetOutputPort());
            writer->Write();
        }
    });
}

VolumeRenderWidget::~VolumeRenderWidget()
{
    if (m_settingsWidget) {
        delete m_settingsWidget;
        m_settingsWidget = nullptr;
    }
}

void VolumeRenderWidget::updateRendering(void)
{
    if (m_volume)
        m_volume->Update();
    m_openGLWidget->renderWindow()->Render();
    m_openGLWidget->update();
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
    if (m_imageData) {
        if (image->image == m_imageData->image) {
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
    double* bounds = m_imageData->image->GetBounds();
    m_orientationProp->getActor()->SetPosition(bounds[0], bounds[2], bounds[4]);
    for (auto prop : m_volumeProps)
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
    m_volumeMapper->SetInteractiveUpdateRate(0.00001);
    m_volumeMapper->AutoAdjustSampleDistancesOn();
    m_volumeMapper->CroppingOn();
    int* extent = m_imageData->image->GetExtent();
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
    if (m_volumeMapper) {
        if (mode == 0) {
            m_volumeMapper->SetRequestedRenderModeToRayCast();
        }
        if (mode == 1) {
            m_volumeMapper->SetRequestedRenderModeToGPU();
        }
#ifdef ENABLE_OSPRAY
        if (mode == 2) {
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

void VolumeRenderWidget::addActorContainer(SourceActorContainer* actorContainer)
{
    auto actor = actorContainer->getActor();
    auto present = m_renderer->GetActors()->IsItemPresent(actor);
    if (present == 0) {
        if (m_imageData)
            actorContainer->setOrientation(m_imageData->directionCosines);
        if (m_actorsVisible)
            m_renderer->AddActor(actor);
        m_volumeProps.push_back(actorContainer);
    }
    updateRendering();
}

void VolumeRenderWidget::removeActorContainer(SourceActorContainer* actorContainer)
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
    m_actorsVisible = visible;
    for (auto m : m_volumeProps) {
        auto actor = m->getActor();
        if (m_actorsVisible)
            m_renderer->AddActor(actor);
        else
            m_renderer->RemoveActor(actor);
    }
    updateRendering();
}
*/