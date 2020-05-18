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

#include "volumeactorcontainer.h"

#include <vtkInteractorStyleImage.h>
#include <vtkImageResliceMapper.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkCornerAnnotation.h>

subclass jystick actor interactor????

class customMouseInteractorStyle : public vtkInteractorStyleImage
{
public:
	static customMouseInteractorStyle* New();
	vtkTypeMacro(customMouseInteractorStyle, vtkInteractorStyleImage);
	void OnMouseWheelForward() override;
	void OnMouseWheelBackward() override;
	void OnMouseMove() override;
	void OnLeftButtonDown() override;
	void OnLeftButtonUp() override;
	void setMapper(vtkSmartPointer<vtkImageResliceMapper> m);
	void setMapperBackground(vtkSmartPointer<vtkImageResliceMapper> m);
	void setRenderWindow(vtkSmartPointer<vtkRenderWindow> m);
	void setCornerAnnotation(vtkSmartPointer<vtkCornerAnnotation> actor);
	static std::string prettyNumber(double number);
	void update();
	void addImagePlaneActor(VolumeActorContainer* container);
	void removeImagePlaneActor(VolumeActorContainer* container);
protected:
	void updateWLText();
	void scrollSlice(bool forward);
	void updatePlaneActors();
	vtkActor* findPickedPlaneActor(int x, int y);
private:
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapper;
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapperBackground;
	vtkSmartPointer<vtkRenderWindow> m_renderWindow;
	vtkSmartPointer<vtkCornerAnnotation> m_textActorCorners;
	std::vector<VolumeActorContainer*> m_imagePlaneActors;
	vtkActor* m_pickedPlaneActor = nullptr;
};


