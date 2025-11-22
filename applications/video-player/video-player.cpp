#include <applications/qt/qt-application.h>
#include <regen/config.h>

#include "video-player-widget.h"
#include "applications/qt/scene-widget.h"

#ifdef Q_WS_X11
#include <X11/Xlib.h>
#endif

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
	app->toplevelWidget()->setWindowTitle("OpenGL player");

	// create the main widget and connect it to applications key events
	ref_ptr<VideoPlayerWidget> widget = ref_ptr<VideoPlayerWidget>::alloc(app.get());
	widget->show();
	app->show();

	// configure OpenAL for the video player
	AudioListener::set3f(AL_POSITION, Vec3f::zero());
	AudioListener::set3f(AL_VELOCITY, Vec3f::zero());
	AudioListener::set6f(AL_ORIENTATION, Vec6f::create(Vec3f{0.0f, 0.0f, 1.0f}, Vec3f::up()));

	int exitCode = app->mainLoop();

	return exitCode;
}
