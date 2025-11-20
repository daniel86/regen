#include <GL/glew.h>
#include "regen/shader/includer.h"
#include <regen/config.h>
#include <regen/gl-types/gl-param.h>
#include <regen/buffer/binding-manager.h>
#include <regen/textures/texture-binder.h>
#include "scene.h"
#include "regen/animation/animation-manager.h"
#include "regen/states/light-pass.h"
#include "regen/textures/texture-loader.h"
#include "regen/textures/devil-loader.h"
#include "regen/textures/stb-loader.h"

//#define REGEN_ENABLE_GL_DEBUG_OUTPUT

using namespace regen;

uint32_t Scene::BUTTON_EVENT =
		EventObject::registerEvent("button-event");
uint32_t Scene::KEY_EVENT =
		EventObject::registerEvent("key-event");
uint32_t Scene::MOUSE_MOTION_EVENT =
		EventObject::registerEvent("mouse-motion-event");
uint32_t Scene::MOUSE_LEAVE_EVENT =
		EventObject::registerEvent("mouse-leave-event");
uint32_t Scene::RESIZE_EVENT =
		EventObject::registerEvent("resize-event");

Scene::Scene(const int& /*argc*/, const char** /*argv*/)
		: EventObject(),
		  renderTree_(ref_ptr<RootNode>::alloc()),
		  renderState_(nullptr),
		  isGLInitialized_(false),
		  isTimeInitialized_(false),
		  isVSyncEnabled_(true) {
	screen_ = ref_ptr<Screen>::alloc(Vec2i(2, 2));

	mousePosition_ = ref_ptr<ShaderInput2f>::alloc("mousePosition");
	mousePosition_->setUniformData(Vec2f::zero());
	mouseTexco_ = ref_ptr<ShaderInput2f>::alloc("mouseTexco");
	mouseTexco_->setUniformData(Vec2f::zero());
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
	requiredExt_.emplace_back("GL_ARB_buffer_storage");
	requiredExt_.emplace_back("GL_ARB_uniform_buffer_object");
	requiredExt_.emplace_back("GL_ARB_vertex_array_object");
	requiredExt_.emplace_back("GL_ARB_map_buffer_range");
	requiredExt_.emplace_back("GL_EXT_texture_filter_anisotropic");

	optionalExt_.emplace_back("GL_ARB_seamless_cube_map");
	optionalExt_.emplace_back("GL_ARB_tessellation_shader");
	optionalExt_.emplace_back("GL_ARB_texture_buffer_range");
	optionalExt_.emplace_back("GL_ARB_shader_viewport_layer_array");

#ifdef HAS_STB
	TextureLoaderRegistry::registerLoader(std::make_unique<STBLoader>());
#endif
	TextureLoaderRegistry::registerLoader(std::make_unique<DevilLoader>());
}

void Scene::addShaderPath(const std::string &path) {
	Includer::get().addIncludePath(path);
}

void Scene::setupShaderLoading() {
	// try src directory first, might be more up to date then installation
	boost::filesystem::path srcPath(REGEN_SOURCE_DIR);
	srcPath /= REGEN_PROJECT_NAME;
	srcPath /= "shader";
	addShaderPath(srcPath.string());

	// if nothing found in src dir, try install directory
	boost::filesystem::path installPath(REGEN_INSTALL_PREFIX);
	installPath /= "share";
	installPath /= REGEN_PROJECT_NAME;
	installPath /= "shader";
	addShaderPath(installPath.string());
}

