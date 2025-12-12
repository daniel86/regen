#include <GL/glew.h>
// NOTE: QT does not like using glew :/ it will print warnings that can be ignored.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <QtGui/QOpenGLContext>
#pragma GCC diagnostic pop

#include <QtGui/QMouseEvent>
#include <QtGui/QWindow>
#include <QVBoxLayout>

#include <regen/compute/threading.h>
#include "scene-widget.h"
#include "qt-application.h"
#include "regen/animation/animation-manager.h"
#include "regen/gl/queries/elapsed-time.h"

using namespace regen;

#define WAIT_ON_VSYNC
//#define REGEN_SCENE_DEBUG_TIME

namespace regen {
	class SceneWindow : public QOpenGLWindow {
	public:
		explicit SceneWindow(SceneWidget *sceneWidget, QWindow *parent = nullptr)
				: QOpenGLWindow(NoPartialUpdate, parent),
				  sceneWidget_(sceneWidget) {}

		void initializeGL() override {}
		void resizeGL(int w, int h) override {}
		void paintGL() override {}

		// forward events to the scene widget
		void mousePressEvent(QMouseEvent *event) override
			{ sceneWidget_->mousePressEvent(event); }
		void mouseDoubleClickEvent(QMouseEvent *event) override
			{ sceneWidget_->mouseDoubleClickEvent(event); }
		void mouseReleaseEvent(QMouseEvent *event) override
			{ sceneWidget_->mouseReleaseEvent(event); }
		void wheelEvent(QWheelEvent *event) override
			{ sceneWidget_->wheelEvent(event); }
		void mouseMoveEvent(QMouseEvent *event) override
			{ sceneWidget_->mouseMoveEvent(event); }
		void keyPressEvent(QKeyEvent *event) override
			{ sceneWidget_->keyPressEvent(event); }
		void keyReleaseEvent(QKeyEvent *event) override
			{ sceneWidget_->keyReleaseEvent(event); }
	protected:
		SceneWidget *sceneWidget_;
	};
}

