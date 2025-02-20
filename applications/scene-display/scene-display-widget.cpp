/*
* scene-display-widget.cpp
 *
 *  Created on: Oct 26, 2013
 *      Author: daniel
 */

#include <regen/camera/camera-controller.h>
#include <regen/animations/animation-manager.h>
#include <regen/utility/filesystem.h>
#include <regen/meshes/texture-mapped-text.h>

//#include <regen/scene/scene-xml.h>
#include <regen/scene/scene-parser.h>
#include <regen/scene/input-processors.h>
#include <regen/scene/resources.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/scene-input-xml.h>

using namespace regen::scene;
using namespace std;

#include "scene-display-widget.h"
#include "view-node.h"
#include "fps-widget.h"
#include "animation-events.h"
#include "interaction-manager.h"
#include "interactions/video-toggle.h"
#include "interactions/node-activation.h"
#include "regen/physics/impulse-controller.h"
#include "regen/physics/character-controller.h"
#include "regen/animations/animal-controller.h"

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

	void glAnimate(RenderState *rs, GLdouble dt) override {
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
			: Animation(false, true), widget_(widget) {}

	void animate(GLdouble dt) override { widget_->updateGameTimeWidget(); }
protected:
	SceneDisplayWidget *widget_;
};

/////////////////
////// Mouse event handler
/////////////////

class SceneDisplayMouseHandler : public EventHandler, public Animation {
public:
	explicit SceneDisplayMouseHandler(Application *app)
			: EventHandler(), Animation(true, false), app_(app) {
	}

