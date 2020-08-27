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
#include "opendxmc/slicerenderwidget.h"
#include "opendxmc/colormap.h"

#include "dxmc/vectormath.h"

#include <QColor>
#include <QColorDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QIcon>
#include <QLabel>
#include <QMenu>
#include <QProgressDialog>
#include <QPushButton>
#include <QSettings>
#include <QSlider>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidgetAction>

#include <vtkCamera.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkImageProperty.h>
#include <vtkLookupTable.h>
#include <vtkPNGWriter.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <vtkTextProperty.h>
#include <vtkWindowLevelLookupTable.h>
#include <vtkWindowToImageFilter.h>

#ifdef WINDOWS
#include <vtkAVIWriter.h>
#endif

#include <charconv>

std::shared_ptr<ImageContainer> makeStartImage(void)
{
    std::array<double, 3> sp = { 1, 1, 1 };
    std::array<double, 3> o = { 0, 0, 0 };
    std::array<std::size_t, 3> dim = { 64, 64, 64 };

    auto data = std::make_shared<std::vector<float>>(dim[0] * dim[1] * dim[2], 0);
    auto im = std::make_shared<ImageContainer>(ImageContainer::ImageType::Empty, data, dim, sp, o);
    im->ID = 0;
    return im;
}

SliceRenderWidget::SliceRenderWidget(QWidget* parent, SliceRenderWidget::Orientation orientation)
    : QWidget(parent)
    , m_orientation(orientation)
{
    m_openGLWidget = new QVTKOpenGLNativeWidget(this);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_openGLWidget);
    this->setLayout(layout);

    //mapper
    m_imageSmoother = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    m_imageSmoother->SetDimensionality(3);
    m_imageSmoother->SetStandardDeviations(0.0, 0.0, 0.0);

    m_imageMapper = vtkSmartPointer<vtkImageResliceMapper>::New();
    m_imageMapper->StreamingOn();
    m_imageMapper->SetInputConnection(m_imageSmoother->GetOutputPort());

    m_imageMapperBackground = vtkSmartPointer<vtkImageResliceMapper>::New();
    m_imageMapperBackground->StreamingOn();

    m_imageSlice = vtkSmartPointer<vtkImageSlice>::New();
    m_imageSlice->SetMapper(m_imageMapper);

    m_imageSliceBackground = vtkSmartPointer<vtkImageSlice>::New();
    m_imageSliceBackground->SetMapper(m_imageMapperBackground);

    //renderer
    // Setup renderers
    m_renderer = vtkSmartPointer<vtkRenderer>::New();
    m_renderer->UseFXAAOn();
    m_renderer->GetActiveCamera()->ParallelProjectionOn();
    m_renderer->SetBackground(0, 0, 0);

    //render window
    //https://lorensen.github.io/VTKExamples/site/Cxx/Images/ImageSliceMapper/
    //http://vtk.org/gitweb?p=VTK.git;a=blob;f=Examples/GUI/Qt/FourPaneViewer/QtVTKRenderWindows.cxx
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    renderWindow->AddRenderer(m_renderer);
    m_openGLWidget->setRenderWindow(renderWindow);

    // Setup render window interactor
    vtkSmartPointer<vtkRenderWindowInteractor> renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

    m_interactionStyle = vtkSmartPointer<customMouseInteractorStyle>::New();
    m_interactionStyle->setMapper(m_imageMapper);
    m_interactionStyle->setMapperBackground(m_imageMapperBackground);
    m_interactionStyle->setRenderWindow(renderWindow);
    m_interactionStyle->setCallback([=]() { emit this->sourceActorChanged(); });

    //vtkSmartPointer<vtkInteractorStyleImage> m_interactionStyle = vtkSmartPointer<vtkInteractorStyleImage>::New();
    //m_interactionStyle->SetInteractionModeToImage3D();
    renderWindowInteractor->SetInteractorStyle(m_interactionStyle);
    renderWindowInteractor->SetRenderWindow(renderWindow);

    m_textActorCorners = vtkSmartPointer<vtkCornerAnnotation>::New();
    m_textActorCorners->SetText(1, "");
    m_textActorCorners->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    m_interactionStyle->setCornerAnnotation(m_textActorCorners);

    //adding colorbar
    m_scalarColorBar = vtkSmartPointer<vtkScalarBarActor>::New();
    m_scalarColorBar->SetMaximumWidthInPixels(200);
    m_scalarColorBar->AnnotationTextScalingOff();

    //other
    m_imageMapper->SliceFacesCameraOn();
    m_imageMapperBackground->SliceFacesCameraOn();
    m_imageMapper->SetJumpToNearestSlice(true);
    m_imageMapperBackground->SetJumpToNearestSlice(true);
    m_imageMapper->SetSliceAtFocalPoint(true);
    m_imageMapperBackground->SetSliceAtFocalPoint(true);

    // filling pipline with som data
    vtkSmartPointer<vtkImageData> dummyData = vtkSmartPointer<vtkImageData>::New();
    dummyData->SetDimensions(1, 1, 1);
    dummyData->AllocateScalars(VTK_FLOAT, 1);
    m_imageSmoother->SetInputData(dummyData);
    m_imageMapperBackground->SetInputData(dummyData);

    if (auto cam = m_renderer->GetActiveCamera(); m_orientation == Orientation::Axial) {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, 0, -1);
        cam->SetViewUp(0, -1, 0);
    } else if (m_orientation == Orientation::Coronal) {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(0, -1, 0);
        cam->SetViewUp(0, 0, 1);
    } else {
        cam->SetFocalPoint(0, 0, 0);
        cam->SetPosition(1, 0, 0);
        cam->SetViewUp(0, 0, 1);
    }

    //color tables
    m_colorTables["GRAY"] = GRAY;
    m_colorTables["JET"] = JET;
    m_colorTables["TURBO"] = TURBO;
    m_colorTables["PET"] = PET;
    m_colorTables["HSV"] = HSV;
    m_colorTables["SIMPLE"] = SIMPLE;
    m_colorTables["SUMMER"] = SUMMER;

    //window settings
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
    if (m_orientation == Orientation::Axial)
        connect(smoothSlider, &QSlider::valueChanged, [=](int value) { m_imageSmoother->SetStandardDeviations(static_cast<double>(value), static_cast<double>(value), 0.0); });
    else if (m_orientation == Orientation::Coronal)
        connect(smoothSlider, &QSlider::valueChanged, [=](int value) { m_imageSmoother->SetStandardDeviations(0.0, static_cast<double>(value), static_cast<double>(value)); });
    else
        connect(smoothSlider, &QSlider::valueChanged, [=](int value) { m_imageSmoother->SetStandardDeviations(static_cast<double>(value), 0.0, static_cast<double>(value)); });
    auto smoothSliderAction = new QWidgetAction(menuButton);
    auto smoothSliderHolder = new QWidget(menuButton);
    auto smoothSliderLayout = new QHBoxLayout(smoothSliderHolder);
    smoothSliderHolder->setLayout(smoothSliderLayout);
    auto smoothSliderLabel = new QLabel("Smoothing", smoothSliderHolder);
    smoothSliderLayout->addWidget(smoothSliderLabel);
    smoothSliderLayout->addWidget(smoothSlider);
    smoothSliderAction->setDefaultWidget(smoothSliderHolder);
    menu->addAction(smoothSliderAction);

    auto showGrapicsAction = menu->addAction(tr("Show graphics"));
    showGrapicsAction->setCheckable(true);
    showGrapicsAction->setChecked(true);
    connect(showGrapicsAction, &QAction::toggled, this, &SliceRenderWidget::setActorsVisible);

    m_colorTablePicker = new QComboBox(menuButton);
    for (const auto& p : m_colorTables) {
        m_colorTablePicker->addItem(p.first);
    }
    connect(m_colorTablePicker, QOverload<const QString&>::of(&QComboBox::currentTextChanged), this, &SliceRenderWidget::setColorTable);
    auto colorTablePickerAction = new QWidgetAction(menuButton);
    auto colorTablePickerHolder = new QWidget(menuButton);
    auto colorTablePickerLayout = new QHBoxLayout(colorTablePickerHolder);
    colorTablePickerHolder->setLayout(colorTablePickerLayout);
    auto colorTablePickerLabel = new QLabel("Color table", colorTablePickerHolder);
    colorTablePickerLayout->addWidget(colorTablePickerLabel);
    colorTablePickerLayout->addWidget(m_colorTablePicker);
    colorTablePickerAction->setDefaultWidget(colorTablePickerHolder);
    menu->addAction(colorTablePickerAction);
    m_colorTablePicker->setDisabled(true);

    auto windowSettingsAction = new QWidgetAction(menuButton);
    auto windowSettingsHolder = new QWidget(menuButton);
    auto windowSettingsLayout = new QHBoxLayout(windowSettingsHolder);
    windowSettingsHolder->setLayout(windowSettingsLayout);
    auto windowSettingsWCLabel = new QLabel("Min:", windowSettingsHolder);
    windowSettingsLayout->addWidget(windowSettingsWCLabel);
    auto windowSettingMax = new QDoubleSpinBox(menuButton);
    auto windowSettingMin = new QDoubleSpinBox(menuButton);
    auto windowSettingsWWLabel = new QLabel("Max:", windowSettingsHolder);
    windowSettingsLayout->addWidget(windowSettingMin);
    windowSettingsLayout->addWidget(windowSettingsWWLabel);
    windowSettingsLayout->addWidget(windowSettingMax);
    windowSettingsAction->setDefaultWidget(windowSettingsHolder);
    menu->addAction(windowSettingsAction);
    connect(menu, &QMenu::aboutToShow, [=](void) {
        auto prop = m_interactionStyle->GetCurrentImageProperty();
        if (prop) {
            double WW = m_interactionStyle->GetCurrentImageProperty()->GetColorWindow();
            double WL = m_interactionStyle->GetCurrentImageProperty()->GetColorLevel();
            double min = WL - WW * 0.5;
            double max = WL + WW * 0.5;
            if (m_image) {
                windowSettingMax->setRange(std::min(m_image->minMax[0], min), std::max(m_image->minMax[1], max));
                windowSettingMin->setRange(std::min(m_image->minMax[0], min), std::max(m_image->minMax[1], max));
            }
            windowSettingMin->setValue(min);
            windowSettingMax->setValue(max);
        }
    });
    connect(menu, &QMenu::aboutToHide, [=](void) {
        auto prop = m_interactionStyle->GetCurrentImageProperty();
        if (prop) {
            double min = windowSettingMin->value();
            double max = windowSettingMax->value();
            if (min >= max)
                return;
            double WW = (max - min);
            double WL = (min + max) * 0.5;
            prop->SetColorLevel(WL);
            prop->SetColorWindow(WW);
            m_interactionStyle->update();
        }
    });

    menu->addAction(QString(tr("Set background color")), [=]() {
        auto color = QColorDialog::getColor(Qt::black, this);
        if (color.isValid())
            m_renderer->SetBackground(color.redF(), color.greenF(), color.blueF());
        updateRendering();
    });

    menu->addAction(QString(tr("Save to file")), [=]() {
        QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
        auto filename = settings.value("mediaexport/image", "untitled.png").value<QString>();
        filename = QFileDialog::getSaveFileName(this, tr("Save File"), filename, tr("Images (*.png)"));

        if (!filename.isEmpty()) {
            settings.setValue("mediaexport/image", filename);
            auto renderWindow = m_openGLWidget->renderWindow();
            vtkSmartPointer<vtkWindowToImageFilter> windowToImageFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
            windowToImageFilter->SetInput(renderWindow);
            //this is somehow broken
            windowToImageFilter->SetScale(3, 3); //set the resolution of the output image (3 times the current resolution of vtk render window)
            windowToImageFilter->SetFixBoundary(true);
            windowToImageFilter->ShouldRerenderOn();
            windowToImageFilter->SetInputBufferTypeToRGB(); //also record the alpha (transparency) channel
            windowToImageFilter->ReadFrontBufferOn(); // read from the front buffer
            windowToImageFilter->Update();
            vtkSmartPointer<vtkPNGWriter> writer = vtkSmartPointer<vtkPNGWriter>::New();
            writer->SetFileName(filename.toLatin1().data());
            writer->SetInputConnection(windowToImageFilter->GetOutputPort());
            writer->Write();
            this->updateRendering();
        }
    });
