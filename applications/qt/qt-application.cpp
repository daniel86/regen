#include <GL/glew.h>
// NOTE: QT does not like using glew :/ it will print warnings that can be ignored.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
#include <QOpenGLContext>
#pragma GCC diagnostic pop

#include <QDesktopWidget>
#include <QtWidgets/QHBoxLayout>

#include <regen/config.h>
#include <regen/animations/animation-manager.h>
#include <regen/utility/threading.h>
#include <regen/text/font.h>

#include "qt-application.h"
#include "scene-widget.h"

using namespace regen;

#ifndef SINGLE_THREAD_GUI_AND_GRAPHICS
#define EVENT_PROCESSING_INTERVAL 20000
#endif

// strange QT argc/argv handling
static const char *appArgs[] = {"dummy"};
static int appArgCount = 1;

QtApplication::QtApplication(
		const int &argc, const char **argv,
		const QSurfaceFormat &glFormat,
		GLuint width, GLuint height,
		QWidget *parent)
		: Scene(argc, argv), isMainloopRunning_(GL_FALSE), exitCode_(0) {
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
	isMainloopRunning_ = GL_FALSE;
}

int QtApplication::mainLoop() {
	AnimationManager::get().resume();
	isMainloopRunning_ = GL_TRUE;

	toplevelWidget()->installEventFilter(glWidget_);

#ifdef SINGLE_THREAD_GUI_AND_GRAPHICS
	glWidget_->run();
#else
	((SceneWidget*)glWidget_)->startRendering();
	while (isMainloopRunning_) {
		app_->processEvents();
		usleepRegen(EVENT_PROCESSING_INTERVAL);
	}
	((SceneWidget*)glWidget_)->stopRendering();
#endif
	AnimationManager::get().close();
	BufferObject::destroyMemoryPools();
	Font::closeLibrary();

	REGEN_INFO("Exiting with status " << exitCode_ << ".");
	return exitCode_;
}

QWidget *QtApplication::toplevelWidget() {
	QWidget *p = glWidget_;
	while (p->parentWidget() != NULL) { p = p->parentWidget(); }
	return p;
}