void Scene::setupLogging() {
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

void Scene::addRequiredExtension(const std::string &ext) { requiredExt_.push_back(ext); }

void Scene::addOptionalExtension(const std::string &ext) { optionalExt_.push_back(ext); }

void Scene::mouseEnter() {
	isMouseEntered_->setVertex(0, 1);
	ref_ptr<MouseLeaveEvent> event = ref_ptr<MouseLeaveEvent>::alloc();
	event->entered = GL_TRUE;
	queueEmit(MOUSE_LEAVE_EVENT, event);
}

void Scene::mouseLeave() {
	isMouseEntered_->setVertex(0, 0);
	ref_ptr<MouseLeaveEvent> event = ref_ptr<MouseLeaveEvent>::alloc();
	event->entered = false;
	queueEmit(MOUSE_LEAVE_EVENT, event);
}

ref_ptr<ShaderInput1i> Scene::isMouseEntered() const {
	return isMouseEntered_;
}

void Scene::updateMousePosition() {
	auto mousePosition = mousePosition_->getVertex(0);
	auto viewport = screen_->viewport();
	// mouse position in range [0,1] within viewport
	mouseTexco_->setVertex(0, Vec2f(
			mousePosition.r.x / (GLfloat) viewport.r.x,
			1.0f - mousePosition.r.y / (GLfloat) viewport.r.y));
}

void Scene::mouseMove(const Vec2i &pos) {
	boost::posix_time::ptime time(
			boost::posix_time::microsec_clock::local_time());
	int dx, dy;
	{
		auto mouse_m = mousePosition_->getVertex(0);
		dx = pos.x - static_cast<int>(mouse_m.r.x);
		dy = pos.y - static_cast<int>(mouse_m.r.y);
	}
	mousePosition_->setVertex(0, Vec2f(pos.x, pos.y));
	updateMousePosition();

	ref_ptr<MouseMotionEvent> event = ref_ptr<MouseMotionEvent>::alloc();
	event->dt = ((GLdouble) (time - lastMotionTime_).total_microseconds()) / 1000.0;
	event->dx = dx;
	event->dy = dy;
	queueEmit(MOUSE_MOTION_EVENT, event);
	lastMotionTime_ = time;
}

void Scene::mouseButton(const ButtonEvent &ev) {
	mousePosition_->setVertex(0, Vec2f(ev.x, ev.y));

	ref_ptr<ButtonEvent> event = ref_ptr<ButtonEvent>::alloc();
	event->button = ev.button;
	event->x = ev.x;
	event->y = ev.y;
	event->pressed = ev.pressed;
	event->isDoubleClick = ev.isDoubleClick;
	queueEmit(BUTTON_EVENT, event);
}

void Scene::keyUp(const KeyEvent &ev) {
	ref_ptr<KeyEvent> event = ref_ptr<KeyEvent>::alloc();
	event->isUp = true;
	event->key = ev.key;
	event->x = ev.x;
	event->y = ev.y;
	queueEmit(KEY_EVENT, event);
}

void Scene::keyDown(const KeyEvent &ev) {
	ref_ptr<KeyEvent> event = ref_ptr<KeyEvent>::alloc();
	event->isUp = false;
	event->key = ev.key;
	event->x = ev.x;
	event->y = ev.y;
	queueEmit(KEY_EVENT, event);
}

void Scene::resizeGL(const Vec2i &size) {
	screen_->setViewport(size);
	queueEmit(RESIZE_EVENT);
	updateMousePosition();
}

#ifdef REGEN_ENABLE_GL_DEBUG_OUTPUT
void GLAPIENTRY openglDebugCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar *message,
    const void *userParam)
{
    REGEN_WARN("GL DEBUG: " << message);
}
#endif