#ifdef WINDOWS
    menu->addAction(QString(tr("Save cine")), [=]() {
        this->saveCine();
    });
#endif // WINDOWS

    //setting start image
    auto startImage = makeStartImage();
    QTimer::singleShot(0, [=]() { this->setImageData(startImage); });
}

void SliceRenderWidget::updateRendering()
{
    //might need to call Render
    auto renderWindow = m_openGLWidget->renderWindow();
    auto renderCollection = renderWindow->GetRenderers();
    auto renderer = renderCollection->GetFirstRenderer();
    m_interactionStyle->update();
    renderWindow->Render();
    m_openGLWidget->update();
    return;
}

void SliceRenderWidget::setImageData(std::shared_ptr<ImageContainer> volume, std::shared_ptr<ImageContainer> background)
{
    if (!volume)
        return;
    if (!volume->image)
        return;

    bool imageIDchanged = true;

    if (m_image) {
        if ((m_image->ID == volume->ID) && (m_image->imageType == volume->imageType) && (m_imageBackground == background))
            return;
        if (std::array<double, 2> wl; m_image->image) {
            auto props = m_imageSlice->GetProperty();
            wl[0] = props->GetColorLevel();
            wl[1] = props->GetColorWindow();
            m_windowLevels[m_image->imageType] = wl;
        }
        imageIDchanged = m_image->ID != volume->ID;
    }

    m_image = volume;
    m_imageBackground = background;

    std::string unitText = "";
    if (m_image->dataUnits.size() > 0)
        unitText = "[" + m_image->dataUnits + "]";
    m_textActorCorners->SetText(1, unitText.c_str());
    m_textActorCorners->SetText(0, "");
    m_renderer->RemoveActor(m_imageSliceBackground);
    m_renderer->RemoveActor(m_imageSlice);
    m_renderer->RemoveViewProp(m_scalarColorBar);
    m_renderer->RemoveViewProp(m_textActorCorners);
    m_colorTablePicker->setDisabled(true);

    if (m_windowLevels.find(m_image->imageType) == m_windowLevels.end()) {
        std::array<double, 2> wl = presetLeveling(m_image->imageType);
        if (wl[1] < 0.0) {
            const auto& mm = m_image->minMax;
            wl[0] = (mm[0] + mm[1]) * 0.5;
            wl[1] = (mm[1] - mm[0]) * 0.5;
        }
        m_windowLevels[m_image->imageType] = wl;
    }
    m_imageSmoother->SetInputData(m_image->image);
    m_imageSmoother->Update();

    //update LUT based on image type
    if (auto prop = m_imageSlice->GetProperty(); m_image->imageType == ImageContainer::ImageType::CTImage) {
        prop->BackingOff();
        prop->UseLookupTableScalarRangeOff();
        prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
        prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);

        m_colorTablePicker->setCurrentText("GRAY");
        setColorTable("GRAY");
        m_renderer->AddViewProp(m_textActorCorners);
    } else if (m_image->imageType == ImageContainer::ImageType::DensityImage) {
        prop->BackingOff();
        prop->UseLookupTableScalarRangeOff();
        prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
        prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);
        m_renderer->AddViewProp(m_scalarColorBar);
        m_renderer->AddViewProp(m_textActorCorners);
        m_scalarColorBar->SetNumberOfLabels(2);
        m_colorTablePicker->setCurrentText("TURBO");
        setColorTable("TURBO");
        m_colorTablePicker->setEnabled(true);
    } else if (m_image->imageType == ImageContainer::ImageType::MaterialImage) {
        prop->BackingOff();
        prop->UseLookupTableScalarRangeOn();
        auto lut = vtkSmartPointer<vtkLookupTable>::New();

        int nColors = static_cast<int>(m_image->minMax[1]) + 1;
        lut->SetNumberOfTableValues(nColors);
        for (int i = 0; i < nColors; ++i) {
            auto arr = getColor(i);
            if (i == 0)
                lut->SetTableValue(i, arr[0], arr[1], arr[2], 0.0);
            else
                lut->SetTableValue(i, arr[0], arr[1], arr[2], 1.0);
        }
        lut->SetTableRange(m_image->minMax.data());
        m_renderer->AddViewProp(m_scalarColorBar);
        m_scalarColorBar->SetLookupTable(lut);
        m_scalarColorBar->SetNumberOfLabels(nColors);
        prop->SetLookupTable(lut);
    } else if (m_image->imageType == ImageContainer::ImageType::OrganImage) {
        prop->BackingOff();
        prop->UseLookupTableScalarRangeOn();
        auto lut = vtkSmartPointer<vtkLookupTable>::New();

        int nColors = static_cast<int>(m_image->minMax[1]) + 1;
        lut->SetNumberOfTableValues(nColors);
        for (int i = 0; i < nColors; ++i) {
            auto arr = getColor(i);
            if (i == 0)
                lut->SetTableValue(i, arr[0], arr[1], arr[2], 0.0);
            else
                lut->SetTableValue(i, arr[0], arr[1], arr[2], 1.0);
        }
        lut->SetTableRange(m_image->minMax.data());
        m_renderer->AddViewProp(m_scalarColorBar);
        m_scalarColorBar->SetLookupTable(lut);
        m_scalarColorBar->SetNumberOfLabels(nColors);
        prop->SetLookupTable(lut);
    } else if (m_image->imageType == ImageContainer::ImageType::DoseImage) {
        prop->BackingOff();
        prop->UseLookupTableScalarRangeOff();
        //making sane window level ond center values from image data
        m_windowLevels[m_image->imageType][0] = (m_image->minMax[0] + m_image->minMax[1]) * 0.25;
        m_windowLevels[m_image->imageType][1] = (m_windowLevels[m_image->imageType][0] - m_image->minMax[0]);
        prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
        prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);

        m_colorTablePicker->setCurrentText("TURBO");
        setColorTable("TURBO");
        m_renderer->AddViewProp(m_scalarColorBar);
        m_renderer->AddViewProp(m_textActorCorners);
        m_scalarColorBar->SetNumberOfLabels(2);
        m_colorTablePicker->setEnabled(true);
    } else if (m_image->imageType == ImageContainer::ImageType::TallyImage) {
        prop->BackingOff();
        prop->UseLookupTableScalarRangeOff();
        //making sane window level ond center values from image data
        m_windowLevels[m_image->imageType][0] = (m_image->minMax[0] + m_image->minMax[1]) * 0.5;
        m_windowLevels[m_image->imageType][1] = (m_windowLevels[m_image->imageType][0] - m_image->minMax[0]);
        prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
        prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);

        m_colorTablePicker->setCurrentText("TURBO");
        setColorTable("TURBO");
        m_renderer->AddViewProp(m_scalarColorBar);
        m_renderer->AddViewProp(m_textActorCorners);
        m_scalarColorBar->SetNumberOfLabels(2);
        m_colorTablePicker->setEnabled(true);
    } else if (m_image->imageType == ImageContainer::ImageType::VarianceImage) {
        prop->BackingOff();
        prop->UseLookupTableScalarRangeOff();
        //making sane window level ond center values from image data
        m_windowLevels[m_image->imageType][0] = (m_image->minMax[0] + m_image->minMax[1]) * 0.5;
        m_windowLevels[m_image->imageType][1] = (m_windowLevels[m_image->imageType][0] - m_image->minMax[0]);
        prop->SetColorLevel(m_windowLevels[m_image->imageType][0]);
        prop->SetColorWindow(m_windowLevels[m_image->imageType][1]);

        m_colorTablePicker->setCurrentText("TURBO");
        setColorTable("TURBO");
        m_renderer->AddViewProp(m_scalarColorBar);
        m_renderer->AddViewProp(m_textActorCorners);
        m_scalarColorBar->SetNumberOfLabels(2);
        m_colorTablePicker->setEnabled(true);
    }
    if (m_imageBackground) {
        if (m_imageBackground->image) {
            m_imageMapperBackground->SetInputData(m_imageBackground->image);

            m_renderer->AddActor(m_imageSliceBackground);

            auto prop = m_imageSliceBackground->GetProperty();
            prop->BackingOff();
            prop->UseLookupTableScalarRangeOff();
            auto wl = presetLeveling(m_imageBackground->imageType);
            prop->SetColorLevel(wl[0]);
            prop->SetColorWindow(wl[1]);
            auto lut = vtkSmartPointer<vtkLookupTable>::New();
            lut->SetHueRange(0.0, 0.0);
            lut->SetSaturationRange(0.0, 0.0);
            lut->SetValueRange(0.0, 1.0);
            lut->SetAboveRangeColor(1.0, 1.0, 1.0, 1.0);
            lut->UseAboveRangeColorOn();
            lut->SetBelowRangeColor(0.0, 0.0, 0.0, 0.0);
            lut->UseBelowRangeColorOn();
            lut->Build();
            prop->SetLookupTable(lut);
        }
    }
    m_renderer->AddActor(m_imageSlice);
    if (imageIDchanged)
        m_renderer->ResetCamera();
    m_interactionStyle->update();
    updateRendering();
}

