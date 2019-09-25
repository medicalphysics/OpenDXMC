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
#include <QSurfaceFormat>
#include <QDebug>

#include <QVTKOpenGLWidget.h>
#include <vtkOpenGLRenderWindow.h>

#include "mainwindow.h"

#include <thread>


int main (int argc, char *argv[])
{
	
	//spdlog::info("Sample Info output.");
	//spdlog::warn("Sample Warn output.");
	//spdlog::error("Sample Error output.");

	// needed to ensure appropriate OpenGL context is created for VTK rendering.
	vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(0);
	QSurfaceFormat::setDefaultFormat(QVTKOpenGLWidget::defaultFormat());

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
	QApplication app(argc, argv);
	QCoreApplication::setApplicationName("OpenDXMC");
	QCoreApplication::setOrganizationName("SSHF");
	app.setWindowIcon(QIcon("resources/icons/icon.png"));

	MainWindow win;
	QString title = "OpenDXMC v" + QString(APP_VERSION);
	win.setWindowTitle(title);
	win.show();
	
    return app.exec();
}
