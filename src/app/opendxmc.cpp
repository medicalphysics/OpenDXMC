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

#include <mainwindow.hpp>

#include <QApplication>
#include <QDir>
#include <QSplashScreen>
#include <QStyle>
#include <QSurfaceFormat>
#include <QVTKOpenGLNativeWidget.h>

// We need this include for some reason to avoid openGL errors
#include <vtkOpenGLRenderWindow.h>

#include <thread>

int main(int argc, char* argv[])
{

    // before initializing QApplication, set the default surface format.
    QSurfaceFormat::setDefaultFormat(QVTKOpenGLNativeWidget::defaultFormat());

    QApplication app(argc, argv);

    // setting current dir to exe dir
    const auto exeDirPath = QCoreApplication::applicationDirPath();
    QDir::setCurrent(exeDirPath);

    QPixmap splashMap(":icons/icon_fill.png");
    QSplashScreen splash(splashMap);
    splash.show();
    app.setStyle("fusion");
    app.processEvents();
    splash.showMessage("Starting OpenDXMC", Qt::AlignCenter, Qt::white);
    app.processEvents();

    app.setApplicationName("OpenDXMC");
    app.setOrganizationName("SSHF");
    app.setWindowIcon(QIcon(":icons/icon.png"));

    MainWindow win;
    QString title = "OpenDXMC v" + QString(APP_VERSION);
    win.setWindowTitle(title);
    win.show();

    splash.finish(&win);
    return app.exec();
}
