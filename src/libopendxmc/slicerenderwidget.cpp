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
#include <colormaps.hpp>
#include <custominteractorstyleimage.hpp>
#include <niftiwrapper.hpp>
#include <slicerenderwidget.hpp>

#include <QDir>
#include <QFileDialog>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

#include <QVTKOpenGLNativeWidget.h>

#include <vtkCallbackCommand.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkImageActor.h>
#include <vtkImageProperty.h>
#include <vtkImageResliceMapper.h>
#include <vtkPNGWriter.h>
#include <vtkRenderWindow.h>
#include <vtkScalarBarActor.h>
#include <vtkTextProperty.h>
#include <vtkWindowToImageFilter.h>

#include <string>

constexpr std::array<double, 3> TEXT_COLOR = { 0.6, 0.5, 0.1 };

vtkSmartPointer<vtkImageData> generateSampleData()
{
    auto data = std::make_shared<DataContainer>();
    data->setDimensions({ 8, 8, 8 });
    data->setSpacing({ 1, 1, 1 });
    std::vector<double> im(8 * 8 * 8, 0);

    data->setImageArray(DataContainer::ImageType::CT, im);
    auto image = data->vtkImage(DataContainer::ImageType::CT);
    return image;
}

SliceRenderWidget::SliceRenderWidget(int orientation, bool lowerLeftText, bool colorbar, QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    openGLWidget = new QVTKOpenGLNativeWidget(this);
    layout->addWidget(openGLWidget);

    this->setLayout(layout);

    // lut
    m_lut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();

    lut_windowing[DataContainer::ImageType::CT] = std::make_pair(100, 800);
    lut_windowing[DataContainer::ImageType::Density] = std::make_pair(1.0, 1.0);

    setupSlicePipeline(orientation, lowerLeftText, colorbar);
    setNewImageData(generateSampleData());

    // adding settingsbutton
    auto settingsButton = new QPushButton(QIcon(":icons/settings.png"), QString {}, openGLWidget);
    settingsButton->setFlat(true);
    settingsButton->setIconSize(QSize(24, 24));
    settingsButton->setStyleSheet("QPushButton {background-color:transparent;}");
    auto menu = new QMenu(settingsButton);
    settingsButton->setMenu(menu);

    // adding settingsactions
    menu->addAction(QString(tr("Save slice (png)")), [this, orientation]() {
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        auto dirpath_str = settings.value("saveload/path", ".").value<QString>();
        QDir dirpath(dirpath_str);

        QString filename;
        if (orientation == 0)
            filename = dirpath.absoluteFilePath(QString("axial.png"));
        else if (orientation == 1)
            filename = dirpath.absoluteFilePath(QString("coronal.png"));
        else
            filename = dirpath.absoluteFilePath(QString("saggital.png"));

        filename = QFileDialog::getSaveFileName(this, tr("Save File"), filename, tr("Images (*.png)"));

        if (!filename.isEmpty()) {
            auto fileinfo = QFileInfo(filename);
            dirpath_str = fileinfo.absolutePath();
            settings.setValue("saveload/path", dirpath_str);
            auto renderWindow = this->openGLWidget->renderWindow();
            vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
            windowToImageFilter->SetInput(renderWindow);
            windowToImageFilter->SetScale(3, 3); // set the resolution of the output image (3 times the current resolution of vtk render window)
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
    menu->addAction(QString(tr("Export volume (nifti)")), [this]() {
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        auto dirpath_str = settings.value("saveload/path", ".").value<QString>();
        QDir dirpath(dirpath_str);

        const auto imagetype = this->lut_current_type;
        const auto image_desc = DataContainer::getImageAsString(imagetype);

        auto filename = QString::fromStdString(image_desc) + QString(".nii.gz");

        filename = QFileDialog::getSaveFileName(this, tr("Save File"), filename, tr("Nifti (*.nii.gz)"));

        if (!filename.isEmpty()) {
            auto fileinfo = QFileInfo(filename);
            dirpath_str = fileinfo.absolutePath();
            settings.setValue("saveload/path", dirpath_str);

            auto std_path = filename.toStdString();

            if (this->m_data){
                auto image = this->m_data->vtkImage(imagetype);
                NiftiWrapper writer;
                auto success = writer.save(std_path, image, imagetype);}
        }
    });
}

void SliceRenderWidget::setupSlicePipeline(int orientation, bool lowerLeftText, bool colorbar)
{
    // renderers
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->GetActiveCamera()->ParallelProjectionOn();
    m_renderer->SetBackground(0, 0, 0);

    // interaction style
    auto interactorStyle = vtkSmartPointer<CustomInteractorStyleImage>::New();
    interactorStyle->SetDefaultRenderer(m_renderer);
    interactorStyle->SetInteractionModeToImageSlicing();
    interactorStyle->AutoAdjustCameraClippingRangeOn();

    auto renderWindowInteractor = openGLWidget->interactor();
    renderWindowInteractor->SetInteractorStyle(interactorStyle);
    auto renWin = openGLWidget->renderWindow();
    renWin->AddRenderer(m_renderer);

    m_imageStack = vtkSmartPointer<vtkImageStack>::New();
    m_imageSliceFront = vtkSmartPointer<vtkImageActor>::New();
    m_imageSliceBack = vtkSmartPointer<vtkImageActor>::New();

    // sinc interpolator
    m_interpolatorSinc = vtkSmartPointer<vtkImageSincInterpolator>::New();
    m_interpolatorSinc->AntialiasingOn();

    // smoother
    m_smoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    m_smoother->SetDimensionality(3);
    m_smoother->SetRadiusFactor(0.0);
    m_smoother->SetStandardDeviation(0.0);

    std::array slices = { m_imageSliceFront, m_imageSliceBack };
    for (auto s : slices) {
        auto imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
        imageMapper->SliceFacesCameraOn();
        imageMapper->SliceAtFocalPointOn();
        imageMapper->JumpToNearestSliceOn();
        imageMapper->ReleaseDataFlagOff();
        imageMapper->StreamingOn();
        imageMapper->ResampleToScreenPixelsOn();
        imageMapper->AutoAdjustImageQualityOff();
        imageMapper->SetInterpolator(m_interpolatorSinc);
        if (s == m_imageSliceFront) {
            imageMapper->SetInputConnection(m_smoother->GetOutputPort());
        }
        s->SetMapper(imageMapper);
        s->InterpolateOn();
    }

    m_imageSliceFront->GetProperty()->SetLayerNumber(1);
    m_imageSliceBack->GetProperty()->SetLayerNumber(0);
    m_imageStack->AddImage(m_imageSliceFront);
    m_imageStack->SetActiveLayer(1);

    m_renderer->AddActor(m_imageStack);

    if (auto cam = m_renderer->GetActiveCamera(); orientation == 0) {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, 0, -1);
        cam->SetViewUp(0, -1, 0);
    } else if (orientation == 1) {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, -1, 0);
        cam->SetViewUp(0, 0, 1);
    } else {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(1, 0, 0);
        cam->SetViewUp(0, 0, 1);
    }

    { // setting up foreground lut
        auto sliceProperty = m_imageSliceFront->GetProperty();
        sliceProperty->SetLookupTable(m_lut);
        sliceProperty->UseLookupTableScalarRangeOff();
        m_lut->SetMinimumTableValue(0, 0, 0, 1);
        m_lut->SetMaximumTableValue(1, 1, 1, 1);
        m_lut->UseBelowRangeColorOn();
        m_lut->SetBelowRangeColor(0, 0, 0, 0);
        m_lut->Build();
        sliceProperty->SetColorLevel(lut_windowing[DataContainer::ImageType::CT].first);
        sliceProperty->SetColorWindow(lut_windowing[DataContainer::ImageType::CT].second);
    }

    { // adding background lut
        auto blut = vtkSmartPointer<vtkWindowLevelLookupTable>::New();
        auto sliceProperty = m_imageSliceBack->GetProperty();
        sliceProperty->SetLookupTable(blut);
        sliceProperty->UseLookupTableScalarRangeOff();
        blut->SetMinimumTableValue(0, 0, 0, 1);
        blut->SetMaximumTableValue(1, 1, 1, 1);
        blut->UseBelowRangeColorOn();
        blut->SetBelowRangeColor(0, 0, 0, 0);
        blut->Build();
        sliceProperty->SetColorLevel(lut_windowing[DataContainer::ImageType::CT].first);
        sliceProperty->SetColorWindow(lut_windowing[DataContainer::ImageType::CT].second);
    }

    // colorbar
    if (colorbar) {
        auto scalarColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
        scalarColorBar->SetNumberOfLabels(2);
        scalarColorBar->SetLookupTable(m_lut);
        scalarColorBar->SetUnconstrainedFontSize(true);
        scalarColorBar->SetBarRatio(0.1);
        auto txtStyle = vtkSmartPointer<vtkTextProperty>::New();
        txtStyle->SetColor(TEXT_COLOR.data());
        txtStyle->BoldOn();
        scalarColorBar->SetLabelTextProperty(txtStyle);
        scalarColorBar->SetTextPositionToPrecedeScalarBar();
        scalarColorBar->AnnotationTextScalingOff();
        m_renderer->AddActor(scalarColorBar);
    }

    if (lowerLeftText) {
        // unit txt
        m_unitText = vtkSmartPointer<vtkTextActor>::New();
        m_renderer->AddActor2D(m_unitText);
        auto txtStyle = m_unitText->GetTextProperty();
        txtStyle->SetColor(TEXT_COLOR.data());
        txtStyle->BoldOn();

        // window txt
        m_windowText = vtkSmartPointer<vtkTextActor>::New();
        m_windowText->SetTextProperty(txtStyle);
        m_renderer->AddActor2D(m_windowText);
    }
}

void SliceRenderWidget::registerStyleCallback(vtkSmartPointer<vtkCallbackCommand> cmd, const std::vector<vtkCommand::EventIds>& events)
{
    auto renderWindowInteractor = openGLWidget->interactor();
    auto style = renderWindowInteractor->GetInteractorStyle();
    for (auto ev : events)
        style->AddObserver(ev, cmd);
}

void SliceRenderWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    updateTextPositions(false);
}

