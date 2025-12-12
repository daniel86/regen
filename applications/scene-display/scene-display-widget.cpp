#include <regen/camera/camera-controller.h>
#include <regen/animation/animation-manager.h>
#include <regen/utility/filesystem.h>
#include <regen/objects/text/texture-mapped-text.h>
#include <regen/textures/texture-binder.h>
#include <QLineEdit>

//#include <regen/scene/scene-xml.h>
#include <regen/scene/scene-loader.h>
#include <regen/scene/scene-processors.h>
#include <regen/scene/resource-processor.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/scene-input-xml.h>

#include "regen/behavior/person-controller.h"
#include "applications/qt/qt-events.h"
#include "regen/behavior/behavior-tree.h"
#include "regen/behavior/user-controller.h"
#include "regen/objects/terrain/blanket-trail.h"
#include "regen/textures/height-map.h"

using namespace regen::scene;
using namespace std;

#include "scene-display-widget.h"
#include "view-node.h"
#include "fps-widget.h"
#include "animation-events.h"
#include "interaction-manager.h"
#include "interactions/video-toggle.h"
#include "interactions/node-activation.h"
#include "regen/simulation/impulse-controller.h"
#include "regen/behavior/animal-controller.h"
#include "regen/av/video-recorder.h"
#include "regen/gl/states/blit-state.h"

#define CONFIG_FILE_NAME ".regen-scene-display.cfg"

/////////////////
////// Scene Loader Animation
/////////////////

class SceneLoaderAnimation : public Animation {
public:
	SceneLoaderAnimation(SceneDisplayWidget *widget, const string &sceneFile)
		: Animation(true, false),
		  widget_(widget), sceneFile_(sceneFile) {
	}

	void gpuUpdate(RenderState *rs, double dt) override {
		widget_->loadSceneGraphicsThread(sceneFile_);
	}

	SceneDisplayWidget *widget_;
	const string sceneFile_;
};

/////////////////
////// Game Time Animation
/////////////////

class GameTimeAnimation : public Animation {
public:
	explicit GameTimeAnimation(SceneDisplayWidget *widget)
		: Animation(false, true), widget_(widget) {
	}

	void cpuUpdate(double dt) override { widget_->updateGameTimeWidget(); }

protected:
	SceneDisplayWidget *widget_;
};

/////////////////
////// Mouse event handler
/////////////////

class SceneDisplayMouseHandler : public EventHandler, public Animation {
public:
	explicit SceneDisplayMouseHandler(Scene *app)
		: EventHandler(), Animation(true, false), app_(app) {
	}

	~SceneDisplayMouseHandler() override = default;

	void call(EventObject *evObject, EventData *data) override {
		if (data->eventID == Scene::BUTTON_EVENT) {
			auto *ev = (Scene::ButtonEvent *) data;
			boost::posix_time::ptime evtTime(boost::posix_time::microsec_clock::local_time());

			if (ev->button == Scene::MOUSE_BUTTON_LEFT && app_->hasHoveredObject()) {
				auto timeDiff = evtTime - buttonClickTime_[ev->button];
				if (timeDiff.total_milliseconds() < 200) {
					auto interaction = app_->getInteraction(app_->hoveredObject()->name());
					if (interaction.get() != nullptr) {
						boost::lock_guard<boost::mutex> lock(animationLock_);
						interactionQueue_.emplace(app_->hoveredObject(), interaction);
					}
				}
			}

			if (ev->pressed) {
				buttonClickTime_[ev->button] = evtTime;
			} else {
				buttonClickTime_.erase(ev->button);
			}
		}
	}

	void gpuUpdate(RenderState *rs, double dt) override {
		while (!interactionQueue_.empty()) {
			boost::lock_guard<boost::mutex> lock(animationLock_);
			auto interaction = interactionQueue_.front();
			if (!interaction.second->interactWith(interaction.first)) {
				REGEN_WARN("interaction failed with " << app_->hoveredObject()->name());
			}
			interactionQueue_.pop();
		}
	}

protected:
	Scene *app_;
	std::map<int, boost::posix_time::ptime> buttonClickTime_;
	std::queue<std::pair<ref_ptr<StateNode>, ref_ptr<SceneInteraction> > > interactionQueue_;
	boost::mutex animationLock_;
};

/////////////////
/////////////////

SceneDisplayWidget::SceneDisplayWidget(QtApplication *app)
	: QMainWindow(),
	  anchorIndex_(0),
	  inputDialog_(nullptr),
	  inputWidget_(nullptr),
	  app_(app),
	  lastUpdateTime_(boost::posix_time::microsec_clock::local_time()),
	  settings_("regen-scene-display") {
	setMouseTracking(true);
	anchorEaseInOutIntensity_ = 1.0;
	anchorPauseTime_ = 2.0;
	anchorTimeScale_ = 1.0;

	InteractionManager::registerInteraction(
		"video-toggle", ref_ptr<VideoToggleInteration>::alloc());

	// load window width/height from Qt settings
	int width = settings_.value("width", 1280).toInt();
	int height = settings_.value("height", 960).toInt();
	REGEN_INFO("Initial window size: " << width << "x" << height);

	ui_.setupUi(this);
	ui_.glWidgetLayout->addWidget(app_->glContainer(), 0, 0, 1, 1);
	ui_.worldTimeFactor->setValue(app_->worldTime().scale);
	resize(width, height);
	readConfig();
}

