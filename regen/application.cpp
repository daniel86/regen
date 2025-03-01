/*
 * application.cpp
 *
 *  Created on: 15.10.2012
 *      Author: daniel
 */

#include <GL/glew.h>

#include <boost/foreach.hpp>

#include <regen/gl-types/glsl/includer.h>
#include <regen/config.h>

#include "application.h"

using namespace regen;

GLuint Application::BUTTON_EVENT =
		EventObject::registerEvent("button-event");
GLuint Application::KEY_EVENT =
		EventObject::registerEvent("key-event");
GLuint Application::MOUSE_MOTION_EVENT =
		EventObject::registerEvent("mouse-motion-event");
GLuint Application::MOUSE_LEAVE_EVENT =
		EventObject::registerEvent("mouse-leave-event");
GLuint Application::RESIZE_EVENT =
		EventObject::registerEvent("resize-event");

Application::Application(const int &argc, const char **argv)
		: EventObject(),
		  renderTree_(ref_ptr<RootNode>::alloc()),
		  renderState_(NULL),
		  isGLInitialized_(GL_FALSE),
		  isTimeInitialized_(GL_FALSE),
		  isVSyncEnabled_(GL_TRUE) {
	windowViewport_ = ref_ptr<ShaderInput2i>::alloc("windowViewport");
	windowViewport_->setUniformData(Vec2i(2, 2));

	mousePosition_ = ref_ptr<ShaderInput2f>::alloc("mousePosition");
	mousePosition_->setUniformData(Vec2f(0.0f));
	mouseTexco_ = ref_ptr<ShaderInput2f>::alloc("mouseTexco");
	mouseTexco_->setUniformData(Vec2f(0.0f));
	mouseDepth_ = ref_ptr<ShaderInput1f>::alloc("mouseDepthVS");
	mouseDepth_->setUniformData(0.0f);

	timeSeconds_ = ref_ptr<ShaderInput1f>::alloc("time");
	timeSeconds_->setUniformData(0.0f);
	timeDelta_ = ref_ptr<ShaderInput1f>::alloc("timeDeltaMS");
	timeDelta_->setUniformData(0.0f);
	// UTC time of the game world
	worldTime_.in = ref_ptr<ShaderInput1f>::alloc("worldTime");
	worldTime_.in->setUniformData(0.0f);
	worldTime_.p_time = boost::posix_time::microsec_clock::local_time();
	worldTime_.scale = 1.0;

	isMouseEntered_ = ref_ptr<ShaderInput1i>::alloc("mouseEntered");
	isMouseEntered_->setUniformData(0);

	requiredExt_.emplace_back("GL_VERSION_3_3");
	requiredExt_.emplace_back("GL_ARB_copy_buffer");
	requiredExt_.emplace_back("GL_ARB_framebuffer_object");
	requiredExt_.emplace_back("GL_ARB_instanced_arrays");
	requiredExt_.emplace_back("GL_ARB_texture_float");
	requiredExt_.emplace_back("GL_ARB_texture_multisample");
	requiredExt_.emplace_back("GL_ARB_viewport_array");
	requiredExt_.emplace_back("GL_ARB_uniform_buffer_object");
	requiredExt_.emplace_back("GL_ARB_vertex_array_object");
	requiredExt_.emplace_back("GL_ARB_map_buffer_range");
	requiredExt_.emplace_back("GL_EXT_texture_filter_anisotropic");

	optionalExt_.emplace_back("GL_ARB_seamless_cube_map");
	optionalExt_.emplace_back("GL_ARB_tessellation_shader");
	optionalExt_.emplace_back("GL_ARB_texture_buffer_range");

	srand(time(0));
}

void Application::addShaderPath(const std::string &path) {
	Includer::get().addIncludePath(path);
}

void Application::setupShaderLoading() {
	// try src directory first, might be more up to date then installation
	boost::filesystem::path srcPath(REGEN_SOURCE_DIR);
	srcPath /= REGEN_PROJECT_NAME;
	srcPath /= "glsl";
	addShaderPath(srcPath.string());

	// if nothing found in src dir, try insatll directory
	boost::filesystem::path installPath(REGEN_INSTALL_PREFIX);
	installPath /= "share";
	installPath /= REGEN_PROJECT_NAME;
	installPath /= "glsl";
	addShaderPath(installPath.string());
}

