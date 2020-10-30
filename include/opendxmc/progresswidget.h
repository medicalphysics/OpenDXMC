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

#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QPushButton>
#include <QWidget>

#include "opendxmc/dxmc_specialization.h"

#include <array>
#include <memory>

class ProgressWidget : public QWidget {
public:
    ProgressWidget(QWidget* parent = nullptr);
    void setImageData(std::shared_ptr<DoseProgressImageData> data);
    void setShowProgress(bool show);
    bool showProgress() const { return m_showProgress; };
    void setCancelRun(bool cancel) { m_cancelProgress = cancel; };
    bool cancelRun() { return m_cancelProgress; };

protected:
    void resizeEvent(QResizeEvent* event);
    void showEvent(QShowEvent* event);

private:
    QPushButton* m_cancelButton = nullptr;
    QGraphicsView* m_view = nullptr;
    QGraphicsPixmapItem* m_pixItem = nullptr;
    QVector<QRgb> m_colormap;
    bool m_showProgress = true;
    bool m_cancelProgress = false;
};