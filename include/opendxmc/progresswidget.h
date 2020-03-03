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
#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <dxmc/progressbar.h>
#include <array>
#include <memory>

class ProgressWidget :public QWidget
{
public:
	ProgressWidget(QWidget* parent = nullptr);
	void setImageData(std::shared_ptr<DoseProgressImageData> data);

protected:
private:
	QGraphicsView* m_view = nullptr;
	QGraphicsPixmapItem* m_pixItem = nullptr;
	QVector<QRgb> m_colormap;
};