void Application::setupLogging() {
	Logging::addLogger(new FileLogger(Logging::INFO, "regen-info.log"));
	Logging::addLogger(new FileLogger(Logging::DEBUG, "regen-debug.log"));
	Logging::addLogger(new FileLogger(Logging::WARN, "regen-error.log"));
	Logging::addLogger(new FileLogger(Logging::ERROR, "regen-error.log"));
	Logging::addLogger(new FileLogger(Logging::FATAL, "regen-error.log"));
	Logging::addLogger(new CoutLogger(Logging::INFO));
	Logging::addLogger(new CerrLogger(Logging::FATAL));
	Logging::addLogger(new CerrLogger(Logging::ERROR));
	Logging::addLogger(new CerrLogger(Logging::WARN));
	Logging::set_verbosity(Logging::V);
}

void Application::addRequiredExtension(const std::string &ext) { requiredExt_.push_back(ext); }

void Application::addOptionalExtension(const std::string &ext) { optionalExt_.push_back(ext); }

void Application::mouseEnter() {
	isMouseEntered_->setVertex(0, 1);
	ref_ptr<MouseLeaveEvent> event = ref_ptr<MouseLeaveEvent>::alloc();
	event->entered = GL_TRUE;
	queueEmit(MOUSE_LEAVE_EVENT, event);
}

void Application::mouseLeave() {
	isMouseEntered_->setVertex(0, 0);
	ref_ptr<MouseLeaveEvent> event = ref_ptr<MouseLeaveEvent>::alloc();
	event->entered = GL_FALSE;
	queueEmit(MOUSE_LEAVE_EVENT, event);
}

ref_ptr<ShaderInput1i> Application::isMouseEntered() const {
	return isMouseEntered_;
}

void Application::updateMousePosition() {
	auto mousePosition = mousePosition_->getVertex(0);
	auto viewport = windowViewport_->getVertex(0);
	// mouse position in range [0,1] within viewport
	mouseTexco_->setVertex(0, Vec2f(
			mousePosition.r.x / (GLfloat) viewport.r.x,
			1.0 - mousePosition.r.y / (GLfloat) viewport.r.y));
}

void Application::mouseMove(const Vec2i &pos) {
	boost::posix_time::ptime time(
			boost::posix_time::microsec_clock::local_time());
	GLint dx = pos.x - mousePosition_->getVertex(0).r.x;
	GLint dy = pos.y - mousePosition_->getVertex(0).r.y;
	mousePosition_->setVertex(0, Vec2f(pos.x, pos.y));
	updateMousePosition();

	ref_ptr<MouseMotionEvent> event = ref_ptr<MouseMotionEvent>::alloc();
	event->dt = ((GLdouble) (time - lastMotionTime_).total_microseconds()) / 1000.0;
	event->dx = dx;
	event->dy = dy;
	queueEmit(MOUSE_MOTION_EVENT, event);
	lastMotionTime_ = time;
}

void Application::mouseButton(const ButtonEvent &ev) {
	mousePosition_->setVertex(0, Vec2f(ev.x, ev.y));

	ref_ptr<ButtonEvent> event = ref_ptr<ButtonEvent>::alloc();
	event->button = ev.button;
	event->x = ev.x;
	event->y = ev.y;
	event->pressed = ev.pressed;
	event->isDoubleClick = ev.isDoubleClick;
	queueEmit(BUTTON_EVENT, event);
}

void Application::keyUp(const KeyEvent &ev) {
	ref_ptr<KeyEvent> event = ref_ptr<KeyEvent>::alloc();
	event->isUp = GL_TRUE;
	event->key = ev.key;
	event->x = ev.x;
	event->y = ev.y;
	queueEmit(KEY_EVENT, event);
}

void Application::keyDown(const KeyEvent &ev) {
	ref_ptr<KeyEvent> event = ref_ptr<KeyEvent>::alloc();
	event->isUp = GL_FALSE;
	event->key = ev.key;
	event->x = ev.x;
	event->y = ev.y;
	queueEmit(KEY_EVENT, event);
}

void Application::resizeGL(const Vec2i &size) {
	windowViewport_->setVertex(0, size);
	queueEmit(RESIZE_EVENT);
	updateMousePosition();
}