static int qtToOgleButton(Qt::MouseButton button) {
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

SceneWidget::SceneWidget(
		QtApplication *app,
		const QSurfaceFormat &surfaceFormat,
		QWidget *parent)
		: QWidget(parent),
		  app_(app),
		  updateInterval_(16000),
		  isRunning_(false),
		  surfaceFormat_(surfaceFormat) {
	setMouseTracking(true);
	setFocusPolicy(Qt::StrongFocus);

	sceneWindow_ = ref_ptr<SceneWindow>::alloc(this, parent->windowHandle());
	sceneWindow_->setFormat(surfaceFormat_);
    sceneWindow_->installEventFilter(this);
    // Create container widget
    winContainer_ = QWidget::createWindowContainer(sceneWindow_.get(), this);
    winContainer_->setFocusPolicy(Qt::StrongFocus);
    winContainer_->installEventFilter(this);

    // Use a layout to embed into the widget
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(winContainer_);

	renderThread_ = ref_ptr<GLThread>::alloc(this);
}

SceneWidget::~SceneWidget() {
	renderThread_ = {};
}

QSurfaceFormat SceneWidget::defaultFormat() {
	QSurfaceFormat format;
	format.setRenderableType(QSurfaceFormat::OpenGL);
	format.setProfile(QSurfaceFormat::CoreProfile);
	format.setVersion(4, 6);
	// Buffer sizes
	format.setRedBufferSize(8);
	format.setGreenBufferSize(8);
	format.setBlueBufferSize(8);
	format.setAlphaBufferSize(0);
	format.setDepthBufferSize(0);
	format.setStencilBufferSize(0);
	// Anti-aliasing
	format.setSamples(0);  // Or 4/8 if you use MSAA
	// Performance
	format.setSwapInterval(0);  // vsync OFF for uncapped FPS
	format.setSwapBehavior(QSurfaceFormat::SingleBuffer);
	format.setStereo(false);
	// Optional
	//format.setOption(QSurfaceFormat::DebugContext);
	return format;
}

void SceneWidget::setUpdateInterval(int interval) {
	updateInterval_ = interval;
}

void SceneWidget::resizeEvent(QResizeEvent *ev) {
	QWidget::resizeEvent(ev);
	app_->resizeGL(Vec2i(ev->size().width(), ev->size().height()));
}

void SceneWidget::startRendering() {
	setFocus();
	renderThread_->start(QThread::HighPriority);
}

void SceneWidget::stopRendering() {
	isRunning_ = false;
	renderThread_->wait();
}

void SceneWidget::run(QOpenGLContext *glContext) {
	if (isRunning_) {
		REGEN_WARN("Render thread already running.");
		return;
	}
	isRunning_ = true;
	int dt;
#ifdef REGEN_SCENE_DEBUG_TIME
	ElapsedTimeDebugger elapsedTime("Scene Drawing", 300);
#endif

	AnimationManager::get().resetTime();
#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
	while (isRunning_)
#else
		while(app_->isMainloopRunning_)
#endif
	{
#ifdef REGEN_SCENE_DEBUG_TIME
		elapsedTime.beginFrame();
#endif
		app_->updateTime();
#ifdef REGEN_SCENE_DEBUG_TIME
		elapsedTime.push("Update Time");
#endif
		app_->updateGL();
#ifdef REGEN_SCENE_DEBUG_TIME
		elapsedTime.push("Update GL");
#endif
		app_->drawGL();
#ifdef REGEN_SCENE_DEBUG_TIME
		elapsedTime.push("Draw GL");
#endif

		// flush GL draw calls
		// Note: Seems screen does not update when other FBO then the
		//  screen FBO is bound to the current draw framebuffer.
		//  Not sure why....
		// Note: It could be render state is re.initialized in above functions,
		//       so better ask for static singleton here.
		RenderState::get()->drawFrameBuffer().apply(0);
		glContext->swapBuffers(sceneWindow_.get());
		app_->flushGL();
#ifdef REGEN_SCENE_DEBUG_TIME
		elapsedTime.push("Flush GL");
#endif

		// invoke event handler of queued events that require GL context
		EventObject::dispatchEvents();
#ifdef SINGLE_THREAD_GUI_AND_GRAPHICS
		app_->app_->processEvents();
#endif
#ifdef REGEN_SCENE_DEBUG_TIME
		elapsedTime.push("Event Handler");
#endif

		if (app_->isVSyncEnabled()) {
			// adjust interval to hit the desired frame rate if we can
			boost::posix_time::ptime t(
					boost::posix_time::microsec_clock::local_time());
			dt = std::max(0, updateInterval_ - static_cast<int>((t -
				app_->systemTime().p_time).total_microseconds()));
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
	auto format = glWidget_->sceneWindow_->requestedFormat();
	REGEN_INFO("OpenGL format version: "
			   << format.majorVersion() << "." << format.minorVersion()
			   << (format.profile() == QSurfaceFormat::CoreProfile ? "-Core" :
				   format.profile() == QSurfaceFormat::CompatibilityProfile ? "-Compatibility" :
				   "NoProfile"));
	sharedContext->setFormat(format);
	sharedContext->setShareContext(QOpenGLContext::globalShareContext());
	sharedContext->create();
	sharedContext->makeCurrent(glWidget_->sceneWindow_.get());

	glWidget_->app_->initGL();
	glWidget_->run(sharedContext);

	sharedContext->doneCurrent();
	delete sharedContext;
}

void SceneWidget::do_mouseClick(QMouseEvent *event, bool isPressed, bool isDoubleClick) {
	int x = event->x(), y = event->y();
	int button = qtToOgleButton(event->button());
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
	do_mouseClick(event, true, false);
	setFocus();
	event->accept();
}

void SceneWidget::mouseDoubleClickEvent(QMouseEvent *event) {
	do_mouseClick(event, true, true);
	setFocus();
	event->accept();
}

void SceneWidget::mouseReleaseEvent(QMouseEvent *event) {
	do_mouseClick(event, false, false);
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
	int button = event->angleDelta().y() > 0 ? Scene::MOUSE_WHEEL_UP : Scene::MOUSE_WHEEL_DOWN;
	Scene::ButtonEvent ev{};
	ev.button = button;
	ev.isDoubleClick = false;
	ev.pressed = false;
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
	ev.x = (int) mousePos.r.x;
	ev.y = (int) mousePos.r.y;
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
			ev.x = static_cast<int>(mousePos.r.x);
			ev.y = static_cast<int>(mousePos.r.y);
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
	else if (event->type() == QEvent::Enter) {
		enterEvent(event);
		return true;
	}
	else if (event->type() == QEvent::Leave) {
		leaveEvent(event);
		return true;
	}
	return QObject::eventFilter(obj, event);
}
