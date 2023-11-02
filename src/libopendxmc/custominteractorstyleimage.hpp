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

#pragma once

#include <vtkInteractorStyleImage.h>

class CustomInteractorStyleImage : public vtkInteractorStyleImage {
public:
    static CustomInteractorStyleImage* New();
    vtkTypeMacro(CustomInteractorStyleImage, vtkInteractorStyleImage);
    void EndWindowLevel() override;
    void StartSlice() override;
    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnChar() override;
    void WindowLevel() override;

protected:
    CustomInteractorStyleImage();
    ~CustomInteractorStyleImage();

private:
    CustomInteractorStyleImage(const CustomInteractorStyleImage&) = delete;
    void operator=(const CustomInteractorStyleImage&) = delete;
};