void SceneDisplayWidget::init() {
	if (activeFile_.empty()) {
		openFile();
	} else {
		loadScene(activeFile_);
	}
}

SceneDisplayWidget::~SceneDisplayWidget() {
	settings_.setValue("width", width());
	settings_.setValue("height", height());
	settings_.sync();
	if (inputDialog_ != nullptr) {
		delete inputDialog_;
		delete inputWidget_;
	}
}

void SceneDisplayWidget::resetFile() {
	activeFile_ = "";
	openFile();
}

void SceneDisplayWidget::readConfig() {
	// just read in the fluid file for now
	boost::filesystem::path p(userDirectory());
	p /= CONFIG_FILE_NAME;
	if (!boost::filesystem::exists(p)) return;
	ifstream cfgFile;
	cfgFile.open(p.c_str());
	cfgFile >> activeFile_;
	cfgFile.close();
}

void SceneDisplayWidget::writeConfig() {
	// just write out the fluid file for now
	boost::filesystem::path p(userDirectory());
	p /= CONFIG_FILE_NAME;
	ofstream cfgFile;
	cfgFile.open(p.c_str());
	cfgFile << activeFile_ << endl;
	cfgFile.close();
}

void SceneDisplayWidget::nextView() {
	if (viewNodes_.empty()) return;
	ViewNode &active0 = *activeView_;
	active0.node->set_isHidden(true);

	activeView_++;
	if (activeView_ == viewNodes_.end()) {
		activeView_ = viewNodes_.begin();
	}

	ViewNode &active1 = *activeView_;
	active1.node->set_isHidden(false);
	app_->toplevelWidget()->setWindowTitle(QString(active1.name.c_str()));

	if (videoRecorder_.get()) {
		auto blitState = active1.node->findStateWithType<BlitToScreen>();
		if (blitState) {
			app_->withGLContext([&] {
				videoRecorder_->setFrameBuffer(blitState->fbo(), blitState->attachment());
			});
		}
	}
}

void SceneDisplayWidget::previousView() {
	if (viewNodes_.empty()) return;
	ViewNode &active0 = *activeView_;
	active0.node->set_isHidden(true);

	if (activeView_ == viewNodes_.begin()) {
		activeView_ = viewNodes_.end();
	}
	activeView_--;

	ViewNode &active1 = *activeView_;
	active1.node->set_isHidden(false);
	app_->toplevelWidget()->setWindowTitle(QString(active1.name.c_str()));

	if (videoRecorder_.get()) {
		auto blitState = active1.node->findStateWithType<BlitToScreen>();
		if (blitState) {
			app_->withGLContext([&] {
				videoRecorder_->setFrameBuffer(blitState->fbo(), blitState->attachment());
			});
		}
	}
}

void SceneDisplayWidget::toggleOffCameraTransform() {
	if (userController_.get()) {
		userController_->stopAnimation();
	}
}

void SceneDisplayWidget::toggleOnCameraTransform() {
	if (userController_.get()) {
		// update the camera position
		userController_->setTransform(userCamera_->position(0), userCamera_->direction(0));
		userController_->startAnimation();
		userController_->cpuUpdate(0.0);
	}
}

double SceneDisplayWidget::getAnchorTime(
	const Vec3f &fromPosition, const Vec3f &toPosition) {
	double linearDistance = (fromPosition - toPosition).length();
	// travel with ~6m per second
	return std::max(2.0, linearDistance / 6.0);
}

void SceneDisplayWidget::activateAnchor() {
	auto &anchor = anchors_[anchorIndex_];
	auto &camPos = userCamera_->position(0);
	auto &camDir = userCamera_->direction(0);
	auto cameraAnchor = ref_ptr<FixedCameraAnchor>::alloc(camPos, camDir);
	double dt = getAnchorTime(anchor->position(), camPos);

	anchorAnim_ = ref_ptr<KeyFrameController>::alloc(userCamera_);
	anchorAnim_->setRepeat(false);
	anchorAnim_->setEaseInOutIntensity(anchorEaseInOutIntensity_);
	anchorAnim_->setPauseBetweenFrames(anchorPauseTime_);
	anchorAnim_->push_back(cameraAnchor, 0.0);
	anchorAnim_->push_back(anchor, dt * anchorTimeScale_);
	anchorAnim_->connect(Animation::ANIMATION_STOPPED, ref_ptr<LambdaEventHandler>::alloc(
		                     [this](EventObject *emitter, EventData *data) {
			                     toggleOnCameraTransform();
			                     anchorAnim_ = ref_ptr<KeyFrameController>();
		                     }));
	anchorAnim_->startAnimation();
}

void SceneDisplayWidget::nextAnchor() {
	if (anchors_.empty()) return;
	if (!userCamera_.get()) return;
	if (anchorAnim_.get()) {
		anchorAnim_->stopAnimation();
	}
	toggleOffCameraTransform();

	anchorIndex_++;
	if (anchorIndex_ >= anchors_.size()) {
		anchorIndex_ = 0;
	}
	activateAnchor();
}