void SliceRenderWidget::updateTextPositions(bool render)
{
    if (m_unitText) {
        double size[2];
        m_unitText->GetSize(m_renderer, size);
        auto rsize = m_renderer->GetSize();
        m_unitText->SetPosition(rsize[0] - size[0] - 5, 5);
        if (render)
            Render();
    }
}

void SliceRenderWidget::useFXAA(bool use)
{
    m_renderer->SetUseFXAA(use);
}

void SliceRenderWidget::setImageSmoothing(double pixels)
{
    m_smoother->SetStandardDeviation(pixels);
    m_smoother->SetRadiusFactor(std::max(1.0, pixels * 2));
    Render();
}

void SliceRenderWidget::setMultisampleAA(int samples)
{
    int s = std::max(samples, int { 0 });
    openGLWidget->renderWindow()->SetMultiSamples(s);
}

void SliceRenderWidget::setInterpolationType(int type)
{
    auto mapperFront = static_cast<vtkImageResliceMapper*>(m_imageSliceFront->GetMapper());
    auto mapperBack = static_cast<vtkImageResliceMapper*>(m_imageSliceBack->GetMapper());

    if (type < 3) {
        mapperFront->SetInterpolator(nullptr);
        mapperBack->SetInterpolator(nullptr);
        m_imageSliceFront->InterpolateOn();
        m_imageSliceBack->InterpolateOn();
        m_imageSliceFront->GetProperty()->SetInterpolationType(type);
        m_imageSliceBack->GetProperty()->SetInterpolationType(type);
    } else {
        mapperFront->SetInterpolator(m_interpolatorSinc);
        mapperBack->SetInterpolator(m_interpolatorSinc);
        m_imageSliceFront->InterpolateOff();
        m_imageSliceBack->InterpolateOff();
    }

    Render();
}

