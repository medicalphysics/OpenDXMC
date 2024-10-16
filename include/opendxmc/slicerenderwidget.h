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

#pragma once

#include <QComboBox>
#include <QWidget>

#include <QVTKOpenGLNativeWidget.h>
#include <vtkImageData.h>
#include <vtkImageGaussianSmooth.h>
#include <vtkImageResliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkRenderer.h>
#include <vtkScalarBarActor.h>
#include <vtkSmartPointer.h>

#include <array>
#include <map>
#include <memory>

#include "opendxmc/imagecontainer.h"
#include "opendxmc/slicerenderinteractor.h"
#include "opendxmc/volumeactorcontainer.h"


// https://github.com/Kitware/VTK/blob/master/Rendering/Image/Testing/Cxx/TestImageResliceMapperAlpha.cxx

class SliceRenderWidget : public QWidget {
    Q_OBJECT
public:
    enum class Orientation { Axial,
        Sagittal,
        Coronal };
    SliceRenderWidget(QWidget* parent = nullptr, Orientation orientation = Orientation::Axial);

    void updateRendering();
    void setImageData(std::shared_ptr<ImageContainer> foreground, std::shared_ptr<ImageContainer> background = nullptr);
    void addActorContainer(SourceActorContainer* actorContainer);
    void removeActorContainer(SourceActorContainer* actorContainer);
    void setActorsVisible(int visible);
signals:
    void sourceActorChanged();

protected:
    std::array<double, 2> presetLeveling(ImageContainer::ImageType type);
    void setColorTable(const QString& colorTableName);
#ifdef WIN32
    void saveCine();
#endif

private:
    Orientation m_orientation = Orientation::Axial;
    QVTKOpenGLNativeWidget* m_openGLWidget = nullptr;
    vtkSmartPointer<vtkImageResliceMapper> m_imageMapper = nullptr;
    vtkSmartPointer<vtkImageGaussianSmooth> m_imageSmoother = nullptr;
    vtkSmartPointer<vtkImageResliceMapper> m_imageMapperBackground = nullptr;
    vtkSmartPointer<vtkImageSlice> m_imageSlice = nullptr;
    vtkSmartPointer<vtkImageSlice> m_imageSliceBackground = nullptr;
    vtkSmartPointer<customMouseInteractorStyle> m_interactionStyle = nullptr;
    std::map<ImageContainer::ImageType, std::array<double, 2>> m_windowLevels;
    vtkSmartPointer<vtkRenderer> m_renderer = nullptr;
    vtkSmartPointer<vtkCornerAnnotation> m_textActorCorners = nullptr;
    vtkSmartPointer<vtkScalarBarActor> m_scalarColorBar = nullptr;
    std::map<const QString, QVector<double>> m_colorTables;
    QComboBox* m_colorTablePicker = nullptr;
    std::shared_ptr<ImageContainer> m_image = nullptr;
    std::shared_ptr<ImageContainer> m_imageBackground = nullptr;
    std::vector<SourceActorContainer*> m_volumeProps;
};
