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
#include <vtkCellPicker.h>

#include <functional>

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
	void StartPan() override;
	void EndPan() override;
	void Pan() override;
	void setCallback(std::function<void(void)> f) { m_callback = f; }
	void setMapper(vtkSmartPointer<vtkImageResliceMapper> m);
	void setMapperBackground(vtkSmartPointer<vtkImageResliceMapper> m);
	void setRenderWindow(vtkSmartPointer<vtkRenderWindow> m);
	void setCornerAnnotation(vtkSmartPointer<vtkCornerAnnotation> actor);
	static std::string prettyNumber(double number);
	void update();
	void addImagePlaneActor(SourceActorContainer* container);
	void removeImagePlaneActor(SourceActorContainer* container);
protected:
	customMouseInteractorStyle();
	void updateWLText();
	void scrollSlice(bool forward);
	void updatePlaneActors();
	SourceActorContainer* findPickedPlaneActor(int x, int y);
private:
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapper = nullptr;
	vtkSmartPointer<vtkImageResliceMapper> m_imageMapperBackground = nullptr;
	vtkSmartPointer<vtkRenderWindow> m_renderWindow = nullptr;
	vtkSmartPointer<vtkCornerAnnotation> m_textActorCorners = nullptr;
	std::vector<SourceActorContainer*> m_imagePlaneActors;
	SourceActorContainer* m_pickedPlaneActor = nullptr;
	vtkSmartPointer<vtkCellPicker> m_interactionPicker = nullptr;
	std::function<void(void)> m_callback;
};


