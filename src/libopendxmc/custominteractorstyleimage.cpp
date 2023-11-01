#include "custominteractorstyleimage.hpp"
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

Copyright 2024 Erlend Andersen
*/

#include <custominteractorstyleimage.hpp>

#include <vtkCallbackCommand.h>
#include <vtkImageProperty.h>
#include <vtkInteractorObserver.h>
#include <vtkObjectFactory.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

vtkStandardNewMacro(CustomInteractorStyleImage);

CustomInteractorStyleImage::CustomInteractorStyleImage()
{
}

CustomInteractorStyleImage::~CustomInteractorStyleImage() { }

void CustomInteractorStyleImage::EndWindowLevel()
{
    vtkInteractorStyleImage::EndWindowLevel();
}

void CustomInteractorStyleImage::StartSlice()
{
    vtkInteractorStyleImage::StartSlice();
}

void CustomInteractorStyleImage::OnMouseMove()
{
    vtkInteractorStyleImage::OnMouseMove();
}
void CustomInteractorStyleImage::OnLeftButtonDown()
{
    // vtkInteractorStyleImage::OnLeftButtonDown();
    // edit

    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];

    this->FindPokedRenderer(x, y);
    if (this->CurrentRenderer == nullptr) {
        return;
    }

    // Redefine this button to handle window/level
    this->GrabFocus(this->EventCallbackCommand);
    if (!this->Interactor->GetShiftKey() && !this->Interactor->GetControlKey()) {
        this->StartSlice();
    }

    else if (this->InteractionMode == VTKIS_IMAGE_SLICING && this->Interactor->GetShiftKey()) {
        this->StartDolly();
    }

    // If shift is held down, do a rotation
    else if (this->InteractionMode == VTKIS_IMAGE3D && this->Interactor->GetShiftKey()) {

        this->StartRotate();
    }

    // If ctrl is held down in slicing mode, slice the image
    else if (this->InteractionMode == VTKIS_IMAGE_SLICING && this->Interactor->GetControlKey()) {

        this->WindowLevelStartPosition[0] = x;
        this->WindowLevelStartPosition[1] = y;
        this->StartWindowLevel();
    }

    // The rest of the button + key combinations remain the same

    else {
        this->Superclass::OnLeftButtonDown();
    }
}

void CustomInteractorStyleImage::OnLeftButtonUp()
{
    // vtkInteractorStyleImage::OnLeftButtonUp();
    switch (this->State) {
    case VTKIS_WINDOW_LEVEL:
        this->EndWindowLevel();
        if (this->Interactor) {
            this->ReleaseFocus();
        }
        break;

    case VTKIS_SLICE:
        this->EndSlice();
        if (this->Interactor) {
            this->ReleaseFocus();
        }
        break;
    }

    // Call parent to handle all other states and perform additional work
    this->Superclass::OnLeftButtonUp();
}

void CustomInteractorStyleImage::OnMiddleButtonDown()
{
    // vtkInteractorStyleImage::OnMiddleButtonDown();
    this->FindPokedRenderer(this->Interactor->GetEventPosition()[0], this->Interactor->GetEventPosition()[1]);
    if (this->CurrentRenderer == nullptr) {
        return;
    }

    if (this->InteractionMode == VTKIS_IMAGE_SLICING) {
        this->StartPick();
    }

    // If shift is held down, change the slice
    else if ((this->InteractionMode == VTKIS_IMAGE3D || this->InteractionMode == VTKIS_IMAGE_SLICING) && this->Interactor->GetShiftKey()) {
        this->StartSlice();
    }

    // The rest of the button + key combinations remain the same
    else {
        this->Superclass::OnMiddleButtonDown();
    }
}

void CustomInteractorStyleImage::OnMiddleButtonUp()
{
    // vtkInteractorStyleImage::OnMiddleButtonUp();
    switch (this->State) {
    case VTKIS_PICK:
        this->EndPick();
        if (this->Interactor) {
            this->ReleaseFocus();
        }
        break;

    case VTKIS_SLICE:
        this->EndSlice();
        if (this->Interactor) {
            this->ReleaseFocus();
        }
        break;
    }

    // Call parent to handle all other states and perform additional work

    this->Superclass::OnMiddleButtonUp();
}

void CustomInteractorStyleImage::OnRightButtonDown()
{
    // vtkInteractorStyleImage::OnRightButtonDown();
    int x = this->Interactor->GetEventPosition()[0];
    int y = this->Interactor->GetEventPosition()[1];

    this->FindPokedRenderer(x, y);
    if (this->CurrentRenderer == nullptr) {
        return;
    }

    // Redefine this button + shift to handle pick
    this->GrabFocus(this->EventCallbackCommand);

    if (!this->Interactor->GetControlKey() && !this->Interactor->GetShiftKey()) {
        this->WindowLevelStartPosition[0] = x;
        this->WindowLevelStartPosition[1] = y;
        this->StartWindowLevel();
    }

    else if (this->Interactor->GetShiftKey()) {
        this->StartPan();
    }

    else if (this->InteractionMode == VTKIS_IMAGE3D && this->Interactor->GetControlKey()) {
        this->StartSlice();
    } else if (this->InteractionMode == VTKIS_IMAGE_SLICING && this->Interactor->GetControlKey()) {
        this->StartSpin();
    }

    // The rest of the button + key combinations remain the same
    else {
        this->Superclass::OnRightButtonDown();
    }
}

void CustomInteractorStyleImage::OnRightButtonUp()
{
    // vtkInteractorStyleImage::OnRightButtonUp();
    switch (this->State) {
    case VTKIS_WINDOW_LEVEL:
        this->EndWindowLevel();
        if (this->Interactor) {
            this->ReleaseFocus();
        }
        break;

    case VTKIS_PICK:
        this->EndPick();
        if (this->Interactor) {
            this->ReleaseFocus();
        }
        break;

    case VTKIS_SLICE:
        this->EndSlice();
        if (this->Interactor) {
            this->ReleaseFocus();
        }
        break;

    case VTKIS_SPIN:
        if (this->Interactor) {
            this->EndSpin();
        }
        break;

    case VTKIS_PAN:
        if (this->Interactor) {
            this->EndPan();
        }
        break;
    }

    // Call parent to handle all other states and perform additional work
    this->Superclass::OnRightButtonUp();
}

void CustomInteractorStyleImage::OnChar()
{
    vtkInteractorStyleImage::OnChar();
}