void SceneDisplayWidget::playAnchor() {
	if (anchors_.empty()) return;
	if (!userCamera_.get()) return;
	if (anchorAnim_.get()) {
		anchorAnim_->stopAnimation();
		anchorAnim_ = ref_ptr<KeyFrameController>();
		toggleOnCameraTransform();
		return;
	}
	toggleOffCameraTransform();

	anchorAnim_ = ref_ptr<KeyFrameController>::alloc(userCamera_);
	anchorAnim_->setRepeat(true);
	//anchorAnim_->setSkipFirstFrameOnLoop(true);
	anchorAnim_->setEaseInOutIntensity(anchorEaseInOutIntensity_);
	anchorAnim_->setPauseBetweenFrames(anchorPauseTime_);
	auto camPos = userCamera_->position(0);
	auto camDir = userCamera_->direction(0);
	anchorAnim_->push_back(camPos, camDir, 0.0);
	Vec3f lastPos = camPos;
	for (auto &anchor: anchors_) {
		double dt = getAnchorTime(anchor->position(), lastPos);
		anchorAnim_->push_back(anchor, dt * anchorTimeScale_);
		lastPos = anchor->position();
	}
	anchorAnim_->cpuUpdate(0.0);
	anchorAnim_->startAnimation();
}

void SceneDisplayWidget::makeVideo(bool isClicked) {
	if (isClicked) {
		auto &view = *activeView_;
		auto blitState = view.node->findStateWithType<BlitToScreen>();
		if (!blitState) {
			REGEN_WARN("No FBOState found in view node");
			return;
		}
		auto blitFBO = blitState->fbo();
		auto &blitTextures = blitFBO->colorTextures();
		auto blitIndex = blitState->attachment() - GL_COLOR_ATTACHMENT0;
		if (blitTextures.size() < blitIndex) {
			REGEN_WARN("FBO has " << blitTextures.size() << " textures, but attachment is " << blitIndex);
			return;
		}
		app_->withGLContext([&] {
			//videoRecorder_ = ref_ptr<VideoRecorder>::alloc(blitTextures[blitIndex]);
			videoRecorder_ = ref_ptr<VideoRecorder>::alloc(blitFBO, blitState->attachment());
			videoRecorder_->initialize();
			videoRecorder_->startAnimation();
		});
	} else {
		videoRecorder_->connect(Animation::ANIMATION_STOPPED, ref_ptr<LambdaEventHandler>::alloc(
			                        [this](EventObject *emitter, EventData *data) {
				                        auto sourceFile = videoRecorder_->filename();
				                        videoRecorder_->finalize();
				                        videoRecorder_ = {};
				                        QMetaObject::invokeMethod(this, [this, sourceFile]() {
					                        // show a dialog asking the user where to save the video
					                        QString fileName = QFileDialog::getSaveFileName(this,
						                        tr("Save Video"),
						                        "regen.mp4",
						                        tr("Video Files (*.mp4 *.avi)"));
					                        if (!fileName.isEmpty()) {
						                        // Save the video to the selected file
						                        boost::filesystem::copy_file(sourceFile,
						                                                     fileName.toStdString(),
						                                                     boost::filesystem::copy_options::overwrite_existing);
						                        REGEN_INFO("Video saved to " << fileName.toStdString());
					                        }
				                        }, Qt::QueuedConnection);
			                        }));
		videoRecorder_->stopAnimation();
	}
}

void SceneDisplayWidget::toggleInputsDialog() {
	if (inputDialog_ == nullptr) {
		inputDialog_ = new QDialog(this);
		inputDialog_->setWindowTitle("ShaderInput Editor");
		inputDialog_->resize(1000, 800);

		auto *gridLayout = new QGridLayout(inputDialog_);
		gridLayout->setContentsMargins(0, 0, 0, 0);
		gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

		inputWidget_ = new ShaderInputWidget(app_, inputDialog_);
		gridLayout->addWidget(inputWidget_);
	}
	if (inputDialog_->isVisible()) {
		inputDialog_->hide();
	} else {
		inputWidget_->setNode(app_->renderTree());
		inputDialog_->show();
	}
}

void SceneDisplayWidget::toggleCameraPopup() {
	if (!userCamera_.get()) {
		REGEN_WARN("No main camera available.");
		return;
	}

	static QDialog *cameraPopup = nullptr;
	static QLineEdit *positionLineEdit = nullptr;
	static QLineEdit *directionLineEdit = nullptr;

	if (cameraPopup && cameraPopup->isVisible()) {
		cameraPopup->hide();
		return;
	}

	if (!cameraPopup) {
		cameraPopup = new QDialog(this);
		cameraPopup->setWindowTitle("Camera Properties");
		cameraPopup->setModal(false);
		cameraPopup->setAttribute(Qt::WA_DeleteOnClose, false);
		cameraPopup->setMinimumSize(500, 200); // Set the minimum size of the dialog

		auto *layout = new QVBoxLayout(cameraPopup);
		layout->setAlignment(Qt::AlignTop); // Align items to the top

		positionLineEdit = new QLineEdit(cameraPopup);
		positionLineEdit->setTextMargins(4, 4, 4, 4);
		positionLineEdit->setStyleSheet("font-size: 14px;");
		positionLineEdit->setReadOnly(true);
		layout->addWidget(new QLabel("Position:", cameraPopup));
		layout->addWidget(positionLineEdit);

		directionLineEdit = new QLineEdit(cameraPopup);
		directionLineEdit->setTextMargins(4, 4, 4, 4);
		directionLineEdit->setStyleSheet("font-size: 14px;");
		directionLineEdit->setReadOnly(true);
		layout->addWidget(new QLabel("Direction:", cameraPopup));
		layout->addWidget(directionLineEdit);

		cameraPopup->setLayout(layout);
	}

	Vec3f position = userCamera_->position(0);
	Vec3f direction = userCamera_->direction(0);

	positionLineEdit->setText(QString("%1, %2, %3").arg(position.x).arg(position.y).arg(position.z));
	directionLineEdit->setText(QString("%1, %2, %3").arg(direction.x).arg(direction.y).arg(direction.z));

	cameraPopup->show();
}