void SliceRenderWidget::addActorContainer(SourceActorContainer* actorContainer)
{
    auto pos = std::find(m_volumeProps.begin(), m_volumeProps.end(), actorContainer);
    if (pos == m_volumeProps.end()) // not present
    {
        m_interactionStyle->addImagePlaneActor(actorContainer);
        m_volumeProps.push_back(actorContainer);
    }
}

void SliceRenderWidget::removeActorContainer(SourceActorContainer* actorContainer)
{
    m_interactionStyle->removeImagePlaneActor(actorContainer);
    auto pos = std::find(m_volumeProps.begin(), m_volumeProps.end(), actorContainer);
    if (pos != m_volumeProps.end())
        m_volumeProps.erase(pos);
}

void SliceRenderWidget::setActorsVisible(int visible)
{
    m_interactionStyle->setImagePlaneActorVisible(visible);
    updateRendering();
}

std::array<double, 2> SliceRenderWidget::presetLeveling(ImageContainer::ImageType type)
{
    std::array<double, 2> wl = { 1.0, -1.0 };
    if (type == ImageContainer::ImageType::CTImage) {
        wl[0] = 10.0;
        wl[1] = 500.0;
    } else if (type == ImageContainer::ImageType::DensityImage) {
        wl[0] = 1.0;
        wl[1] = 0.5;
    } else if (type == ImageContainer::ImageType::DoseImage) {
        wl[0] = 0.1;
        wl[1] = 0.1;
    }
    return wl;
}

