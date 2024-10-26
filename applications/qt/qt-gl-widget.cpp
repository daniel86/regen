/*
 * qt-gl-widget.cpp
 *
 *  Created on: 31.12.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>
#include <QtGui/QCloseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

#include <regen/utility/threading.h>
#include "qt-gl-widget.h"
#include "qt-application.h"

using namespace regen;

#define WAIT_ON_VSYNC

static GLint qtToOgleButton(Qt::MouseButton button) {
	switch (button) {
		case Qt::LeftButton:
			return Application::MOUSE_BUTTON_LEFT;
		case Qt::RightButton:
			return Application::MOUSE_BUTTON_RIGHT;
		case Qt::MiddleButton:
			return Application::MOUSE_BUTTON_MIDDLE;
		case Qt::XButton1:
		case Qt::XButton2:
		case Qt::NoButton:
		case Qt::MouseButtonMask:
		default:
			return -1;
	}
}

static QSurfaceFormat convertFormat(const QGLFormat &glFormat) {
	QSurfaceFormat surfaceFormat;

	surfaceFormat.setDepthBufferSize(glFormat.depthBufferSize());
	surfaceFormat.setStencilBufferSize(glFormat.stencilBufferSize());
	surfaceFormat.setRedBufferSize(glFormat.redBufferSize());
	surfaceFormat.setGreenBufferSize(glFormat.greenBufferSize());
	surfaceFormat.setBlueBufferSize(glFormat.blueBufferSize());
	surfaceFormat.setAlphaBufferSize(glFormat.alphaBufferSize());
	surfaceFormat.setSamples(glFormat.samples());
	surfaceFormat.setSwapBehavior(
			glFormat.doubleBuffer() ? QSurfaceFormat::DoubleBuffer : QSurfaceFormat::SingleBuffer);
	surfaceFormat.setStereo(glFormat.stereo());
	surfaceFormat.setVersion(glFormat.majorVersion(), glFormat.minorVersion());
	surfaceFormat.setProfile(static_cast<QSurfaceFormat::OpenGLContextProfile>(glFormat.profile()));
	surfaceFormat.setSwapInterval(glFormat.swapInterval());

	return surfaceFormat;
}

QTGLWidget::QTGLWidget(
		QtApplication *app,
		const QGLFormat &glFormat,
		QWidget *parent)
		: QGLWidget(glFormat, parent),
		  app_(app),
		  renderThread_(this),
		  updateInterval_(16000),
		  isRunning_(GL_FALSE),
		  surfaceFormat_(convertFormat(glFormat)) {
	setMouseTracking(true);
	setAutoBufferSwap(false);
}

void QTGLWidget::setUpdateInterval(GLint interval) {
	updateInterval_ = interval;
}

void QTGLWidget::resizeEvent(QResizeEvent *ev) {
	if (!isRunning_) {
		// QGLWidget wants to do the first resize...
		QGLWidget::resizeEvent(ev);
	} else {
		resizeGL(ev->size().width(), ev->size().height());
	}
}

void QTGLWidget::paintEvent(QPaintEvent *ev) {
}

// init GL in main thread
void QTGLWidget::initializeGL() { app_->initGL(); }

// queue resize event to be processed in render thread
void QTGLWidget::resizeGL(int w, int h) { app_->resizeGL(Vec2i(w, h)); }

void QTGLWidget::paintGL() {}

void QTGLWidget::updateGL() {}

void QTGLWidget::startRendering() {
	renderThread_.start(QThread::HighPriority);
}

void QTGLWidget::stopRendering() {
	isRunning_ = GL_FALSE;
	renderThread_.wait();
}

void QTGLWidget::run() {
	if (isRunning_) {
		REGEN_WARN("Render thread already running.");
		return;
	}
	isRunning_ = GL_TRUE;
#ifdef WAIT_ON_VSYNC
	GLint dt;
#endif

#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
	while (isRunning_)
#else
		while(app_->isMainloopRunning_)
#endif
	{
		app_->drawGL();

		// flush GL draw calls
		// Note: Seems screen does not update when other FBO then the
		//  screen FBO is bound to the current draw framebuffer.
		//  Not sure why....
		RenderState::get()->drawFrameBuffer().push(0);
		glFlush();
		RenderState::get()->drawFrameBuffer().pop();
		// some tools require buffer swapping for detecting frames.
		// for example apitrace. Is it so unusual to use single buffer
		// with offscreen FBO ?
		//swapBuffers();

#ifdef SINGLE_THREAD_GUI_AND_GRAPHICS
		app_->app_->processEvents();
#endif
		app_->updateGL();
#ifdef WAIT_ON_VSYNC
		// adjust interval to hit the desired frame rate if we can
		boost::posix_time::ptime t(
				boost::posix_time::microsec_clock::local_time());
		dt = std::max(0, updateInterval_ - (GLint)
				(t - app_->lastDisplayTime_).total_microseconds());
		// sleep desired interval
		usleepRegen(dt);
#endif
	}
}

QTGLWidget::GLThread::GLThread(QTGLWidget *glWidget)
		: QThread(), glWidget_(glWidget) {
}

void QTGLWidget::GLThread::run() {
	auto context = new QOpenGLContext();
	context->setFormat(glWidget_->surfaceFormat());
	context->create();
	context->makeCurrent(glWidget_->windowHandle());

	glWidget_->run();

	context->doneCurrent();
	delete context;
}

void QTGLWidget::mouseClick__(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick) {
	GLint x = event->x(), y = event->y();
	GLint button = qtToOgleButton(event->button());
	if (button == -1) { return; }
	Application::ButtonEvent ev;
	ev.button = button;
	ev.isDoubleClick = isDoubleClick;
	ev.pressed = isPressed;
	ev.x = x;
	ev.y = y;
	app_->mouseButton(ev);
	event->accept();
}

void QTGLWidget::mousePressEvent(QMouseEvent *event) {
	mouseClick__(event, GL_TRUE, GL_FALSE);
	event->accept();
}

void QTGLWidget::mouseDoubleClickEvent(QMouseEvent *event) {
	mouseClick__(event, GL_TRUE, GL_TRUE);
	event->accept();
}

void QTGLWidget::mouseReleaseEvent(QMouseEvent *event) {
	mouseClick__(event, GL_FALSE, GL_FALSE);
	event->accept();
}

void QTGLWidget::enterEvent(QEvent *event) {
	app_->mouseEnter();
	event->accept();
}

void QTGLWidget::leaveEvent(QEvent *event) {
	app_->mouseLeave();
	event->accept();
}

void QTGLWidget::wheelEvent(QWheelEvent *event) {
	QPointF pos = event->position();
	auto x = pos.x(), y = pos.y();
	GLint button = event->angleDelta().y() > 0 ? Application::MOUSE_WHEEL_UP : Application::MOUSE_WHEEL_DOWN;
	Application::ButtonEvent ev;
	ev.button = button;
	ev.isDoubleClick = GL_FALSE;
	ev.pressed = GL_FALSE;
	ev.x = static_cast<int>(x);
	ev.y = static_cast<int>(y);
	app_->mouseButton(ev);
	event->accept();
}

void QTGLWidget::mouseMoveEvent(QMouseEvent *event) {
	app_->mouseMove(Vec2i(event->x(), event->y()));
	event->accept();
}

void QTGLWidget::keyPressEvent(QKeyEvent *event) {
	if (event->isAutoRepeat()) {
		event->ignore();
		return;
	}
	const Vec2f &mousePos = app_->mousePosition()->getVertex(0);
	Application::KeyEvent ev;
	ev.key = event->key();
	ev.x = (GLint) mousePos.x;
	ev.y = (GLint) mousePos.y;
	app_->keyDown(ev);
	event->accept();
}

void QTGLWidget::keyReleaseEvent(QKeyEvent *event) {
	if (event->isAutoRepeat()) {
		event->ignore();
		return;
	}
	switch (event->key()) {
		case Qt::Key_Escape:
			app_->exitMainLoop(0);
			break;
		case Qt::Key_F:
			app_->toggleFullscreen();
			break;
		default: {
			const Vec2f &mousePos = app_->mousePosition()->getVertex(0);
			Application::KeyEvent ev;
			ev.key = event->key();
			ev.x = (GLint) mousePos.x;
			ev.y = (GLint) mousePos.y;
			app_->keyUp(ev);
			break;
		}
	}
	event->accept();
}

bool QTGLWidget::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QEvent::Close) {
		app_->exitMainLoop(0);
		return true;
	}
	return QObject::eventFilter(obj, event);
}
