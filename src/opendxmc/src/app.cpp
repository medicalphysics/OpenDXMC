#include <QApplication>
#include <QSurfaceFormat>

#include <QVTKOpenGLWidget.h>
#include <vtkOpenGLRenderWindow.h>

#include "mainwindow.h"

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
	win.show();

    return app.exec();
}
