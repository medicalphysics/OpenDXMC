#pragma once


#include "imagecontainer.h"

#include <QWidget>
#include <QMap>

#include <QVTKOpenGLWidget.h>
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkImageSliceMapper.h>
#include <vtkImageSlice.h>
#include <vtkRenderer.h>
#include <vtkScalarsToColors.h>
#include <vtkDiscretizableColorTransferFunction.h>
#include <vtkLookupTable.h>
#include <vtkImageProperty.h>
#include <vtkTextActor.h>

#include <memory>

class SliceRenderWidget : public QWidget
{
	Q_OBJECT
public:
	enum Orientation { Axial, Sagittal, Coronal };
	SliceRenderWidget(QWidget *parent = nullptr, Orientation orientation = Axial);

	void updateRendering();
	void setImageData(std::shared_ptr<ImageContainer> volume);

private:
	Orientation m_orientation;
	QVTKOpenGLWidget *m_openGLWidget;
	vtkSmartPointer<vtkImageSliceMapper> m_imageSliceMapper;
	vtkSmartPointer<vtkLookupTable> m_lut;
	vtkSmartPointer<vtkDiscretizableColorTransferFunction> m_dctf;
	vtkSmartPointer<vtkImageProperty> m_imageProperty;
	vtkSmartPointer<vtkRenderer> m_renderer;
	vtkSmartPointer<vtkTextActor> m_textActor;
	std::shared_ptr<ImageContainer> m_image;

	QMap<int, std::pair<double, double>> m_windowLevelMaps;
	//vtkSmartPointer<vtkWindowLevelLookupTable> m_lut;
}; 


/*class SliceRenderWidget : public QWidget
{
	Q_OBJECT
public:
	enum Orientation { Axial, Sagittal, Coronal };
	SliceRenderWidget(QWidget *parent = nullptr, Orientation orientation = Axial);

	void updateRendering();
	void setImageData(ImageContainer volume);

private:
	Orientation m_orientation;
	QVTKOpenGLWidget *m_openGLWidget;
	vtkSmartPointer<vtkResliceImageViewer> m_resliceImageViewer;
	ImageContainer m_image;
	vtkSmartPointer<vtkWindowLevelLookupTable> m_lut;
};*/