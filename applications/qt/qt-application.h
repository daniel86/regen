#ifndef QT_APPLICATION_H_
#define QT_APPLICATION_H_

#include <GL/glew.h>

#include <QtOpenGL/QGLWidget>
#include <QtWidgets/QApplication>
#include <regen/scene/scene.h>

#include <string>

namespace regen {
	class QtApplication : public Scene {
	public:
		QtApplication(
				const int &argc, const char **argv,
				const QSurfaceFormat &glFormat,
				uint32_t width = 800,
				uint32_t height = 600,
				QWidget *parent = nullptr);

		/**
		 * @return topmost parent of GL widget.
		 */
		QWidget *toplevelWidget();

		/**
		 * @return the rendering widget.
		 */
		QWidget *glWidget() { return glWidget_; }

		QWidget *glWidgetContainer()  { return glContainer_; }

		void toggleFullscreen();

		void show();

		int mainLoop();

		void exitMainLoop(int errorCode);

	protected:
		QApplication *app_;
		QWidget *glContainer_;
		QWidget *glWidget_;
		bool isMainloopRunning_;
		int32_t exitCode_;
	};
}

#endif /* QT_APPLICATION_H_ */