void SceneDisplayWidget::toggleWireframe() {
	bool toggleState = ui_.wireframeToggle->isChecked();
	if (toggleState) {
		if (wireframeState_.get() == nullptr) {
			wireframeState_ = ref_ptr<FillModeState>::alloc(GL_LINE);
			app_->renderTree()->state()->joinStates(wireframeState_);
		}
	} else if (wireframeState_.get() != nullptr) {
		app_->renderTree()->state()->disjoinStates(wireframeState_);
		wireframeState_ = ref_ptr<State>();
	}
}

void SceneDisplayWidget::toggleVSync() {
	if (ui_.vsyncButton->isChecked()) {
		app_->setVSyncEnabled(true);
	} else {
		app_->setVSyncEnabled(false);
	}
}

void SceneDisplayWidget::toggleInfo(bool isOn) {
	if (isOn) {
		auto guiNode = app_->renderTree()->findNodeWithName("GUI-Pass");
		if (guiNode) {
			guiNode->set_isHidden(false);
		}
	} else {
		auto guiNode = app_->renderTree()->findNodeWithName("GUI-Pass");
		if (guiNode) {
			guiNode->set_isHidden(true);
		}
	}
}

void SceneDisplayWidget::updateGameTimeWidget() {
	auto &t_ptime = app_->worldTime().p_time;
	auto t_seconds = t_ptime.time_of_day().total_seconds();
	auto t_hours = t_seconds / 3600;
	auto t_minutes = (t_seconds % 3600) / 60;
	auto t_seconds_ = t_seconds % 60;
	QTime q_time(t_hours, t_minutes, t_seconds_);

	boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
	if ((now - lastUpdateTime_).total_seconds() >= 1) {
		ui_.worldTime->setTime(q_time);
		lastUpdateTime_ = now;
	}
}

void SceneDisplayWidget::onWorldTimeChanged() {
	// get time from the time widget
	auto q_time = ui_.worldTime->time();
	// convert to time_t
	time_t t = q_time.hour() * 3600 + q_time.minute() * 60 + q_time.second();
	app_->setWorldTime(t);
}

void SceneDisplayWidget::onWorldTimeFactorChanged(double value) {
	app_->setWorldTimeScale(value);
}

void SceneDisplayWidget::openFile() {
	QFileDialog dialog(this);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setNameFilters({"XML Files (*.xml)", "All files (*.*)"});
	dialog.setViewMode(QFileDialog::Detail);
	dialog.selectFile(QString(activeFile_.c_str()));

	if (!dialog.exec()) {
		if (activeFile_.empty()) {
			REGEN_WARN("no texture updater file selected, exiting.");
			exit(0);
		}
		return;
	}

	QStringList fileNames = dialog.selectedFiles();
	activeFile_ = fileNames.first().toStdString();
	writeConfig();

	loadScene(activeFile_);
}

void SceneDisplayWidget::updateSize() {
}

void SceneDisplayWidget::loadScene(const string &sceneFile) {
	if (inputDialog_ != nullptr && inputDialog_->isVisible()) {
		inputDialog_->hide();
	}
	loadAnim_ = ref_ptr<SceneLoaderAnimation>::alloc(this, sceneFile);
	loadAnim_->startAnimation();
}

/////////////////////////////
//////// Application Configuration Node
/////////////////////////////

