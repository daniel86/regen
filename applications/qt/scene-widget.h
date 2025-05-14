#ifndef REGEN_SCENE_WIDGET_H_
#define REGEN_SCENE_WIDGET_H_

#include <GL/glew.h>

#include <QtOpenGL/QGLWidget>
#include <QtCore/QThread>
#include "qt-application.h"

namespace regen {
	/**
	 * QT Widget that can be used for OpenGL rendering.
	 */
	class SceneWidget : public QGLWidget {
	public:
		SceneWidget(QtApplication *app,
					const QGLFormat &glFormat,
					QWidget *parent = nullptr);

		void startRendering();

		void stopRendering();

		void run();

		auto surfaceFormat() const -> QSurfaceFormat const & { return surfaceFormat_; }

		/**
		 * @param interval update interval in milliseconds.
		 */
		void setUpdateInterval(GLint interval);

	protected:
		class GLThread : public QThread {
		public:
			explicit GLThread(SceneWidget *glWidget);

			void run() override;

		private:
			SceneWidget *glWidget_;
		};

		QtApplication *app_;
		GLint updateInterval_;
		GLboolean isRunning_;
		QSurfaceFormat surfaceFormat_;
		GLThread renderThread_;

		void initializeGL() override;

		void paintGL() override {};

		void updateGL() override {};

		void resizeGL(int width, int height) override;

		void resizeEvent(QResizeEvent *) override;

		void paintEvent(QPaintEvent *) override {};

		void mousePressEvent(QMouseEvent *) override;

		void mouseDoubleClickEvent(QMouseEvent *) override;

		void mouseReleaseEvent(QMouseEvent *) override;

		void enterEvent(QEvent *) override;

		void leaveEvent(QEvent *) override;

		void wheelEvent(QWheelEvent *) override;

		void mouseMoveEvent(QMouseEvent *event) override;

		void keyPressEvent(QKeyEvent *event) override;

		void keyReleaseEvent(QKeyEvent *event) override;

		bool eventFilter(QObject *obj, QEvent *event) override;

		void mouseClick__(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick);
	};

}

#endif /* REGEN_SCENE_WIDGET_H_ */
