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

#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include <thread>

void createLogger()
{
	auto logger = spdlog::rotating_logger_mt("OpenDXMCapp", "log.txt", 1048576 * 5, 1);
	spdlog::set_level(spdlog::level::trace);
	logger->info("Application started");
	logger->info("Number of hardware threads available: {}.", std::thread::hardware_concurrency());
	logger->flush();
}


int main (int argc, char *argv[])
{
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
	createLogger();

    return app.exec();
}
