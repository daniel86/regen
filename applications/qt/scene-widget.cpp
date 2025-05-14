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
#include "scene-widget.h"
#include "qt-application.h"
#include "regen/animations/animation-manager.h"

using namespace regen;

#define WAIT_ON_VSYNC

static GLint qtToOgleButton(Qt::MouseButton button) {
	switch (button) {
		case Qt::LeftButton:
			return Scene::MOUSE_BUTTON_LEFT;
		case Qt::RightButton:
			return Scene::MOUSE_BUTTON_RIGHT;
		case Qt::MiddleButton:
			return Scene::MOUSE_BUTTON_MIDDLE;
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

SceneWidget::SceneWidget(
		QtApplication *app,
		const QGLFormat &glFormat,
		QWidget *parent)
		: QGLWidget(glFormat, parent),
		  app_(app),
		  updateInterval_(16000),
		  isRunning_(GL_FALSE),
		  surfaceFormat_(convertFormat(glFormat)),
		  renderThread_(this) {
	setMouseTracking(true);
	setAutoBufferSwap(false);
}

void SceneWidget::setUpdateInterval(GLint interval) {
	updateInterval_ = interval;
}

void SceneWidget::resizeEvent(QResizeEvent *ev) {
	if (!isRunning_) {
		// QGLWidget wants to do the first resize...
		QGLWidget::resizeEvent(ev);
	} else {
		resizeGL(ev->size().width(), ev->size().height());
	}
}

// init GL in main thread
void SceneWidget::initializeGL() { app_->initGL(); }

// queue resize event to be processed in render thread
void SceneWidget::resizeGL(int w, int h) { app_->resizeGL(Vec2i(w, h)); }

void SceneWidget::startRendering() {
	renderThread_.start(QThread::HighPriority);
}

void SceneWidget::stopRendering() {
	isRunning_ = GL_FALSE;
	renderThread_.wait();
}

void SceneWidget::run() {
	if (isRunning_) {
		REGEN_WARN("Render thread already running.");
		return;
	}
	isRunning_ = GL_TRUE;
#ifdef WAIT_ON_VSYNC
	GLint dt;
#endif

	AnimationManager::get().resetTime();
#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
	while (isRunning_)
#else
		while(app_->isMainloopRunning_)
#endif
	{
		app_->updateTime();
		app_->updateGL();
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
		if (app_->isVSyncEnabled()) {
			// adjust interval to hit the desired frame rate if we can
			boost::posix_time::ptime t(
					boost::posix_time::microsec_clock::local_time());
			dt = std::max(0, updateInterval_ - (GLint)
					(t - app_->lastTime_).total_microseconds());
			// sleep desired interval
			usleepRegen(dt);
		}
	}
}

SceneWidget::GLThread::GLThread(SceneWidget *glWidget)
		: QThread(), glWidget_(glWidget) {
}

void SceneWidget::GLThread::run() {
	auto sharedContext = new QOpenGLContext();
	sharedContext->setFormat(glWidget_->surfaceFormat());
	sharedContext->setShareContext(QOpenGLContext::globalShareContext());
	sharedContext->create();
	sharedContext->makeCurrent(glWidget_->windowHandle());

	glWidget_->run();

	sharedContext->doneCurrent();
	delete sharedContext;
}

void SceneWidget::mouseClick__(QMouseEvent *event, GLboolean isPressed, GLboolean isDoubleClick) {
	GLint x = event->x(), y = event->y();
	GLint button = qtToOgleButton(event->button());
	if (button == -1) { return; }
	Scene::ButtonEvent ev{};
	ev.button = button;
	ev.isDoubleClick = isDoubleClick;
	ev.pressed = isPressed;
	ev.x = x;
	ev.y = y;
	app_->mouseButton(ev);
	event->accept();
}

void SceneWidget::mousePressEvent(QMouseEvent *event) {
	mouseClick__(event, GL_TRUE, GL_FALSE);
	event->accept();
}

void SceneWidget::mouseDoubleClickEvent(QMouseEvent *event) {
	mouseClick__(event, GL_TRUE, GL_TRUE);
	event->accept();
}

void SceneWidget::mouseReleaseEvent(QMouseEvent *event) {
	mouseClick__(event, GL_FALSE, GL_FALSE);
	event->accept();
}

void SceneWidget::enterEvent(QEvent *event) {
	app_->mouseEnter();
	event->accept();
}

void SceneWidget::leaveEvent(QEvent *event) {
	app_->mouseLeave();
	event->accept();
}

void SceneWidget::wheelEvent(QWheelEvent *event) {
	QPointF pos = event->position();
	auto x = pos.x(), y = pos.y();
	GLint button = event->angleDelta().y() > 0 ? Scene::MOUSE_WHEEL_UP : Scene::MOUSE_WHEEL_DOWN;
	Scene::ButtonEvent ev{};
	ev.button = button;
	ev.isDoubleClick = GL_FALSE;
	ev.pressed = GL_FALSE;
	ev.x = static_cast<int>(x);
	ev.y = static_cast<int>(y);
	app_->mouseButton(ev);
	event->accept();
}

void SceneWidget::mouseMoveEvent(QMouseEvent *event) {
	app_->mouseMove(Vec2i(event->x(), event->y()));
	event->accept();
}

void SceneWidget::keyPressEvent(QKeyEvent *event) {
	if (event->isAutoRepeat()) {
		event->ignore();
		return;
	}
	auto mousePos = app_->mousePosition()->getVertex(0);
	Scene::KeyEvent ev{};
	ev.key = event->key();
	ev.x = (GLint) mousePos.r.x;
	ev.y = (GLint) mousePos.r.y;
	app_->keyDown(ev);
	event->accept();
}

void SceneWidget::keyReleaseEvent(QKeyEvent *event) {
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
			auto mousePos = app_->mousePosition()->getVertex(0);
			Scene::KeyEvent ev{};
			ev.key = event->key();
			ev.x = (GLint) mousePos.r.x;
			ev.y = (GLint) mousePos.r.y;
			app_->keyUp(ev);
			break;
		}
	}
	event->accept();
}

bool SceneWidget::eventFilter(QObject *obj, QEvent *event) {
	if (event->type() == QEvent::Close) {
		app_->exitMainLoop(0);
		return true;
	}
	return QObject::eventFilter(obj, event);
}