void SceneDisplayWidget::handleControllerConfiguration(
		scene::SceneLoader &scene, const ref_ptr<SceneInputNode> &node) {
	auto controllerMode = node->getValue<string>("mode", "first-person");
	auto controllerType = node->getValue<string>("type", "default");
	bool isUserController = (
		controllerType == "default" ||
		controllerType == "user-kinematic" ||
		controllerType == "user-impulse");
	LoadingContext ctx(&scene, {});

	if (isUserController) {
		std::vector<CameraCommandMapping> camKeyMappings;
		std::vector<MotionCommandMapping> motionKeyMappings;

		for (const auto &x: node->getChildren("key-mapping")) {
			std::string key = x->getValue("key");
			if (x->hasAttribute("camera")) {
				CameraCommandMapping mapping;
				mapping.key = key;
				mapping.command = x->getValue<CameraCommand>("camera", CameraCommand::NONE);
				if (mapping.command == CameraCommand::NONE) {
					REGEN_WARN("Invalid camera command for key mapping for key '" << mapping.key << "'.");
					continue;
				}
				camKeyMappings.push_back(mapping);
			}
			if (x->hasAttribute("motion")) {
				MotionCommandMapping mapping;
				mapping.key = key;
				mapping.toggle = x->getValue<bool>("toggle", false);
				mapping.command = x->getValue<MotionType>("motion", MotionType::MOTION_LAST);
				if (mapping.command == MotionType::MOTION_LAST) {
					REGEN_WARN("Invalid motion command for key mapping for key '" << mapping.key << "'.");
					continue;
				}
				motionKeyMappings.push_back(mapping);
			}
		}

		for (const auto &x: node->getChildren("mouse-mapping")) {
			uint32_t button = x->getValue<uint32_t>("button", 1);
			std::string key = REGEN_STRING("button" << button);
			if (x->hasAttribute("motion")) {
				MotionCommandMapping mapping;
				mapping.key = key;
				mapping.toggle = x->getValue<bool>("toggle", false);
				mapping.command = x->getValue<MotionType>("motion", MotionType::MOTION_LAST);
				if (mapping.command == MotionType::MOTION_LAST) {
					REGEN_WARN("Invalid motion command for mouse mapping for button '" << button << "'.");
					continue;
				}
				motionKeyMappings.push_back(mapping);
			}
		}

		if (node->hasAttribute("camera")) {
			userCamera_ = scene.getResources()->getCamera(&scene, node->getValue("camera"));
		}
		if (userCamera_.get() == nullptr) {
			REGEN_WARN("Unable to find camera for '" << node->getDescription() << "'.");
			return;
		}

		if (controllerType == "default") {
			userController_ = ref_ptr<CameraController>::alloc(userCamera_);
			userController_->setMeshEyeOffset(
				node->getValue<Vec3f>("eye-offset", Vec3f::zero()));
			userController_->set_moveAmount(
				node->getValue<float>("speed", 0.01f));
			userController_->setHorizontalOrientation(
				node->getValue<float>("horizontal-orientation", 0.0));
			userController_->setVerticalOrientation(
				node->getValue<float>("vertical-orientation", 0.0));
		} else if (controllerType == "user-impulse") {
			ref_ptr<ImpulseController> impulseController =
				ImpulseController::load(ctx, *node.get(), userCamera_);
			userController_ = impulseController;
		} else {
			auto userController = UserController::load(ctx, *node.get(), userCamera_);
			userController_ = userController;
			if (!motionKeyMappings.empty()) {
				auto motionEventHandler =
					ref_ptr<QtMotionEventHandler>::alloc(userController, motionKeyMappings);
				app_->connect(Scene::KEY_EVENT, motionEventHandler);
				app_->connect(Scene::BUTTON_EVENT, motionEventHandler);
				eventHandler_.emplace_back(motionEventHandler);
			}
		}

		auto cameraEventHandler =
			ref_ptr<QtCameraEventHandler>::alloc(userController_, camKeyMappings);
		cameraEventHandler->set_sensitivity(node->getValue<float>("sensitivity", 0.005f));
		app_->connect(Scene::KEY_EVENT, cameraEventHandler);
		app_->connect(Scene::BUTTON_EVENT, cameraEventHandler);
		app_->connect(Scene::MOUSE_MOTION_EVENT, cameraEventHandler);
		eventHandler_.emplace_back(cameraEventHandler);

		// Handle anchor points
		anchorEaseInOutIntensity_ = node->getValue<float>("ease-in-out", 1.0);
		anchorPauseTime_ = node->getValue<float>("anchor-pause-time", 0.5);
		anchorTimeScale_ = node->getValue<float>("anchor-time-scale", 1.0);
		for (const auto &x: node->getChildren("anchor")) {
			if (x->hasAttribute("transform")) {
				auto transform = scene.getResources()->getTransform(&scene, x->getValue("transform"));
				if (transform.get() == nullptr) {
					REGEN_WARN("Unable to find transform for anchor.");
					continue;
				}
				auto anchor = ref_ptr<TransformCameraAnchor>::alloc(transform);
				auto anchorOffset = x->getValue<Vec3f>("offset", Vec3f::one());
				anchor->setOffset(anchorOffset);
				anchor->setFollowing(x->getValue<bool>("follow", false));
				auto anchorMode = x->getValue("look-at");
				if (anchorMode == "back") {
					anchor->setMode(TransformCameraAnchor::LOOK_AT_BACK);
				} else {
					anchor->setMode(TransformCameraAnchor::LOOK_AT_FRONT);
				}
				anchors_.emplace_back(anchor);
			} else {
				auto pos = x->getValue<Vec3f>("pos", Vec3f::zero());
				auto dir = x->getValue<Vec3f>("dir", Vec3f::zero());
				pos += x->getValue<Vec3f>("offset", Vec3f::zero());
				auto anchor = ref_ptr<FixedCameraAnchor>::alloc(pos, dir);
				anchors_.emplace_back(anchor);
			}
		}
		userController_->initCameraController();
		userController_->startAnimation();
	}
	else if (controllerType == "animal") {
		auto controller = AnimalController::load(ctx, *node.get());
		for (auto &anim: controller) {
			anim->startAnimation();
		}
		animations_.insert(animations_.end(), controller.begin(), controller.end());
	}
	else if (controllerType == "person") {
		auto controller = PersonController::load(ctx, *node.get());
		for (auto &anim: controller) {
			anim->startAnimation();
		}
		animations_.insert(animations_.end(), controller.begin(), controller.end());
	}
	else if (controllerType == "key-frames") {
		ref_ptr<Camera> cam;
		if (node->hasAttribute("camera")) {
			cam = scene.getResources()->getCamera(&scene, node->getValue("camera"));
		}
		if (cam.get() == nullptr) {
			REGEN_WARN("Unable to find camera for '" << node->getDescription() << "'.");
			return;
		}
		ref_ptr<KeyFrameController> keyFramesCamera = ref_ptr<KeyFrameController>::alloc(cam);
		if (node->hasAttribute("ease-in-out")) {
			keyFramesCamera->setEaseInOutIntensity(node->getValue<double>("ease-in-out", 1.0));
		}
		for (const auto &x: node->getChildren("key-frame")) {
			keyFramesCamera->push_back(
				x->getValue<Vec3f>("pos", Vec3f(0.0f, 0.0f, 1.0f)),
				x->getValue<Vec3f>("dir", Vec3f(0.0f, 0.0f, -1.0f)),
				x->getValue<double>("dt", 0.0)
			);
		}
		keyFramesCamera->startAnimation();
		animations_.emplace_back(keyFramesCamera);
	}
	else {
		REGEN_WARN("Unhandled controller type in '" << node->getDescription() << "'.");
	}
}