void Application::initGL() {
	//glewExperimental=GL_TRUE;
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		exit(1);
	}
	// glewInit may calls glGetString(GL_EXTENSIONS),
	// then invalid enum error is generated.
	GL_ERROR_LOG();

	REGEN_DEBUG("VENDOR: " << glGetString(GL_VENDOR));
	REGEN_DEBUG("RENDERER: " << glGetString(GL_RENDERER));
	REGEN_DEBUG("VERSION: " << glGetString(GL_VERSION));

	// check for required and optional extensions
	for (auto & it : requiredExt_) {
		if (!glewIsSupported(it.c_str())) {
			REGEN_ERROR(it << " unsupported.");
			exit(0);
		} else {REGEN_DEBUG(it << " supported."); }
	}
	for (auto & it : optionalExt_) {
		if (!glewIsSupported(it.c_str())) {REGEN_DEBUG(it << " unsupported."); }
		else {REGEN_DEBUG(it << " supported."); }
	}

#define DEBUG_GLi(dname, pname) { \
    GLint val; \
    glGetIntegerv(pname, &val); \
    REGEN_DEBUG(dname << ": " << val); }
	DEBUG_GLi("MAX_3D_TEXTURE_SIZE", GL_MAX_3D_TEXTURE_SIZE);
	DEBUG_GLi("MAX_ARRAY_TEXTURE_LAYERS", GL_MAX_ARRAY_TEXTURE_LAYERS);
	DEBUG_GLi("MAX_COLOR_ATTACHMENTS", GL_MAX_COLOR_ATTACHMENTS);
	DEBUG_GLi("MAX_CUBE_MAP_TEXTURE_SIZE", GL_MAX_CUBE_MAP_TEXTURE_SIZE);
	DEBUG_GLi("MAX_DRAW_BUFFERS", GL_MAX_DRAW_BUFFERS);
#ifdef GL_MAX_FRAMEBUFFER_HEIGHT
	DEBUG_GLi("MAX_FRAMEBUFFER_HEIGHT", GL_MAX_FRAMEBUFFER_HEIGHT);
#endif
#ifdef GL_MAX_FRAMEBUFFER_WIDTH
	DEBUG_GLi("MAX_FRAMEBUFFER_WIDTH", GL_MAX_FRAMEBUFFER_WIDTH);
#endif
#ifdef GL_MAX_FRAMEBUFFER_LAYERS
	DEBUG_GLi("MAX_FRAMEBUFFER_LAYERS", GL_MAX_FRAMEBUFFER_LAYERS);
#endif
	DEBUG_GLi("MAX_TEXTURE_IMAGE_UNITS", GL_MAX_TEXTURE_IMAGE_UNITS);
	DEBUG_GLi("MAX_TEXTURE_SIZE", GL_MAX_TEXTURE_SIZE);
	DEBUG_GLi("MAX_TEXTURE_BUFFER_SIZE", GL_MAX_TEXTURE_BUFFER_SIZE);
#ifdef GL_MAX_UNIFORM_LOCATIONS
	DEBUG_GLi("MAX_UNIFORM_LOCATIONS", GL_MAX_UNIFORM_LOCATIONS);
#endif
	DEBUG_GLi("MAX_UNIFORM_BLOCK_SIZE", GL_MAX_UNIFORM_BLOCK_SIZE);
	DEBUG_GLi("MAX_VERTEX_ATTRIBS", GL_MAX_VERTEX_ATTRIBS);
	DEBUG_GLi("MAX_VIEWPORTS", GL_MAX_VIEWPORTS);
#ifdef GL_ARB_texture_buffer_range
	DEBUG_GLi("TEXTURE_BUFFER_OFFSET_ALIGNMENT", GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT);
#endif
#undef DEBUG_GLi

	setupShaderLoading();

	VBO::createMemoryPools();
	renderTree_->init();
	renderState_ = RenderState::get();
	isGLInitialized_ = GL_TRUE;
	REGEN_INFO("GL initialized.");
}

void Application::setTime() {
	lastTime_ = boost::posix_time::ptime(
			boost::posix_time::microsec_clock::local_time());
	lastMotionTime_ = lastTime_;
	isTimeInitialized_ = GL_TRUE;
}