void Scene::initGL() {
	GLenum err = glewInit();
	if (GLEW_OK != err) {
		std::cerr << "Error: " << glewGetErrorString(err) << std::endl;
		exit(1);
	}
	// note: glewInit may call glGetString(GL_EXTENSIONS), then invalid enum error is generated.
	GL_ERROR_LOG();

	REGEN_DEBUG("VENDOR: " << glGetString(GL_VENDOR));
	REGEN_DEBUG("RENDERER: " << glGetString(GL_RENDERER));
	REGEN_DEBUG("VERSION: " << glGetString(GL_VERSION));

	// check for required and optional extensions
	for (auto &it: requiredExt_) {
		if (!glewIsSupported(it.c_str())) {
			REGEN_ERROR(it << " unsupported.");
			exit(0);
		} else { REGEN_DEBUG(it << " supported."); }
	}
	for (auto &it: optionalExt_) {
		if (!glewIsSupported(it.c_str())) { REGEN_DEBUG(it << " unsupported."); }
		else { REGEN_DEBUG(it << " supported."); }
	}

	REGEN_DEBUG("MAX_3D_TEXTURE_SIZE: " << glParam<int>(GL_MAX_3D_TEXTURE_SIZE));
	REGEN_DEBUG("MAX_ARRAY_TEXTURE_LAYERS: " << glParam<int>(GL_MAX_ARRAY_TEXTURE_LAYERS));
	REGEN_DEBUG("MAX_COLOR_ATTACHMENTS: " << glParam<int>(GL_MAX_COLOR_ATTACHMENTS));
	REGEN_DEBUG("MAX_CUBE_MAP_TEXTURE_SIZE: " << glParam<int>(GL_MAX_CUBE_MAP_TEXTURE_SIZE));
	REGEN_DEBUG("MAX_DRAW_BUFFERS: " << glParam<int>(GL_MAX_DRAW_BUFFERS));
#ifdef GL_MAX_FRAMEBUFFER_HEIGHT
	REGEN_DEBUG("MAX_FRAMEBUFFER_HEIGHT: " << glParam<int>(GL_MAX_FRAMEBUFFER_HEIGHT));
#endif
#ifdef GL_MAX_FRAMEBUFFER_WIDTH
	REGEN_DEBUG("MAX_FRAMEBUFFER_WIDTH: " << glParam<int>(GL_MAX_FRAMEBUFFER_WIDTH));
#endif
#ifdef GL_MAX_FRAMEBUFFER_LAYERS
	REGEN_DEBUG("MAX_FRAMEBUFFER_LAYERS: " << glParam<int>(GL_MAX_FRAMEBUFFER_LAYERS));
#endif
	REGEN_DEBUG("MAX_TEXTURE_IMAGE_UNITS: " << glParam<int>(GL_MAX_TEXTURE_IMAGE_UNITS));
	REGEN_DEBUG("MAX_TEXTURE_SIZE: " << glParam<int>(GL_MAX_TEXTURE_SIZE));
	REGEN_DEBUG("MAX_TEXTURE_BUFFER_SIZE: " << glParam<int>(GL_MAX_TEXTURE_BUFFER_SIZE));
#ifdef GL_MAX_UNIFORM_LOCATIONS
	REGEN_DEBUG("MAX_UNIFORM_LOCATIONS: " << glParam<int>(GL_MAX_UNIFORM_LOCATIONS));
#endif
	REGEN_DEBUG("MAX_UNIFORM_BLOCK_SIZE: " << glParam<int>(GL_MAX_UNIFORM_BLOCK_SIZE));
	REGEN_DEBUG("MAX_UNIFORM_BUFFER_BINDINGS: " << glParam<int>(GL_MAX_UNIFORM_BUFFER_BINDINGS));
	REGEN_DEBUG("UNIFORM_BUFFER_OFFSET_ALIGNMENT: " << glParam<int>(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT));
	REGEN_DEBUG("MAX_SHADER_STORAGE_BLOCK_SIZE: " << glParam<int>(GL_MAX_SHADER_STORAGE_BLOCK_SIZE));
	REGEN_DEBUG("MAX_SHADER_STORAGE_BUFFER_BINDINGS: " << glParam<int>(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS));
	REGEN_DEBUG("SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT: " << glParam<int>(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT));
	REGEN_DEBUG("MAX_VERTEX_ATTRIBS: " << glParam<int>(GL_MAX_VERTEX_ATTRIBS));
	REGEN_DEBUG("MAX_VIEWPORTS: " << glParam<int>(GL_MAX_VIEWPORTS));
#ifdef GL_ARB_texture_buffer_range
	REGEN_DEBUG("TEXTURE_BUFFER_OFFSET_ALIGNMENT: " << glParam<int>(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT));
#endif
	REGEN_DEBUG("MAX_COMPUTE_WORK_GROUP_SIZE: " << glParam<Vec3i>(GL_MAX_COMPUTE_WORK_GROUP_SIZE));
	REGEN_DEBUG("MAX_COMPUTE_WORK_GROUP_COUNT: " << glParam<Vec3i>(GL_MAX_COMPUTE_WORK_GROUP_COUNT));
	REGEN_DEBUG("MAX_COMPUTE_WORK_GROUP_INVOCATIONS: " << glParam<int>(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS));
	REGEN_DEBUG("MAX_COMPUTE_SHARED_MEMORY_SIZE: " << glParam<int>(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE));
	REGEN_DEBUG("MIN_MAP_BUFFER_ALIGNMENT: " << glParam<int>(GL_MIN_MAP_BUFFER_ALIGNMENT));
#undef DEBUG_GLi

#ifdef REGEN_ENABLE_GL_DEBUG_OUTPUT
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglDebugCallback, NULL);
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
#endif

	setupShaderLoading();

	BufferObject::createMemoryPools();
	renderTree_->init();
	renderState_ = RenderState::get();
	isGLInitialized_ = GL_TRUE;
	REGEN_INFO("GL initialized.");

	globalUniforms_ = ref_ptr<UBO>::alloc("GlobalUniforms", BufferUpdateFlags::FULL_PER_FRAME);
	globalUniforms_->addStagedInput(mousePosition_);
	globalUniforms_->addStagedInput(mouseTexco_);
	globalUniforms_->addStagedInput(mouseDepth_);
	globalUniforms_->addStagedInput(isMouseEntered_);
	globalUniforms_->addStagedInput(worldTime_.in);
	globalUniforms_->addStagedInput(timeSeconds_);
	globalUniforms_->addStagedInput(timeDelta_);
	renderTree_->state()->setInput(globalUniforms_);
	// Note: don't add to the UBO as it might use ring buffer causing
	// the viewport values to change with a delay of a few frames.
	renderTree_->state()->setInput(screen_->sh_viewport());
}