float getHeight(const Vec2f &pos, const ref_ptr<HeightMap> &heightMap) {
	if (heightMap.get()) {
		heightMap->ensureTextureData();
		return heightMap->sampleHeight(pos);
	} else {
		return 0.0f;
	}
}

static ref_ptr<WorldModel> loadWorldModel(
	scene::SceneLoader &sceneParser,
	const ref_ptr<SceneInputNode> &configNode) {
	ref_ptr<WorldModel> worldModel = sceneParser.getResources()->getWorldModel();

	auto worldNode = configNode->getFirstChild("world");
	if (!worldNode.get()) return worldModel;

	ref_ptr<HeightMap> heightMap;
	if (worldNode->hasAttribute("height-map")) {
		auto heightMap2D = sceneParser.getResources()->getTexture2D(
			&sceneParser, worldNode->getValue("height-map"));
		if (!heightMap2D) {
			REGEN_WARN("Unable to find height map texture for world model in `" << worldNode->getDescription() << "`.");
		} else {
			heightMap = ref_ptr<HeightMap>::dynamicCast(heightMap2D);
			if (!heightMap) {
				REGEN_WARN("Height map texture is not a height map in `" << worldNode->getDescription() << "`.");
			}
		}
	}

	for (const auto &placeNode: worldNode->getChildren("place")) {
		// Add place of interest to the controller.
		// Here we distinguish between "HOME", "SPIRITUAL" and "GATHERING" places.
		auto placeType = placeNode->getValue<PlaceType>("type", PlaceType::HOME);
		auto placePos = placeNode->getValue<Vec2f>("point", Vec2f::zero());
		auto placeRadius = placeNode->getValue<float>("radius", 1.0f);
		float placeRadiusSq = placeRadius * placeRadius;
		auto placeHeight = getHeight(placePos, heightMap);
		auto place = ref_ptr<Place>::alloc(placeNode->getName(), placeType);
		place->setRadius(placeRadius);
		place->setPosition(Vec3f(placePos.x, placeHeight + 0.1f, placePos.y));
		ref_ptr<WorldObject> placeWO = place;
		ref_ptr<WorldObjectVec> placeWOVec = ref_ptr<WorldObjectVec>::alloc();
		placeWOVec->push_back(placeWO);
		sceneParser.putResource<WorldObjectVec>(place->name(), placeWOVec);

		// Iterate over all existing world objects, and if they were not added to a place yet,
		// check if they are within the radius of this place. If yes, add them to this place.
		for (auto &obj: worldModel->worldObjects()) {
			// Only add things to places
			if (obj->objectType() != ObjectType::THING) continue;
			if (obj->placeOfObject().get() != nullptr) continue;
			if ((obj->position2D() - placePos).lengthSquared() <= placeRadiusSq) {
				place->addWorldObject(obj);
				obj->setPlaceOfObject(place);
			}
		}

		LoadingContext ctx(&sceneParser, {});
		// load objects that are part of this place
		for (const auto &s: placeNode->getChildren("object")) {
			auto objVec = WorldObjectVec::load(ctx, *s.get());
			if (!objVec || objVec->empty()) {
				auto relPoint = s->getValue<Vec2f>("point", Vec2f::zero());
				auto absObjPos = relPoint + placePos;
				auto objHeight = getHeight(absObjPos, heightMap);
				auto wo = WorldObject::load(ctx, *s.get());
				wo->setPosition(Vec3f(absObjPos.x, objHeight + 0.1f, absObjPos.y));
				place->addWorldObject(wo);
				worldModel->addWorldObject(wo);
			} else {
				for (const auto &obj: *objVec.get()) {
					place->addWorldObject(obj);
				}
			}
		}

		for (const auto &s: placeNode->getChildren("path")) {
			std::vector<ref_ptr<WayPoint> > pathPoints;
			for (const auto &t: s->getChildren("point")) {
				auto relPoint = t->getValue<Vec2f>("value", Vec2f::zero());
				auto absPoint = relPoint + placePos;
				auto pointHeight = getHeight(absPoint, heightMap);
				// place way points slightly above the ground to avoid z-fighting
				auto point = ref_ptr<WayPoint>::alloc(
					t->getName(),
					Vec3f(absPoint.x, pointHeight + 0.1f, absPoint.y));
				point->setRadius(t->getValue<float>("radius", 1.0f));
				point->setObjectType(ObjectType::WAYPOINT);
				pathPoints.push_back(point);
				worldModel->addWorldObject(point);
			}
			auto pathType = s->getValue<PathwayType>("type", PathwayType::STROLL);
			place->addPathWay(pathType, pathPoints);
		}
		worldModel->places.push_back(place);
		worldModel->addWorldObject(place);
	}

	for (const auto &x: worldNode->getChildren("object")) {
		auto objType = x->getValue("type");
		if (objType == "waypoint") {
			auto wpPos = x->getValue<Vec2f>("point", Vec2f::zero());
			auto wpHeight = getHeight(wpPos, heightMap);
			// place way points slightly above the ground to avoid z-fighting
			ref_ptr<WayPoint> wp = ref_ptr<WayPoint>::alloc(
				x->getName(),
				Vec3f(wpPos.x, wpHeight + 0.1f, wpPos.y));
			wp->setRadius(x->getValue<float>("radius", 1.0f));
			wp->setObjectType(ObjectType::WAYPOINT);
			worldModel->wayPoints.push_back(wp);
			worldModel->wayPointMap[x->getName()] = wp;
			worldModel->addWorldObject(wp);
			auto wpVec = ref_ptr<WorldObjectVec>::alloc();
			wpVec->push_back(wp);
			sceneParser.putResource<WorldObjectVec>(wp->name(), wpVec);
		} else {
			REGEN_WARN("Unhandled object type '" << objType << "' in NPC controller.");
		}
	}

	for (const auto &x: worldNode->getChildren("path")) {
		auto from = worldModel->wayPointMap.find(x->getValue("from"));
		auto to = worldModel->wayPointMap.find(x->getValue("to"));
		if (from != worldModel->wayPointMap.end() && to != worldModel->wayPointMap.end()) {
			worldModel->wayPointConnections.push_back(std::make_pair(from->second, to->second));
		} else {
			REGEN_WARN("Unable to find way point for path in NPC controller.");
		}
	}

	sceneParser.setWorldModel(worldModel);
	return worldModel;
}

