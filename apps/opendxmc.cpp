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

#include <QApplication>
#include <QSplashScreen>
#include <QSurfaceFormat>

#include <QVTKOpenGLWidget.h>
#include <vtkOpenGLRenderWindow.h>

#include "opendxmc/mainwindow.h"

#include <thread>

int main(int argc, char* argv[])
{
    // needed to ensure appropriate OpenGL context is created for VTK rendering.
    vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);
    auto format = QVTKOpenGLWidget::defaultFormat();
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);

    QPixmap splashMap("resources/icons/icon_fill.png");
    QSplashScreen splash(splashMap);
    splash.show();
    app.processEvents();
    splash.showMessage("Starting OpenDXMC", Qt::AlignCenter, Qt::white);
    app.processEvents();

    QCoreApplication::setApplicationName("OpenDXMC");
    QCoreApplication::setOrganizationName("SSHF");
    app.setWindowIcon(QIcon("resources/icons/icon.png"));

    MainWindow win;
    QString title = "OpenDXMC v" + QString(APP_VERSION);
    win.setWindowTitle(title);
    win.show();

    splash.finish(&win);
    return app.exec();
}