void Application::clear() {
	renderTree_->clear();
	namedToObject_.clear();
	idToObject_.clear();
	isTimeInitialized_ = GL_FALSE;
	RenderState::reset();

	if (!globalUniforms_.get()) {
		// TODO: for some reason UBO cannot be created in initGL above.
		//       well it gets ID 1, but the first UBO in the loaded scene
		//       gets ID 1 too, then there is flickering. Not sure why.
		globalUniforms_ = ref_ptr<UniformBlock>::alloc("GlobalUniforms");
		globalUniforms_->addUniform(windowViewport_);
		globalUniforms_->addUniform(mousePosition_);
		globalUniforms_->addUniform(mouseTexco_);
		globalUniforms_->addUniform(mouseDepth_);
		globalUniforms_->addUniform(timeSeconds_);
		globalUniforms_->addUniform(timeDelta_);
		globalUniforms_->addUniform(worldTime_.in);
		globalUniforms_->addUniform(isMouseEntered_);
		renderTree_->state()->joinShaderInput(globalUniforms_);
	}
}

void Application::registerInteraction(const std::string &name, const ref_ptr<SceneInteraction> &interaction) {
	interactions_.emplace(name, interaction);
}

ref_ptr<SceneInteraction> Application::getInteraction(const std::string &name) {
	auto it = interactions_.find(name);
	if (it == interactions_.end()) {
		REGEN_WARN("Interaction with name '" << name << "' not found.");
		return {};
	}
	return it->second;
}

int Application::putNamedObject(const ref_ptr<StateNode> &node) {
	int nextId = namedToObject_.size();
	auto needId = namedToObject_.find(node->name());
	if (needId != namedToObject_.end()) {
		REGEN_WARN("Named object with name '" << node->name() << "' already exists.");
		return needId->second.id;
	}

	namedToObject_.emplace(node->name(), NamedObject{nextId, node});
	idToObject_.emplace(nextId, node);
	return nextId;
}

void Application::setHoveredObject(const ref_ptr<StateNode> &hoveredObject, const GeomPicking::PickData *pickData) {
	hoveredObject_ = hoveredObject;
	hoveredObjectPickData_ = *pickData;
	mouseDepth_->setVertex(0, pickData->depth);
}

void Application::unsetHoveredObject() {
	hoveredObject_ = {};
}

void Application::setWorldTimeScale(const double &scale) {
	worldTime_.scale = scale;
}

void Application::updateTime() {
	if (!isTimeInitialized_) {
		setTime();
	}
	boost::posix_time::ptime t(boost::posix_time::microsec_clock::local_time());
	auto dt = (t - lastTime_).total_milliseconds();
	timeDelta_->setVertex(0, (double)dt);
	timeSeconds_->setVertex(0, t.time_of_day().total_microseconds()/1e+6);
	lastTime_ = t;
	worldTime_.p_time += boost::posix_time::milliseconds(static_cast<long>(dt * worldTime_.scale));
}

void Application::setWorldTime(const time_t &t) {
	worldTime_.p_time = boost::posix_time::from_time_t(t);
	worldTime_.in->setUniformData((float)t);
}

void Application::setWorldTime(float timeInSeconds) {
	worldTime_.in->setUniformData(timeInSeconds);
	worldTime_.p_time = boost::posix_time::from_time_t((time_t)timeInSeconds);
}

void Application::drawGL() {
	renderTree_->render(timeDelta_->getVertex(0).r);
}

void Application::updateGL() {
	renderTree_->postRender(timeDelta_->getVertex(0).r);
}

namespace regen {
	class FunctionCallWithGL : public Animation {
	public:
		explicit FunctionCallWithGL(std::function<void()> f) : Animation(true, false), f_(f) {}

		void glAnimate(RenderState *rs, GLdouble dt) override {
			f_();
			stopAnimation();
		}

	protected:
		std::function<void()> f_;
	};
}

void Application::withGLContext(std::function<void()> f) {
	auto anim = ref_ptr<FunctionCallWithGL>::alloc(f);
	glCalls_.emplace_back(anim);
	anim->connect(Animation::ANIMATION_STOPPED, ref_ptr<LambdaEventHandler>::alloc(
			[this](EventObject *emitter, EventData *data) {
				for (auto & it : glCalls_) {
					if (it.get() == emitter) {
						glCalls_.erase(std::remove(glCalls_.begin(), glCalls_.end(), it), glCalls_.end());
						break;
					}
				}
			}));
	anim->startAnimation();
}