static void handleMouseConfiguration(
	QtApplication *app_,
	scene::SceneLoader &sceneParser,
	const ref_ptr<SceneInputNode> &mouseNode) {
	for (auto &child: mouseNode->getChildren()) {
		if (child->getCategory() == string("click")) {
			auto nodeName = child->getValue("node");
			if (nodeName.empty()) {
				REGEN_WARN("No node name specified for mouse interaction.");
				continue;
			}

			auto interactionName = child->getValue("interaction");
			if (!interactionName.empty()) {
				auto interaction = InteractionManager::getInteraction(interactionName);
				if (!interaction.get()) {
					REGEN_WARN("Unable to find interaction with name '" << interactionName << "'.");
					continue;
				}
				app_->registerInteraction(nodeName, interaction);
			}

			auto interactionNodeName = child->getValue("interaction-node");
			if (!interactionNodeName.empty()) {
				// parse the interaction node
				auto interactionNode = ref_ptr<StateNode>::alloc();
				interactionNode->state()->joinStates(app_->renderTree()->state());
				sceneParser.processNode(interactionNode, interactionNodeName, "node");
				// create and register the interaction
				auto interaction = ref_ptr<NodeActivation>::alloc(app_, interactionNode);
				app_->registerInteraction(nodeName, interaction);
			}
		}
	}
}

/////////////////////////////
/////////////////////////////
/////////////////////////////