void Scene::setTime() {
	lastTime_ = boost::posix_time::ptime(
			boost::posix_time::microsec_clock::local_time());
	lastMotionTime_ = lastTime_;
	isTimeInitialized_ = GL_TRUE;
}

void Scene::clear() {
	disconnectAll();
	renderTree_->clear();
	namedToObject_.clear();
	idToObject_.clear();
	isTimeInitialized_ = GL_FALSE;
	StagingSystem::instance().clear();
	RenderState::reset();
	BindingManager::clear();
	TextureBinder::reset();
	ClientBuffer::resetMemoryPool();
}

void Scene::registerInteraction(const std::string &name, const ref_ptr<SceneInteraction> &interaction) {
	interactions_.emplace(name, interaction);
}

ref_ptr<SceneInteraction> Scene::getInteraction(const std::string &name) {
	auto it = interactions_.find(name);
	if (it == interactions_.end()) {
		REGEN_WARN("Interaction with name '" << name << "' not found.");
		return {};
	}
	return it->second;
}

int Scene::putNamedObject(const ref_ptr<StateNode> &node) {
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

void Scene::setHoveredObject(const ref_ptr<StateNode> &hoveredObject, const PickData *pickData) {
	hoveredObject_ = hoveredObject;
	hoveredObjectPickData_ = *pickData;
	mouseDepth_->setVertex(0, pickData->depth);
}

void Scene::unsetHoveredObject() {
	hoveredObject_ = {};
}

void Scene::setWorldTimeScale(const double &scale) {
	worldTime_.scale = scale;
}

void Scene::updateTime() {
	if (!isTimeInitialized_) {
		setTime();
	}
	boost::posix_time::ptime t(boost::posix_time::microsec_clock::local_time());
	auto dt = (t - lastTime_).total_microseconds();
	timeDelta_->setVertex(0, ((float) dt)/1000.0f);
	timeSeconds_->setVertex(0, t.time_of_day().total_microseconds() / 1e+6);
	lastTime_ = t;
	worldTime_.p_time += boost::posix_time::milliseconds(static_cast<long>((dt/1000.0) * worldTime_.scale));
}

void Scene::setWorldTime(const time_t &t) {
	worldTime_.p_time = boost::posix_time::from_time_t(t);
	worldTime_.in->setUniformData((float) t);
}

void Scene::setWorldTime(float timeInSeconds) {
	worldTime_.in->setUniformData(timeInSeconds);
	worldTime_.p_time = boost::posix_time::from_time_t((time_t) timeInSeconds);
}

void Scene::updateBOs() {
	std::stack<const StateNode*> nodeQueue;
	std::stack<const State*> stateQueue;
	std::set<const ShaderInput*> visited;
	nodeQueue.push(renderTree_.get());

	for (auto &anim : AnimationManager::get().graphicsAnimations()) {
		if(anim->animationState().get() != nullptr)
			stateQueue.push(anim->animationState().get());
	}

	while (!nodeQueue.empty()) {
		const StateNode *node = nodeQueue.top();
		nodeQueue.pop();
		stateQueue.push(node->state().get());

		// push node children to the queue
		for (const auto &child: node->childs()) {
			nodeQueue.push(child.get());
		}

		// process state of node
		while (!stateQueue.empty()) {
			const State *state = stateQueue.top();
			stateQueue.pop();

			const auto *lp = dynamic_cast<const LightPass*>(state);
			if (lp) {
				for (const auto &light: lp->lights()) {
					stateQueue.push(light.light.get());
				}
			}

			for (const auto &ni: state->inputs()) {
				auto input = ni.in_;
				if (visited.find(input.get()) != visited.end()) {
					continue; // already processed
				}
				visited.insert(input.get());

				ref_ptr<BufferBlock> bufferBlock = ref_ptr<BufferBlock>::dynamicCast(input);
				if (bufferBlock.get() && bufferBlock->stagingUpdateHint().frequency <= BUFFER_UPDATE_PER_FRAME) {
					bufferBlock->update();
				}
			}

			auto joined = state->joined();
			for (const auto &x: *joined.get()) {
				stateQueue.push(x.get());
			}
		}
	}
}

void Scene::initializeScene() {
	// traverse the scene and add buffer objects to staging arenas.
	updateBOs();
	// adopt buffer ranges for staging
	StagingSystem::instance().updateBuffers();
	StagingSystem::instance().updateData();
}

void Scene::drawGL() {
	// update staging arenas (up to per-frame frequency)
	float dt_ms = timeDelta_->getVertex(0).r;
	StagingSystem::instance().updateData(dt_ms);
	renderTree_->render(dt_ms);
}

void Scene::updateGL() {
	// Note: make sure to bind to local variable here. When using
	// `timeDelta_->getVertex(0).r` as argument, then maybe the lock is not lifted
	// before postRender starts blocking until scene loading is done.
	float dt_ms = timeDelta_->getVertex(0).r;
	renderTree_->postRender(static_cast<double>(dt_ms));
}

void Scene::flushGL() {
	AnimationManager::get().flushGraphics();
}

namespace regen {
	class FunctionCallWithGL : public Animation {
	public:
		explicit FunctionCallWithGL(std::function<void()> f) :
				Animation(true, false),
				f_(f) {}

		void glAnimate(RenderState *rs, GLdouble dt) override {
			f_();
			stopAnimation();
		}

	protected:
		std::function<void()> f_;
	};
}

void Scene::withGLContext(std::function<void()> f) {
	auto anim = ref_ptr<FunctionCallWithGL>::alloc(f);
	glCalls_.emplace_back(anim);
	anim->connect(Animation::ANIMATION_STOPPED, ref_ptr<LambdaEventHandler>::alloc(
			[this](EventObject *emitter, EventData *data) {
				for (auto &it: glCalls_) {
					if (it.get() == emitter) {
						glCalls_.erase(std::remove(glCalls_.begin(), glCalls_.end(), it), glCalls_.end());
						break;
					}
				}
			}));
	anim->startAnimation();
}
