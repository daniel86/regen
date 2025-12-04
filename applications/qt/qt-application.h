#ifndef REGEN_QT_APPLICATION_H_
#define REGEN_QT_APPLICATION_H_

#include <GL/glew.h>

#include <QtOpenGL/QGLWidget>
#include <QtWidgets/QApplication>
#include <regen/scene/scene.h>

namespace regen {
	/**
	 * QT based application class that manages the main loop and window.
	 */
	class QtApplication final : public Scene {
	public:
		/**
		 * @param argc argument count.
		 * @param argv argument values.
		 * @param glFormat OpenGL surface format.
		 * @param width initial window width.
		 * @param height initial window height.
		 */
		QtApplication(
				const int &argc, const char **argv,
				const QSurfaceFormat &glFormat,
				uint32_t width = 800,
				uint32_t height = 600);

		/**
		 * @return topmost parent of GL widget.
		 */
		QWidget *toplevelWidget();

		/**
		 * @return the rendering widget.
		 */
		QWidget* glWidget() { return glWidget_; }

		/**
		 * @return the container widget for the GL widget.
		 */
		QWidget* glContainer()  { return glContainer_; }

		/**
		 * Toggle fullscreen mode for the toplevel widget.
		 */
		void toggleFullscreen();

		/**
		 * Show the application window.
		 */
		void show();

		/**
		 * Start the main GUI loop.
		 * This will first ensure that the window is shown
		 * and then start the rendering loop, and process QT events
		 * in a loop.
		 * @return exit code.
		 */
		int mainLoopGUI();

		/**
		 * Exit the main loop with the given error code.
		 * @param errorCode exit code.
		 */
		void exitMainLoop(int errorCode);

	protected:
		QApplication *app_;
		QWidget *glContainer_;
		QWidget *glWidget_;
		int32_t exitCode_;
		// atomic flag indicating if main loop is running
		std::atomic<bool> isMainloopRunning_ = { false };
	};
}

#endif /* REGEN_QT_APPLICATION_H_ */