void SliceRenderWidget::setBackgroundColor(double r, double g, double b)
{
    m_renderer->SetBackground(r, g, b);
    Render();
}

void SliceRenderWidget::setUseCTDataBackground(bool on)
{
    m_useCTBackground = on;
    // setup background
    if (m_data->hasImage(DataContainer::ImageType::CT)) {
        bool currentTypeIsNotCt = m_imageSliceFront->GetMapper()->GetInput() != m_imageSliceBack->GetMapper()->GetInput();
        if (m_useCTBackground && currentTypeIsNotCt) {
            if (!m_imageStack->HasImage(m_imageSliceBack))
                m_imageStack->AddImage(m_imageSliceBack);
        } else {
            if (m_imageStack->HasImage(m_imageSliceBack))
                m_imageStack->RemoveImage(m_imageSliceBack);
        }
        Render();
    }
}

void SliceRenderWidget::switchLUTtable(DataContainer::ImageType type)
{
    auto prop = m_imageSliceFront->GetProperty();
    lut_windowing[lut_current_type] = std::make_pair(prop->GetColorLevel(), prop->GetColorWindow());

    if (type == DataContainer::ImageType::Material || type == DataContainer::ImageType::Organ) {
        auto vtkimage = m_data->vtkImage(type);
        auto n_colors = static_cast<int>(vtkimage->GetScalarRange()[1]) + 1;
        if (n_colors > 1 && m_lut->GetNumberOfColors() != n_colors) {
            m_lut->SetNumberOfTableValues(n_colors);
            m_lut->SetTableValue(0, 0, 0, 0, 0);
            for (int i = 1; i < n_colors; ++i) {
                auto rgba = Colormaps::discreetColor(i);
                m_lut->SetTableValue(i, rgba.data());
            }

            m_lut->SetTableRange(0, n_colors - 1);
            m_lut->Build();
        }
        prop->UseLookupTableScalarRangeOn();
    } else {
        prop->UseLookupTableScalarRangeOff();
        m_lut->SetNumberOfTableValues(256);
        m_lut->SetValueRange(0, 1);
        m_lut->SetMinimumTableValue(0, 0, 0, 1);
        m_lut->SetMaximumTableValue(1, 1, 1, 1);
        m_lut->ForceBuild();

        if (type == DataContainer::ImageType::Density) {
            // replacing with turbo cmap
            const auto map = Colormaps::colormapLongForm("TURBO");
            for (int i = 0; i < map.size() / 3; ++i) {
                const auto ii = i * 3;
                m_lut->SetTableValue(i, map[ii], map[ii + 1], map[ii + 2], 1.0);
            }
        } else if (type == DataContainer::ImageType::Dose) {
            // replacing with turbo cmap
            const auto map = Colormaps::colormapLongForm("TURBO");
            for (int i = 0; i < map.size() / 3; ++i) {
                const auto ii = i * 3;
                m_lut->SetTableValue(i, map[ii], map[ii + 1], map[ii + 2], 1.0);
            }
        }

        if (lut_windowing.contains(type)) {
            prop->SetColorLevel(lut_windowing[type].first);
            prop->SetColorWindow(lut_windowing[type].second);
        } else {
            auto vtkimage = m_data->vtkImage(type);
            std::array<double, 2> range;
            vtkimage->GetScalarRange(range.data());
            auto wl = (range[0] + range[1]) / 2;
            auto ww = range[1] - range[0];
            prop->SetColorLevel(wl);
            prop->SetColorWindow(ww);
        }
    }
    m_imageStack->Update();
    lut_current_type = type;
}