void SceneDisplayWidget::loadSceneGraphicsThread(const string &sceneFile) {
	REGEN_INFO("Loading XML scene at " << sceneFile << ".");

	AnimationManager::get().pause(true);
	// Ensure all GL operations are finished before deleting GL resources.
	glFinish();

	AnimationManager::get().clear();
	AnimationManager::get().setRootState(app_->renderTree()->state());
	TextureBinder::reset();

	animations_.clear();
	viewNodes_.clear();
	anchors_.clear();
	if (physics_.get()) {
		physics_->clear();
		physics_ = {};
	}
	userCamera_ = {};
	anchorAnim_ = {};
	timeWidgetAnimation_ = {};
	anchorIndex_ = 0;
	app_->clear();
	eventHandler_.clear();

	ref_ptr<RootNode> tree = app_->renderTree();

	ref_ptr<SceneInputXML> xmlInput = ref_ptr<SceneInputXML>::alloc(sceneFile);
	scene::SceneLoader sceneParser(app_, xmlInput);
	sceneParser.setNodeProcessor(ref_ptr<ViewNodeProcessor>::alloc(&viewNodes_));
	ref_ptr<SceneInputNode> root = sceneParser.getRoot();

	// Note: configurations may refer to resources, so better to load them after the root node.
	ref_ptr<SceneInputNode> configurationNode = root->getFirstChild("node", "configuration");
	if (configurationNode.get() == nullptr) { configurationNode = root; }

	// load world model
	auto worldModel = loadWorldModel(sceneParser, configurationNode);
	app_->setWorldModel(worldModel);

	// Note: We need to do this before processing the root node, so that
	// any buffer objects created in the initialization node are available
	// when processing the root node.
	REGEN_INFO("Traversal of initialization node...");
	if (root->getFirstChild("node", "initialize").get() != nullptr) {
		ref_ptr<StateNode> initializeNode = ref_ptr<StateNode>::alloc();
		initializeNode->state()->joinStates(app_->renderTree()->state());
		sceneParser.processNode(initializeNode, "initialize", "node");
		// ensure the buffer objects of the initialization node are updated.
		StagingSystem::instance().updateData();
		initializeNode->traverse(RenderState::get());
		// Make sure that any FBO that is written to in the initialization node
		// has its content actually written.
		glFinish();
	}
	REGEN_INFO("Traversal of initialization node done.");

	// pre-load meshes and shapes
	REGEN_INFO("Loading shapes from scene...");
	sceneParser.loadShapes();
	REGEN_INFO("Shapes loaded.");

	// Process the root node
	sceneParser.processNode(tree, "root", "node");
	physics_ = sceneParser.getPhysics();
	eventHandler_.insert(eventHandler_.end(),
	                     sceneParser.getEventHandler().begin(),
	                     sceneParser.getEventHandler().end());
	spatialIndices_ = sceneParser.getResources()->getIndices();
	for (auto &x: spatialIndices_) {
		spatialIndexList_.push_back(x.second);
	}

	// Process the configuration node
	for (const auto &x: configurationNode->getChildren()) {
		if (x->getCategory() == string("controller")) {
			handleControllerConfiguration(sceneParser, x);
		} else if (x->getCategory() == string("mouse")) {
			handleMouseConfiguration(app_, sceneParser, x);
		}
	}

	app_->initializeScene();

	/////////////////////////////
	//////// Configure World Time
	/////////////////////////////

	if (configurationNode->hasAttribute("date")) {
		auto dateString = configurationNode->getValue("date");
		struct tm tm;
		if (strptime(dateString.c_str(), "%d-%m-%Y %H:%M:%S", &tm)) {
			time_t raw_time = mktime(&tm);
			boost::posix_time::ptime local_time = boost::posix_time::from_time_t(raw_time);
			boost::posix_time::ptime utc_time = local_time + boost::posix_time::hours(2);
			app_->setWorldTime(boost::posix_time::to_time_t(utc_time));
		} else {
			REGEN_WARN("Invalid date string: " << dateString << ".");
		}
	}
	if (configurationNode->hasAttribute("time-scale")) {
		auto timeScale = configurationNode->getValue<double>("time-scale", 1.0);
		app_->setWorldTimeScale(timeScale);
		ui_.worldTimeFactor->setValue(timeScale);
	}
	if (configurationNode->hasAttribute("timestamp")) {
		auto time_d = configurationNode->getValue<double>("timestamp", 0.0);
		app_->setWorldTime(static_cast<time_t>(time_d));
	}

	/////////////////////////////
	/////////////////////////////
	/////////////////////////////

	// Update view...
	if (!viewNodes_.empty()) {
		activeView_ = viewNodes_.end();
		activeView_--;
		ViewNode &active = *activeView_;
		active.node->set_isHidden(false);
		app_->toplevelWidget()->setWindowTitle(QString(active.name.c_str()));
	}

	// Update text of FPS widget
	ref_ptr<CompositeMesh> fpsWidget =
			sceneParser.getResources()->getMesh(&sceneParser, "fps-widget");
	if (fpsWidget.get() != nullptr && !fpsWidget->meshes().empty()) {
		ref_ptr<TextureMappedText> text =
				ref_ptr<TextureMappedText>::dynamicCast(*fpsWidget->meshes().begin());
		if (text.get() != nullptr) {
			fbsWidgetUpdater_ = ref_ptr<UpdateFPS>::alloc(text);
			fbsWidgetUpdater_->startAnimation();
			REGEN_INFO("FPS widget found.");
		} else {
			fbsWidgetUpdater_ = ref_ptr<Animation>();
			REGEN_INFO("Unable to find FPS widget.");
		}
	} else {
		fbsWidgetUpdater_ = ref_ptr<Animation>();
		REGEN_INFO("Unable to find FPS widget.");
	}

	if (sceneParser.getRoot()->getFirstChild("node", "animations").get() != nullptr) {
		sceneParser.processNode(tree, "animations", "node");
	}

	if (anchors_.empty()) {
		ui_.playAnchor->setEnabled(false);
		ui_.nextAnchor->setEnabled(false);
	} else {
		ui_.playAnchor->setEnabled(true);
		ui_.nextAnchor->setEnabled(true);
	}

	// add mouse event handler
	auto mouseEventHandler = ref_ptr<SceneDisplayMouseHandler>::alloc(app_);
	mouseEventHandler->setAnimationName("mouse");
	mouseEventHandler->startAnimation();
	app_->connect(Scene::BUTTON_EVENT, mouseEventHandler);
	eventHandler_.emplace_back(mouseEventHandler);

	timeWidgetAnimation_ = ref_ptr<GameTimeAnimation>::alloc(this);
	timeWidgetAnimation_->startAnimation();
	animations_.emplace_back(timeWidgetAnimation_);
	loadAnim_ = ref_ptr<Animation>();
	lightStates_ = sceneParser.getResources()->getLights();
	AnimationManager::get().setSpatialIndices(spatialIndexList_);
	AnimationManager::get().resetTime();

	AnimationManager::get().resume();
	REGEN_INFO("XML Scene Loaded.");
}

void SceneDisplayWidget::resizeEvent(QResizeEvent *event) {
	updateSize();
}