void SliceRenderWidget::setColorTable(const QString& colorTableName)
{
    auto lut = vtkSmartPointer<vtkLookupTable>::New();
    const auto arr = generateStandardColorTable(m_colorTables.at(colorTableName));
    lut->Allocate();
    for (std::size_t i = 1; i < 256; ++i) {
        std::size_t ai = i * 3;
        lut->SetTableValue(i, arr[ai], arr[ai + 1], arr[ai + 2]);
    }
    lut->SetTableValue(0, 0.0, 0.0, 0.0, 0.0); // buttom should be zero for background
    auto prop = m_imageSlice->GetProperty();
    prop->SetLookupTable(lut);
    m_scalarColorBar->SetLookupTable(lut);
}
#ifdef WINDOWS
void SliceRenderWidget::saveCine()
{

    if (!(m_imageMapper && m_image))
        return;

    auto windowFilter = vtkSmartPointer<vtkWindowToImageFilter>::New();
    auto renderWindow = m_renderer->GetRenderWindow();
    windowFilter->SetInput(renderWindow);
    windowFilter->SetInputBufferTypeToRGB();
    windowFilter->ReadFrontBufferOff();
    windowFilter->Update();
    auto writer = vtkSmartPointer<vtkAVIWriter>::New();
    writer->SetInputConnection(windowFilter->GetOutputPort());

    QSettings settings(QSettings::NativeFormat, QSettings::UserScope, "OpenDXMC", "app");
    auto filename = settings.value("mediaexport/video", "untitled.avi").value<QString>();
    filename = QFileDialog::getSaveFileName(this, tr("Save File"), filename, tr("Movies (*.avi)"));
    if (!filename.isEmpty()) {
        settings.setValue("mediaexport/video", filename);
        writer->SetFileName(filename.toLatin1().data());
        m_imageMapper->UpdateInformation();
        auto plane = m_imageMapper->GetSlicePlane();
        auto normal = plane->GetNormal();
        auto stepAxis = vectormath::argmax3<int, double>(normal);

        double* imbounds = m_imageMapper->GetBounds();
        double* origin = plane->GetOrigin();
        double originHold[3];
        for (int i = 0; i < 3; ++i)
            originHold[i] = origin[i];
        origin[stepAxis] = imbounds[stepAxis * 2];
        plane->SetOrigin(origin);
        m_imageMapper->SetSlicePlane(plane);
        if (m_imageMapperBackground)
            m_imageMapperBackground->SetSlicePlane(plane);

        auto dimensions = m_image->image->GetDimensions();
        const int nFrames = dimensions[stepAxis] * 2;
        const int nSec = 20;
        int frameRate = std::max(nFrames / nSec, 2);
        writer->SetRate(frameRate);

        QProgressDialog progress("Generating movie", "Cancel", 0, nFrames, this);
        progress.setWindowModality(Qt::WindowModal);

        writer->Start();
        windowFilter->Modified();
        writer->Write();

        const double step = (imbounds[stepAxis * 2 + 1] - imbounds[stepAxis * 2]) / static_cast<double>(nFrames);
        int currentFrame = 0;
        while (origin[stepAxis] <= imbounds[stepAxis * 2 + 1]) {
            origin[stepAxis] += step;
            currentFrame += 1;
            plane->SetOrigin(origin);
            m_imageMapper->SetSlicePlane(plane);
            if (m_imageMapperBackground)
                m_imageMapperBackground->SetSlicePlane(plane);
            windowFilter->Modified();
            writer->Write();
            progress.setValue(currentFrame);
            if (progress.wasCanceled()) {
                writer->End();
                return;
            }
        }
        writer->End();
        plane->SetOrigin(originHold);
        m_imageMapper->SetSlicePlane(plane);
        m_imageMapper->UpdateInformation();
        renderWindow->Render();
    }
}
#endif // WINDOWS
