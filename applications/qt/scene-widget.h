#ifndef REGEN_SCENE_WIDGET_H_
#define REGEN_SCENE_WIDGET_H_

#include <GL/glew.h>
// NOTE: QT does not like using glew :/ it will print warnings that can be ignored.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <QOpenGLWindow>
#pragma GCC diagnostic pop

#include <QtWidgets/QWidget>
#include <QtCore/QThread>
#include "qt-application.h"

namespace regen {
	/**
	 * QT Widget that can be used for OpenGL rendering.
	 */
	class SceneWidget : public QWidget {
	public:
		static QSurfaceFormat defaultFormat();

		explicit SceneWidget(QtApplication *app,
					const QSurfaceFormat &glFormat = defaultFormat(),
					QWidget *parent = nullptr);

		~SceneWidget() override;

		SceneWidget(const SceneWidget&) = delete;

		void startRendering();

		void stopRendering();

		void run(QOpenGLContext *glContext);

		auto surfaceFormat() const -> QSurfaceFormat const & { return surfaceFormat_; }

		/**
		 * @param interval update interval in milliseconds.
		 */
		void setUpdateInterval(int interval);

		// override
		void mousePressEvent(QMouseEvent *) override;

		// override
		void mouseDoubleClickEvent(QMouseEvent *) override;

		// override
		void mouseReleaseEvent(QMouseEvent *) override;

		// override
		void enterEvent(QEvent *) override;

		// override
		void leaveEvent(QEvent *) override;

		// override
		void wheelEvent(QWheelEvent *) override;

		// override
		void mouseMoveEvent(QMouseEvent *event) override;

		// override
		void keyPressEvent(QKeyEvent *event) override;

		// override
		void keyReleaseEvent(QKeyEvent *event) override;

		// override
		void resizeEvent(QResizeEvent *) override;

		// override
		void paintEvent(QPaintEvent *) override {};

		// override
		bool eventFilter(QObject *obj, QEvent *event) override;

	protected:
		class GLThread : public QThread {
		public:
			explicit GLThread(SceneWidget *glWidget);

			void run() override;

		private:
			SceneWidget *glWidget_;
		};

		QtApplication *app_;
		int updateInterval_;
		bool isRunning_;
		QSurfaceFormat surfaceFormat_;
		QWidget *winContainer_;
		ref_ptr<QOpenGLWindow> sceneWindow_;
		ref_ptr<GLThread> renderThread_;

		void do_mouseClick(QMouseEvent *event, bool isPressed, bool isDoubleClick);
	};

}

#endif /* REGEN_SCENE_WIDGET_H_ */
