#include <GL/glew.h>
// NOTE: QT does not like using glew :/ it will print warnings that can be ignored.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <QOpenGLContext>
#pragma GCC diagnostic pop
#include <QtWidgets/QHBoxLayout>

#include <regen/config.h>
#include <regen/animation/animation-manager.h>
#include <regen/compute/threading.h>
#include <regen/objects/text/font.h>

#include "qt-application.h"
#include "scene-widget.h"

using namespace regen;

namespace regen {
	static constexpr uint32_t GUI_EVENT_INTERVAL = 20000; // microseconds
}

// strange QT argc/argv handling
static const char *appArgs[] = {"dummy"};
static int appArgCount = 1;

QtApplication::QtApplication(
		const int &argc, const char **argv,
		const QSurfaceFormat &glFormat,
		uint32_t width, uint32_t height)
		: Scene(argc, argv), exitCode_(0) {
	QApplication::setAttribute(Qt::AA_ShareOpenGLContexts);
	app_ = new QApplication(appArgCount, (char **) appArgs);

	glContainer_ = new QWidget(nullptr);
	glWidget_ = new SceneWidget(this, glFormat, glContainer_);
	glWidget_->setMinimumSize(100, 100);
	glWidget_->setFocusPolicy(Qt::StrongFocus);

	auto *layout = new QHBoxLayout();
	layout->setObjectName(QString::fromUtf8("shaderInputLayout"));
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setMargin(0);
	layout->setSpacing(0);
	layout->addWidget(glWidget_, 1);
	glContainer_->setLayout(layout);
	glContainer_->resize(width, height);
}

void QtApplication::toggleFullscreen() {
	QWidget *p = toplevelWidget();
	if (p->isFullScreen()) { p->showNormal(); }
	else { p->showFullScreen(); }
}

void QtApplication::show() {
	glContainer_->show();
	glWidget_->show();
	glWidget_->setFocus();
}

void QtApplication::exitMainLoop(int errorCode) {
	exitCode_ = errorCode;
	isMainloopRunning_.store(false, std::memory_order_release);
}

int QtApplication::mainLoopGUI() {
	// Make sure the window is exposed before starting rendering
	app_->processEvents();

	AnimationManager::get().resume();
	isMainloopRunning_ = true;

	toplevelWidget()->installEventFilter(glWidget_);

	auto *sceneWidget = static_cast<SceneWidget *>(glWidget_);

	sceneWidget->sceneWindow()->create();

	uint32_t waitCount = 0;
	while (!sceneWidget->sceneWindow()->isExposed()) {
		app_->processEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		if (waitCount++ > 5000) {
			// throw error after 5 seconds
			qFatal("Timeout waiting for window to become exposed.");
			return -1;
		}
	}

	sceneWidget->startRendering();
	while (isMainloopRunning_.load(std::memory_order_acquire)) {
		app_->processEvents();
		usleepRegen(GUI_EVENT_INTERVAL);
	}
	sceneWidget->stopRendering();

	AnimationManager::get().shutdown();
	BufferObject::destroyMemoryPools();
	Font::closeLibrary();

	REGEN_INFO("Exiting with status " << exitCode_ << ".");
	return exitCode_;
}

QWidget *QtApplication::toplevelWidget() {
	QWidget *p = glWidget_;
	while (p->parentWidget() != nullptr) { p = p->parentWidget(); }
	return p;
}
