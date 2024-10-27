/*
* scene-display-widget.cpp
 *
 *  Created on: Oct 26, 2013
 *      Author: daniel
 */

#include <regen/camera/camera-manipulator.h>
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

#define CONFIG_FILE_NAME ".regen-scene-display.cfg"

/////////////////
////// Scene Loader Animation
/////////////////

class SceneLoaderAnimation : public Animation {
public:
	SceneLoaderAnimation(SceneDisplayWidget *widget, const string &sceneFile)
			: Animation(GL_TRUE, GL_FALSE),
			  widget_(widget), sceneFile_(sceneFile) {
	}

	void glAnimate(RenderState *rs, GLdouble dt) override {
		widget_->loadSceneGraphicsThread(sceneFile_);
	}

	SceneDisplayWidget *widget_;
	const string sceneFile_;
};

/////////////////
////// Mouse event handler
/////////////////

class SceneDisplayMouseHandler : public EventHandler, Animation {
public:
	explicit SceneDisplayMouseHandler(Application *app)
			: EventHandler(), Animation(GL_TRUE, GL_FALSE), app_(app) {
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
		: QMainWindow(), inputDialog_(nullptr), inputWidget_(nullptr), app_(app) {
	setMouseTracking(true);

	InteractionManager::registerInteraction(
			"video-toggle", ref_ptr<VideoToggleInteration>::alloc());

	ui_.setupUi(this);
	ui_.glWidgetLayout->addWidget(app_->glWidgetContainer(), 0, 0, 1, 1);
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

void SceneDisplayWidget::toggleInputsDialog() {
	if (inputDialog_ == nullptr) {
		inputDialog_ = new QDialog(this);
		inputDialog_->setWindowTitle("ShaderInput Editor");
		inputDialog_->resize(600, 500);

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
}

/////////////////////////////
//////// Application Configuration Node
/////////////////////////////

static void handleFirstPersonCamera(
		QtApplication *app_,
		const ref_ptr<FirstPersonCameraTransform> &fpsCamera,
		list<ref_ptr<EventHandler> > &eventHandler,
		const ref_ptr<SceneInputNode> &cameraNode) {
	fpsCamera->set_moveAmount(cameraNode->getValue<GLfloat>("speed", 0.01f));

	ref_ptr<QtFirstPersonEventHandler> cameraEventHandler = ref_ptr<QtFirstPersonEventHandler>::alloc(fpsCamera);
	cameraEventHandler->set_sensitivity(cameraNode->getValue<GLfloat>("sensitivity", 0.005f));

	app_->connect(Application::KEY_EVENT, cameraEventHandler);
	app_->connect(Application::BUTTON_EVENT, cameraEventHandler);
	app_->connect(Application::MOUSE_MOTION_EVENT, cameraEventHandler);

	eventHandler.emplace_back(cameraEventHandler);
}

static void handleCameraConfiguration(
		QtApplication *app_,
		scene::SceneParser &sceneParser,
		list<ref_ptr<EventHandler> > &eventHandler,
		list<ref_ptr<Animation> > &animations,
		const ref_ptr<SceneInputNode> &cameraNode) {
	ref_ptr<Camera> cam = sceneParser.getResources()->getCamera(&sceneParser, cameraNode->getName());
	if (cam.get() == nullptr) {
		REGEN_WARN("Unable to find camera for '" << cameraNode->getDescription() << "'.");
		return;
	}
	auto eyeOffset = cameraNode->getValue<Vec3f>("eye-offset", Vec3f(0.0));
	auto eyeOrientation = cameraNode->getValue<GLfloat>("eye-orientation", 0.0);
	auto cameraOrientation = cameraNode->getValue<GLfloat>("camera-orientation", 0.0);
	auto mode = cameraNode->getValue<string>("type", "first-person");

	ref_ptr<FirstPersonTransform> cameraTransform;

	if (cameraNode->hasAttribute("mesh") && cameraNode->hasAttribute("transform")) {
		ref_ptr<ModelTransformation> transform = sceneParser.getResources()->getTransform(&sceneParser,
																						  cameraNode->getValue(
																								  "transform"));
		ref_ptr<MeshVector> meshes = sceneParser.getResources()->getMesh(&sceneParser, cameraNode->getValue("mesh"));
		if (transform.get() == nullptr) {
			REGEN_WARN("Unable to find transform for '" << cameraNode->getDescription() << "'.");
			return;
		}
		if (meshes.get() == nullptr || meshes->empty()) {
			REGEN_WARN("Unable to find mesh with for '" << cameraNode->getDescription() << "'.");
			return;
		}
		auto meshIndex = cameraNode->getValue<GLuint>("mesh-index", 0u);
		if (meshIndex >= meshes->size()) {
			REGEN_WARN("Invalid mesh index for '" << cameraNode->getDescription() << "'.");
			meshIndex = 0;
		}
		ref_ptr<Mesh> mesh = (*meshes.get())[meshIndex];

		if (mode == string("first-person")) {
			ref_ptr<FirstPersonCameraTransform> fpsCamera =
					ref_ptr<FirstPersonCameraTransform>::alloc(cam, mesh, transform, eyeOffset, eyeOrientation);
			fpsCamera->setCameraOrientation(cameraOrientation);
			handleFirstPersonCamera(app_, fpsCamera, eventHandler, cameraNode);
			cameraTransform = fpsCamera;
		} else if (mode == string("third-person")) {
			ref_ptr<ThirdPersonCameraTransform> fpsCamera =
					ref_ptr<ThirdPersonCameraTransform>::alloc(cam, mesh, transform, eyeOffset, eyeOrientation);
			fpsCamera->setCameraOrientation(cameraOrientation);
			handleFirstPersonCamera(app_, fpsCamera, eventHandler, cameraNode);
			cameraTransform = fpsCamera;
		}
	} else {
		if (mode == string("first-person")) {
			ref_ptr<FirstPersonCameraTransform> fpsCamera = ref_ptr<FirstPersonCameraTransform>::alloc(cam);
			fpsCamera->setCameraOrientation(cameraOrientation);
			handleFirstPersonCamera(app_, fpsCamera, eventHandler, cameraNode);
			cameraTransform = fpsCamera;
		} else if (mode == string("key-frames")) {
			ref_ptr<KeyFrameCameraTransform> keyFramesCamera = ref_ptr<KeyFrameCameraTransform>::alloc(cam);
			for (const auto &x: cameraNode->getChildren("key-frame")) {
				keyFramesCamera->push_back(
						x->getValue<Vec3f>("pos", Vec3f(0.0f, 0.0f, 1.0f)),
						x->getValue<Vec3f>("dir", Vec3f(0.0f, 0.0f, -1.0f)),
						x->getValue<GLdouble>("dt", 0.0)
				);
			}
			animations.emplace_back(keyFramesCamera);
			//cameraTransform = keyFramesCamera;
		}
	}

	if (cameraTransform.get() != nullptr) {
		auto physicsMode = FirstPersonTransform::IMPULSE;
		if (cameraNode->hasAttribute("physics-mode")) {
			auto modeStr = cameraNode->getValue("physics-mode");
			if (modeStr == "character") {
				physicsMode = FirstPersonTransform::CHARACTER;
			}
		}
		auto physicsSpeedFactor = cameraNode->getValue<GLfloat>("physics-speed-factor", 80.0f);

		cameraTransform->setPhysicsMode(physicsMode);
		cameraTransform->setPhysicsSpeedFactor(physicsSpeedFactor);

		// make sure camera transforms are updated in first few frames
		cameraTransform->animate(0.0);
		cameraTransform->glAnimate(RenderState::get(), 0.0);
	}
}

static void handleAssetAnimationConfiguration(
		QtApplication *app_,
		scene::SceneParser &sceneParser,
		list<ref_ptr<EventHandler> > &eventHandler,
		const ref_ptr<SceneInputNode> &animationNode) {
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
		for (GLuint i = 0; i < nodeAnimations_.size(); ++i) {
			const ref_ptr<NodeAnimation> &anim = nodeAnimations_[i];
			ref_ptr<EventHandler> animStopped = ref_ptr<RandomAnimationRangeUpdater>::alloc(anim, ranges);
			anim->connect(Animation::ANIMATION_STOPPED, animStopped);
			eventHandler.push_back(animStopped);

			EventData evData;
			evData.eventID = Animation::ANIMATION_STOPPED;
			animStopped->call(anim.get(), &evData);
		}
	} else {
		map<string, KeyAnimationMapping> keyMappings;
		string idleAnimation = animationNode->getValue("idle");

		for (const auto &x: animationNode->getChildren()) {
			if (x->getCategory() == string("key-mapping")) {
				KeyAnimationMapping mapping;
				mapping.key = x->getValue("key");
				mapping.press = x->getValue("press");
				mapping.toggle = x->getValue<GLboolean>("toggle", GL_FALSE);
				mapping.interrupt = x->getValue<GLboolean>("interrupt", GL_FALSE);
				mapping.releaseInterrupt = x->getValue<GLboolean>("release-interrupt", GL_FALSE);
				mapping.backwards = x->getValue<GLboolean>("backwards", GL_FALSE);
				mapping.idle = x->getValue("idle");
				keyMappings[mapping.key] = mapping;
			}
		}

		if (!keyMappings.empty()) {
			for (const auto &anim: nodeAnimations_) {
				auto keyHandler = ref_ptr<KeyAnimationRangeUpdater>::alloc(
						anim, ranges, keyMappings, idleAnimation);
				app_->connect(Application::KEY_EVENT, keyHandler);
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

	animations_.clear();
	viewNodes_.clear();
	physics_ = ref_ptr<BulletPhysics>();

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

	if (xmlInput->getRoot()->getFirstChild("node", "initialize").get() != nullptr) {
		ref_ptr<StateNode> initializeNode = ref_ptr<StateNode>::alloc();
		sceneParser.processNode(initializeNode, "initialize", "node");
		initializeNode->traverse(RenderState::get());
	}

	/////////////////////////////
	//////// Application Configuration Node
	/////////////////////////////

	ref_ptr<SceneInputNode> root = sceneParser.getRoot();
	ref_ptr<SceneInputNode> configurationNode = root->getFirstChild("node", "configuration");
	if (configurationNode.get() == nullptr) { configurationNode = root; }

	// Process node children
	for (const auto &x: configurationNode->getChildren()) {
		if (x->getCategory() == string("animation")) {
			if (x->getValue("type") == string("asset")) {
				handleAssetAnimationConfiguration(app_, sceneParser, eventHandler_, x);
			}
		} else if (x->getCategory() == string("camera")) {
			handleCameraConfiguration(app_, sceneParser, eventHandler_, animations_, x);
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
			REGEN_INFO("FPS widget found.");
		} else {
			fbsWidgetUpdater_ = ref_ptr<Animation>();
			REGEN_INFO("Unable to find FPS widget.");
		}
	} else {
		fbsWidgetUpdater_ = ref_ptr<Animation>();
		REGEN_INFO("Unable to find FPS widget.");
	}

	if (xmlInput->getRoot()->getFirstChild("node", "animations").get() != nullptr) {
		sceneParser.processNode(tree, "animations", "node");
	}

	// add mouse event handler
	auto mouseEventHandler = ref_ptr<SceneDisplayMouseHandler>::alloc(app_);
	app_->connect(Application::BUTTON_EVENT, mouseEventHandler);
	eventHandler_.emplace_back(mouseEventHandler);

	loadAnim_ = ref_ptr<Animation>();
	AnimationManager::get().resetTime();
	AnimationManager::get().resume();
	REGEN_INFO("XML Scene Loaded.");
}

void SceneDisplayWidget::resizeEvent(QResizeEvent *event) {
	updateSize();
}
