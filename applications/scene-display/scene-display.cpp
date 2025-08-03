
#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include "scene-display-widget.h"
#include "applications/qt/scene-widget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

#include <QtWidgets/QFileDialog>
#include <QtCore/QString>

#include <regen/utility/filesystem.h>
#include "regen/text/font.h"

#define CONFIG_FILE_NAME ".regen-scene-display.cfg"

using namespace regen;

int main(int argc, char **argv) {
#ifdef Q_WS_X11
#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
	XInitThreads();
#endif
#endif
	// create and show application window
	ref_ptr<QtApplication> app = ref_ptr<QtApplication>::alloc(
		argc, (const char **) argv,
		SceneWidget::defaultFormat());
	app->setupLogging();

	// add a custom path for shader loading
	boost::filesystem::path shaderPath(REGEN_SOURCE_DIR);
	shaderPath /= "applications";
	shaderPath /= "scene-display";
	shaderPath /= "shader";
	app->addShaderPath(shaderPath.string());

	auto *widget = new SceneDisplayWidget(app.get());
	app->show();
	widget->show();
	widget->init();

	widget->setWindowTitle("Scene Viewer");

	auto exitCode = app->mainLoop();
	REGEN_INFO("Shutting down application");
	//delete widget;
	Logging::shutdown();
	_Exit(exitCode);
	return exitCode;
}