	void call(EventObject *evObject, EventData *data) override {
		if (data->eventID == Application::BUTTON_EVENT) {
			auto *ev = (Application::ButtonEvent *) data;
			boost::posix_time::ptime evtTime(boost::posix_time::microsec_clock::local_time());

			if (ev->button == Application::MOUSE_BUTTON_LEFT && app_->hasHoveredObject()) {
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

	void glAnimate(RenderState *rs, GLdouble dt) override {
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
	Application *app_;
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
		  lastUpdateTime_(boost::posix_time::microsec_clock::local_time()) {
	setMouseTracking(true);
	anchorEaseInOutIntensity_ = 1.0;
	anchorPauseTime_ = 2.0;
	anchorTimeScale_ = 1.0;

	InteractionManager::registerInteraction(
			"video-toggle", ref_ptr<VideoToggleInteration>::alloc());

	ui_.setupUi(this);
	ui_.glWidgetLayout->addWidget(app_->glWidgetContainer(), 0, 0, 1, 1);
	ui_.worldTimeFactor->setValue(app_->worldTime().scale);
	resize(1600, 1200);
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
	active0.node->set_isHidden(GL_TRUE);

	activeView_++;
	if (activeView_ == viewNodes_.end()) {
		activeView_ = viewNodes_.begin();
	}

	ViewNode &active1 = *activeView_;
	active1.node->set_isHidden(GL_FALSE);
	app_->toplevelWidget()->setWindowTitle(QString(active1.name.c_str()));
}

void SceneDisplayWidget::previousView() {
	if (viewNodes_.empty()) return;
	ViewNode &active0 = *activeView_;
	active0.node->set_isHidden(GL_TRUE);

	if (activeView_ == viewNodes_.begin()) {
		activeView_ = viewNodes_.end();
	}
	activeView_--;

	ViewNode &active1 = *activeView_;
	active1.node->set_isHidden(GL_FALSE);
	app_->toplevelWidget()->setWindowTitle(QString(active1.name.c_str()));
}

void SceneDisplayWidget::toggleOffCameraTransform() {
	if (cameraController_.get()) {
		cameraController_->stopAnimation();
	}
}

void SceneDisplayWidget::toggleOnCameraTransform() {
	if (cameraController_.get()) {
		// update the camera position
		cameraController_->setTransform(
				mainCamera_->position()->getVertex(0).r,
				mainCamera_->direction()->getVertex(0).r);
		cameraController_->startAnimation();
		cameraController_->animate(0.0);
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
	auto camPos = mainCamera_->position()->getVertex(0);
	auto camDir = mainCamera_->direction()->getVertex(0);
	auto cameraAnchor = ref_ptr<FixedCameraAnchor>::alloc(camPos.r, camDir.r);
	double dt = getAnchorTime(anchor->position(), camPos.r);
	camPos.unmap();
	camDir.unmap();

	anchorAnim_ = ref_ptr<KeyFrameController>::alloc(mainCamera_);
	anchorAnim_->setRepeat(GL_FALSE);
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
	if (!mainCamera_.get()) return;
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
	if (!mainCamera_.get()) return;
	auto camPos = mainCamera_->position()->getVertex(0);
	auto camDir = mainCamera_->direction()->getVertex(0);
	if (anchorAnim_.get()) {
		anchorAnim_->stopAnimation();
		anchorAnim_->updateCamera(anchorAnim_->cameraPosition(), anchorAnim_->cameraDirection(), 0.0);
		anchorAnim_ = ref_ptr<KeyFrameController>();
		toggleOnCameraTransform();
		return;
	}
	toggleOffCameraTransform();

	anchorAnim_ = ref_ptr<KeyFrameController>::alloc(mainCamera_);
	anchorAnim_->setRepeat(GL_TRUE);
	//anchorAnim_->setSkipFirstFrameOnLoop(GL_TRUE);
	anchorAnim_->setEaseInOutIntensity(anchorEaseInOutIntensity_);
	anchorAnim_->setPauseBetweenFrames(anchorPauseTime_);
	anchorAnim_->push_back(camPos.r, camDir.r, 0.0);
	Vec3f lastPos = camPos.r;
	for (auto &anchor: anchors_) {
		double dt = getAnchorTime(anchor->position(), lastPos);
		anchorAnim_->push_back(anchor, dt * anchorTimeScale_);
		lastPos = anchor->position();
	}
	camPos.unmap();
	camDir.unmap();
	anchorAnim_->animate(0.0);
	anchorAnim_->startAnimation();
}

void SceneDisplayWidget::toggleInputsDialog() {
	if (inputDialog_ == nullptr) {
		inputDialog_ = new QDialog(this);
		inputDialog_->setWindowTitle("ShaderInput Editor");
		inputDialog_->resize(1000, 800);

		auto *gridLayout = new QGridLayout(inputDialog_);
		gridLayout->setContentsMargins(0, 0, 0, 0);
		gridLayout->setObjectName(QString::fromUtf8("gridLayout"));

		inputWidget_ = new ShaderInputWidget(inputDialog_);
		gridLayout->addWidget(inputWidget_);
	}
	if (inputDialog_->isVisible()) {
		inputDialog_->hide();
	} else {
		inputWidget_->setNode(app_->renderTree());
		inputDialog_->show();
	}
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
	// FIXME: potential synchronization issue
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

void SceneDisplayWidget::handleCameraConfiguration(
		scene::SceneParser &sceneParser,
		const ref_ptr<SceneInputNode> &cameraNode) {
	ref_ptr<Camera> cam = sceneParser.getResources()->getCamera(&sceneParser, cameraNode->getName());
	if (cam.get() == nullptr) {
		REGEN_WARN("Unable to find camera for '" << cameraNode->getDescription() << "'.");
		return;
	}
	ref_ptr<ModelTransformation> transform;
	if (cameraNode->hasAttribute("transform")) {
		transform = sceneParser.getResources()->getTransform(
				&sceneParser, cameraNode->getValue("transform"));
		if (transform.get() == nullptr) {
			REGEN_WARN("Unable to find transform for '" << cameraNode->getDescription() << "'.");
			return;
		}
	}
	ref_ptr<Mesh> mesh;
	auto meshes = sceneParser.getResources()->getMesh(&sceneParser, cameraNode->getValue("mesh"));
	if (meshes.get() != nullptr && !meshes->empty()) {
		auto meshIndex = cameraNode->getValue<GLuint>("mesh-index", 0u);
		if (meshIndex >= meshes->size()) {
			REGEN_WARN("Invalid mesh index for '" << cameraNode->getDescription() << "'.");
			meshIndex = 0;
		}
		mesh = (*meshes.get())[meshIndex];
	}
	mainCamera_ = cam;
	cameraController_ = ref_ptr<CameraController>();

	auto cameraMode = cameraNode->getValue<string>("mode", "first-person");
	auto cameraType = cameraNode->getValue<string>("type", "default");

	if (cameraType == "default") {
		cameraController_ = ref_ptr<CameraController>::alloc(cam);
	}
	// IMPULSE CONTROLLER
	else if (cameraType == "impulse") {
		if (mesh.get() == nullptr) {
			REGEN_WARN("Impulse controller requires a mesh.");
			return;
		}
		if (transform.get() == nullptr) {
			REGEN_WARN("Impulse controller requires a transform.");
			return;
		}
		if (mesh->physicalObjects().empty()) {
			REGEN_WARN("Impulse controller requires a physical object.");
			return;
		}
		auto &physicalObject = mesh->physicalObjects().front();
		auto impulseController = ref_ptr<ImpulseController>::alloc(cam, physicalObject);
		cameraController_ = impulseController;
	}
	// CHARACTER CONTROLLER
	else if (cameraType == "physical-character") {
		if (mesh.get() == nullptr) {
			REGEN_WARN("Physical character controller requires a mesh.");
			return;
		}
		if (transform.get() == nullptr) {
			REGEN_WARN("Physical character controller requires a transform.");
			return;
		}
		auto characterController = ref_ptr<CharacterController>::alloc(cam, physics_);
		characterController->setCollisionHeight(
				cameraNode->getValue<GLfloat>("collision-height", 0.8));
		characterController->setCollisionRadius(
				cameraNode->getValue<GLfloat>("collision-radius", 0.8));
		characterController->setStepHeight(
				cameraNode->getValue<GLfloat>("step-height", 0.35));
		characterController->setMaxSlope(
				cameraNode->getValue<GLfloat>("max-slope", 0.8));
		characterController->setGravityForce(
				cameraNode->getValue<GLfloat>("gravity-force", 30.0));
		characterController->setJumpVelocity(
				cameraNode->getValue<GLfloat>("jump-velocity", 16.0f));
		cameraController_ = characterController;
	}
	// KEY-FRAME CONTROLLER
	else if (cameraType == "key-frames") {
		ref_ptr<KeyFrameController> keyFramesCamera = ref_ptr<KeyFrameController>::alloc(cam);
		if (cameraNode->hasAttribute("ease-in-out-intensity")) {
			keyFramesCamera->setEaseInOutIntensity(
					cameraNode->getValue<GLdouble>("ease-in-out-intensity", 1.0));
		}
		for (const auto &x: cameraNode->getChildren("key-frame")) {
			keyFramesCamera->push_back(
					x->getValue<Vec3f>("pos", Vec3f(0.0f, 0.0f, 1.0f)),
					x->getValue<Vec3f>("dir", Vec3f(0.0f, 0.0f, -1.0f)),
					x->getValue<GLdouble>("dt", 0.0)
			);
		}
		keyFramesCamera->startAnimation();
		animations_.emplace_back(keyFramesCamera);
	}
	else {
		REGEN_WARN("Invalid camera type '" << cameraType << "'.");
		return;
	}

	if (cameraController_.get()) {
		cameraController_->setMeshEyeOffset(
				cameraNode->getValue<Vec3f>("eye-offset", Vec3f(0.0)));
		cameraController_->set_moveAmount(
				cameraNode->getValue<GLfloat>("speed", 0.01f));
		cameraController_->setMeshDistance(
				cameraNode->getValue<GLfloat>("mesh-distance", 10.0f));
		cameraController_->setHorizontalOrientation(
				cameraNode->getValue<GLfloat>("horizontal-orientation", 0.0));
		cameraController_->setVerticalOrientation(
				cameraNode->getValue<GLfloat>("vertical-orientation", 0.0));
		cameraController_->setMeshHorizontalOrientation(
				cameraNode->getValue<GLfloat>("mesh-horizontal-orientation", 0.0));
		if (cameraMode == "third-person") {
			cameraController_->setCameraMode(CameraController::THIRD_PERSON);
		} else {
			cameraController_->setCameraMode(CameraController::FIRST_PERSON);
		}
		// attach the camera to a transform
		if (transform.get()) {
			cameraController_->setAttachedTo(transform->get(), mesh);
		}

		std::vector<CameraCommandMapping> keyMappings;
		for (const auto &x: cameraNode->getChildren()) {
			if (x->getCategory() == string("key-mapping")) {
				CameraCommandMapping mapping;
				mapping.key = x->getValue("key");
				mapping.command = x->getValue<CameraCommand>("command", CameraCommand::NONE);
				if (mapping.command == CameraCommand::NONE) {
					REGEN_WARN("Invalid camera command for key mapping for key '" << mapping.key << "'.");
					continue;
				}
				keyMappings.push_back(mapping);
			}
		}

		ref_ptr<QtFirstPersonEventHandler> cameraEventHandler = ref_ptr<QtFirstPersonEventHandler>::alloc(
				cameraController_, keyMappings);
		cameraEventHandler->set_sensitivity(cameraNode->getValue<GLfloat>("sensitivity", 0.005f));
		app_->connect(Application::KEY_EVENT, cameraEventHandler);
		app_->connect(Application::BUTTON_EVENT, cameraEventHandler);
		app_->connect(Application::MOUSE_MOTION_EVENT, cameraEventHandler);
		eventHandler_.emplace_back(cameraEventHandler);

		// make sure camera transforms are updated in first few frames
		cameraController_->animate(0.0);
		cameraController_->glAnimate(RenderState::get(), 0.0);
		cameraController_->startAnimation();
	}

	// read anchor points
	anchorEaseInOutIntensity_ = cameraNode->getValue<GLfloat>("ease-in-out-intensity", 1.0);
	anchorPauseTime_ = cameraNode->getValue<GLfloat>("anchor-pause-time", 0.5);
	anchorTimeScale_ = cameraNode->getValue<GLfloat>("anchor-time-scale", 1.0);
	for (const auto &x: cameraNode->getChildren("anchor")) {
		if (x->hasAttribute("transform")) {
			auto transform = sceneParser.getResources()->getTransform(&sceneParser, x->getValue("transform"));
			if (transform.get() == nullptr) {
				REGEN_WARN("Unable to find transform for anchor.");
				continue;
			}
			auto anchor = ref_ptr<TransformCameraAnchor>::alloc(transform);
			auto anchorOffset = x->getValue<Vec3f>("offset", Vec3f(1.0f));
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
			auto pos = x->getValue<Vec3f>("pos", Vec3f(0.0));
			auto dir = x->getValue<Vec3f>("dir", Vec3f(0.0));
			auto anchor = ref_ptr<FixedCameraAnchor>::alloc(pos, dir);
			anchors_.emplace_back(anchor);
		}
	}
}

static void handleAssetController(
		scene::SceneParser &sceneParser,
		const ref_ptr<SceneInputNode> &animationNode,
		std::list<ref_ptr<Animation> > &animations,
		const std::vector<ref_ptr<NodeAnimation> > &nodeAnimations,
		const std::vector<AnimRange> &ranges) {
	auto controllerType = animationNode->getValue("type");
	auto tf = sceneParser.getResources()->getTransform(
			&sceneParser, animationNode->getValue("tf"));
	auto instanceIndex = animationNode->getValue<int>("instance", 0);
	ref_ptr<AnimationController> controller;

	if (controllerType == "animal") {
		auto animalController = ref_ptr<AnimalController>::alloc(
				tf, nodeAnimations[instanceIndex], ranges);
		controller = animalController;
		animalController->setWorldTime(&sceneParser.application()->worldTime());
		animalController->setWalkSpeed(animationNode->getValue<float>("walk-speed", 0.05f));
		animalController->setRunSpeed(animationNode->getValue<float>("run-speed", 0.1f));
		animalController->setFloorHeight(animationNode->getValue<float>("floor-height", 0.0f));
		animalController->setLaziness(animationNode->getValue<float>("laziness", 0.5f));
		animalController->setMaxHeight(animationNode->getValue<float>("max-height", std::numeric_limits<float>::max()));
		animalController->setMinHeight(animationNode->getValue<float>("min-height", std::numeric_limits<float>::lowest()));
		animalController->setTerritoryBounds(
				animationNode->getValue<Vec2f>("territory-center", Vec2f(0.0)),
				animationNode->getValue<Vec2f>("territory-size", Vec2f(10.0)));

		if (animationNode->hasAttribute("height-map")) {
			auto heightMap = sceneParser.getResources()->getTexture2D(
					&sceneParser, animationNode->getValue("height-map"));
			if (heightMap.get()) {
				auto heightMapCenter = animationNode->getValue<Vec2f>("height-map-center", Vec2f(0.0));
				auto heightMapSize = animationNode->getValue<Vec2f>("height-map-size", Vec2f(10.0));
				auto heightMapFactor = animationNode->getValue<float>("height-map-factor", 8.0f);
				animalController->setHeightMap(heightMap, heightMapCenter, heightMapSize, heightMapFactor);
			} else {
				REGEN_WARN("Unable to find height map for animal controller.");
			}
		}

		for (const auto &x: animationNode->getChildren()) {
			if (x->getCategory() == string("special")) {
				animalController->addSpecial(x->getValue("name"));
			}
		}

		animalController->startAnimation();
	} else {
		REGEN_WARN("Unhandled controller type in '" << animationNode->getDescription() << "'.");
	}

	if (controller.get()) {
		animations.emplace_back(controller);
	}
}

static void handleAssetAnimationConfiguration(
		QtApplication *app_,
		scene::SceneParser &sceneParser,
		list<ref_ptr<EventHandler> > &eventHandler,
		const ref_ptr<SceneInputNode> &animationNode,
		std::list<ref_ptr<Animation> > &animations) {
	ref_ptr<AssetImporter> animAsset =
			sceneParser.getResources()->getAsset(&sceneParser, animationNode->getName());
	if (animAsset.get() == nullptr) {
		REGEN_WARN("Unable to find animation with name '" << animationNode->getName() << "'.");
		return;
	}
	vector<AnimRange> ranges = sceneParser.getAnimationRanges(animationNode->getName());
	if (ranges.empty()) {
		REGEN_WARN("Unable to find animation ranges for animation with name '" << animationNode->getName() << "'.");
		return;
	}
	std::vector<ref_ptr<NodeAnimation> > nodeAnimations_ = animAsset->getNodeAnimations();

	for (const auto &anim: nodeAnimations_) {
		anim->startAnimation();
	}

	if (animationNode->getValue("mode") == string("random")) {
		for (const auto &anim: nodeAnimations_) {
			ref_ptr<EventHandler> animStopped = ref_ptr<RandomAnimationRangeUpdater>::alloc(anim, ranges);
			anim->connect(Animation::ANIMATION_STOPPED, animStopped);
			eventHandler.push_back(animStopped);

			EventData evData;
			evData.eventID = Animation::ANIMATION_STOPPED;
			animStopped->call(anim.get(), &evData);
		}
	} else if (animationNode->getCategory() == "controller") {
		handleAssetController(sceneParser, animationNode, animations, nodeAnimations_, ranges);
	} else {
		map<string, KeyAnimationMapping> keyMappings;
		string idleAnimation = animationNode->getValue("idle");

		for (const auto &x: animationNode->getChildren()) {
			KeyAnimationMapping mapping;
			if (x->getCategory() == string("key-mapping")) {
				mapping.key = x->getValue("key");
			} else if (x->getCategory() == string("mouse-mapping")) {
				mapping.key = REGEN_STRING("button" << x->getValue<int>("button", 1));
			} else if (x->getCategory() == string("controller")) {
				handleAssetController(sceneParser, x, animations, nodeAnimations_, ranges);
				continue;
			} else {
				REGEN_WARN("Unhandled animation node " << x->getDescription() << ".");
				continue;
			}
			mapping.press = x->getValue("press");
			mapping.toggle = x->getValue<GLboolean>("toggle", GL_FALSE);
			mapping.interrupt = x->getValue<GLboolean>("interrupt", GL_FALSE);
			mapping.releaseInterrupt = x->getValue<GLboolean>("release-interrupt", GL_FALSE);
			mapping.backwards = x->getValue<GLboolean>("backwards", GL_FALSE);
			mapping.idle = x->getValue("idle");
			keyMappings[mapping.key] = mapping;
		}

		if (!keyMappings.empty()) {
			for (const auto &anim: nodeAnimations_) {
				auto keyHandler = ref_ptr<KeyAnimationRangeUpdater>::alloc(
						anim, ranges, keyMappings, idleAnimation);
				app_->connect(Application::KEY_EVENT, keyHandler);
				app_->connect(Application::BUTTON_EVENT, keyHandler);
				anim->connect(Animation::ANIMATION_STOPPED, keyHandler);
				eventHandler.emplace_back(keyHandler);
			}
		}

	}
}

static void handleMouseConfiguration(
		QtApplication *app_,
		scene::SceneParser &sceneParser,
		list<ref_ptr<EventHandler> > &eventHandler,
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

	AnimationManager::get().pause(GL_TRUE);
	AnimationManager::get().clear();
	AnimationManager::get().setRootState(app_->renderTree()->state());

	animations_.clear();
	viewNodes_.clear();
	anchors_.clear();
	if (physics_.get()) {
		physics_->clear();
		physics_ = {};
	}
	mainCamera_ = {};
	anchorAnim_ = {};
	timeWidgetAnimation_ = {};
	anchorIndex_ = 0;

	for (auto &it: eventHandler_) {
		app_->disconnect(it);
	}
	app_->clear();

	ref_ptr<RootNode> tree = app_->renderTree();

	ref_ptr<SceneInputXML> xmlInput = ref_ptr<SceneInputXML>::alloc(sceneFile);
	scene::SceneParser sceneParser(app_, xmlInput);
	sceneParser.setNodeProcessor(ref_ptr<ViewNodeProcessor>::alloc(&viewNodes_));
	sceneParser.processNode(tree, "root", "node");
	physics_ = sceneParser.getPhysics();
	eventHandler_ = sceneParser.getEventHandler();
	spatialIndices_ = sceneParser.getResources()->getIndices();

	ref_ptr<SceneInputNode> root = sceneParser.getRoot();
	ref_ptr<SceneInputNode> configurationNode = root->getFirstChild("node", "configuration");
	if (configurationNode.get() == nullptr) { configurationNode = root; }

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
	//////// Scene Parsing
	/////////////////////////////

	if (root->getFirstChild("node", "initialize").get() != nullptr) {
		ref_ptr<StateNode> initializeNode = ref_ptr<StateNode>::alloc();
		sceneParser.processNode(initializeNode, "initialize", "node");
		initializeNode->traverse(RenderState::get());
	}

	/////////////////////////////
	//////// Configuration Node
	/////////////////////////////

	// Process node children
	for (const auto &x: configurationNode->getChildren()) {
		if (x->getCategory() == string("animation")) {
			if (x->getValue("type") == string("asset")) {
				handleAssetAnimationConfiguration(app_, sceneParser, eventHandler_, x, animations_);
			}
		} else if (x->getCategory() == string("camera")) {
			handleCameraConfiguration(sceneParser, x);
		} else if (x->getCategory() == string("mouse")) {
			handleMouseConfiguration(app_, sceneParser, eventHandler_, x);
		}
	}

	/////////////////////////////
	/////////////////////////////
	/////////////////////////////

	// Update view...
	if (!viewNodes_.empty()) {
		activeView_ = viewNodes_.end();
		activeView_--;
		ViewNode &active = *activeView_;
		active.node->set_isHidden(GL_FALSE);
		app_->toplevelWidget()->setWindowTitle(QString(active.name.c_str()));
	}

	// Update text of FPS widget
	ref_ptr<MeshVector> fpsWidget =
			sceneParser.getResources()->getMesh(&sceneParser, "fps-widget");
	if (fpsWidget.get() != nullptr && !fpsWidget->empty()) {
		ref_ptr<TextureMappedText> text =
				ref_ptr<TextureMappedText>::dynamicCast(*fpsWidget->begin());
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
	app_->connect(Application::BUTTON_EVENT, mouseEventHandler);
	eventHandler_.emplace_back(mouseEventHandler);

	timeWidgetAnimation_ = ref_ptr<GameTimeAnimation>::alloc(this);
	timeWidgetAnimation_->startAnimation();
	animations_.emplace_back(timeWidgetAnimation_);
	loadAnim_ = ref_ptr<Animation>();
	lightStates_ = sceneParser.getResources()->getLights();
	AnimationManager::get().setSpatialIndices(spatialIndices_);
	AnimationManager::get().resetTime();
	AnimationManager::get().resume();
	REGEN_INFO("XML Scene Loaded.");
}

void SceneDisplayWidget::resizeEvent(QResizeEvent *event) {
	updateSize();
}