void SliceRenderWidget::showData(DataContainer::ImageType type)
{
    if (!m_data)
        return;
    if (m_data->hasImage(type)) {
        auto vtkimage = m_data->vtkImage(type);
        switchLUTtable(type);

        // setup background
        if (m_useCTBackground && type != DataContainer::ImageType::CT && m_data->hasImage(DataContainer::ImageType::CT)) {
            if (!m_imageStack->HasImage(m_imageSliceBack))
                m_imageStack->AddImage(m_imageSliceBack);
        } else {
            if (m_imageStack->HasImage(m_imageSliceBack))
                m_imageStack->RemoveImage(m_imageSliceBack);
        }

        setNewImageData(vtkimage, false);
        if (m_unitText) {
            m_unitText->SetInput(m_data->units(type).c_str());
            updateTextPositions();
        }
        Render();
    }
}

int argmax3(double v[3])
{
    if (std::abs(v[0]) > std::abs(v[1]) && std::abs(v[0]) > std::abs(v[2]))
        return 0;
    else if (std::abs(v[1]) > std::abs(v[0]) && std::abs(v[1]) > std::abs(v[2]))
        return 1;
    return 2;
}

void SliceRenderWidget::resetCamera()
{
    m_renderer->ResetCamera();
    auto camera = m_renderer->GetActiveCamera();
    double dir[3];
    camera->GetDirectionOfProjection(dir);
    int dIdx = argmax3(dir);

    double pos[3];
    camera->GetPosition(pos);
    for (int i = 0; i < 3; ++i) {
        if (i != dIdx) {
            pos[i] = 0;
        }
    }
    camera->SetPosition(pos);
    camera->SetFocalPoint(0.0, 0.0, 0.0);
    if (m_data) {
        const auto span = (m_data->dimensions()[dIdx] * m_data->spacing()[dIdx]) / 2;
        const auto dist = std::abs(pos[dIdx]);
        camera->SetClippingRange(dist - span, dist + span);
    }
}

void SliceRenderWidget::Render(bool reset_camera)
{
    if (reset_camera) {
        resetCamera();
    }
    openGLWidget->renderWindow()->Render();
}

void SliceRenderWidget::setNewImageData(vtkSmartPointer<vtkImageData> data, bool rezoom_camera)
{
    if (data) {
        m_smoother->SetInputData(data);
        m_imageSliceFront->SetDisplayExtent(data->GetExtent());
        Render(rezoom_camera);
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

    if (data) {
        m_data = data;
        if (data->hasImage(DataContainer::ImageType::CT) && uid_is_new) {
            auto vtkimage = data->vtkImage(DataContainer::ImageType::CT);
            m_imageSliceBack->GetMapper()->SetInputData(vtkimage);
            showData(DataContainer::ImageType::CT);
        } else if (data->hasImage(DataContainer::ImageType::Density) && uid_is_new) {
            auto vtkimage = data->vtkImage(DataContainer::ImageType::Density);
            showData(DataContainer::ImageType::Density);
        }
        Render(true);
    }
}

void SliceRenderWidget::addActor(vtkSmartPointer<vtkActor> actor)
{
    m_renderer->AddActor(actor);
    Render();
}

void SliceRenderWidget::removeActor(vtkSmartPointer<vtkActor> actor)
{
    m_renderer->RemoveActor(actor);
    Render();
}
