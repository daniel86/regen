#include <iostream>

#include "mesh-viewer-widget.h"
#include <QtWidgets/QFileDialog>
#include <QtCore/QList>
#include <QtCore/QDirIterator>
#include <QtEvents>
#include <boost/filesystem/path.hpp>
#include <assimp/postprocess.h>

#include "regen/shader/shader-state.h"
#include "regen/textures/fbo-state.h"
#include <regen/gl/states/blit-state.h>
#include <regen/utility/filesystem.h>
#include <regen/animation/animation-manager.h>
#include <regen/objects/lod/mesh-simplifier.h>
#include <regen/objects/lod/impostor-billboard.h>
#include <regen/shading/direct-shading.h>
#include <applications/qt/qt-events.h>
#include <applications/qt/ColorWidget.h>
#include <applications/scene-display/animation-events.h>

using namespace std;

// Resizes Framebuffer texture when the window size changed
class FBOResizer : public EventHandler {
public:
	explicit FBOResizer(const ref_ptr<FBOState> &fbo) : EventHandler(), fboState_(fbo) {}

	~FBOResizer() override = default;

	void call(EventObject *evObject, EventData *) override {
		auto *app = (Scene *) evObject;
		auto winSize = app->screen()->viewport();
		fboState_->resize(winSize.r.x, winSize.r.y);
	}
protected:
	ref_ptr<FBOState> fboState_;
};

class RotateAnimation : public Animation {
public:
	explicit RotateAnimation(MeshViewerWidget *widget)
			: Animation(false, true), widget_(widget) {}

	~RotateAnimation() override = default;

	void cpuUpdate(double dt) override { widget_->transformMesh(dt); }
	MeshViewerWidget *widget_;
};

template<class WidgetType>
QWidget *createParameterWidget(QtApplication *app, QWidget *parent,
		const ref_ptr<StateNode> &node,
		const ref_ptr<ShaderInput> &input) {
	auto *widget = new WidgetType({app, node, input}, parent);
	widget->show();
	return widget;
}

static void hideLayout(QLayout *layout) {
	for (int i = 0; i < layout->count(); ++i) {
		QLayoutItem *item = layout->itemAt(i);
		if (item->widget()) { item->widget()->hide(); }
		if (item->layout()) { hideLayout(item->layout()); }
	}
}

static void showLayout(QLayout *layout) {
	for (int i = 0; i < layout->count(); ++i) {
		QLayoutItem *item = layout->itemAt(i);
		if (item->widget()) { item->widget()->show(); }
		if (item->layout()) { showLayout(item->layout()); }
	}
}

MeshViewerWidget::MeshViewerWidget(QtApplication *app)
		: QMainWindow(),
		  app_(app),
		  wereControlsShown_(false),
		  settings_("regen-mesh-viewer") {
	setMouseTracking(true);
	setAcceptDrops(true);
	controlsShown_ = true;
	sceneRoot_ = ref_ptr<StateNode>::alloc();
	meshRoot_ = ref_ptr<StateNode>::alloc();
	lodMeshRoot_ = ref_ptr<StateNode>::alloc();
	sceneRoot_->set_name("sceneRoot");
	meshRoot_->set_name("meshRoot");
	lodMeshRoot_->set_name("lodMeshRoot");
	sceneRoot_->addChild(meshRoot_);
	sceneRoot_->addChild(lodMeshRoot_);

	ui_.setupUi(this);
	ui_.loadMeshButton->setEnabled(false);
	ui_.meshIndexCombo->setEnabled(false);
	// load initial settings
	for (auto [check, name]: {
			std::make_pair(ui_.simplifyCheckBox, "simplify"),
			std::make_pair(ui_.genNorCheck, "genNorCheck"),
			std::make_pair(ui_.fixNorCheck, "fixNorCheck"),
			std::make_pair(ui_.genUvCheck, "genUvCheck"),
			std::make_pair(ui_.flipUvCheck, "flipUvCheck"),
			std::make_pair(ui_.deboneCheck, "deboneCheck"),
			std::make_pair(ui_.limitBonesCheck, "limitBonesCheck"),
			std::make_pair(ui_.optimizeMeshesCheck, "optimizeMeshesCheck"),
			std::make_pair(ui_.optimizeGraphCheck, "optimizeGraphCheck"),
			std::make_pair(ui_.animateCheck, "animateCheck"),
			std::make_pair(ui_.strictBoundaryCheck, "strictBoundaryCheck"),
			std::make_pair(ui_.impostorCheck, "impostorCheck"),
			std::make_pair(ui_.hemisphericalCheck, "hemisphericalCheck"),
			std::make_pair(ui_.topViewCheck, "topViewCheck"),
			std::make_pair(ui_.bottomViewCheck, "bottomViewCheck"),
			std::make_pair(ui_.depthCorrectionCheck, "depthCorrectionCheck"),
			std::make_pair(ui_.normalCorrectionCheck, "normalCorrectionCheck"),
	}) {
		check->setChecked(settings_.value(name, check->isChecked()).toBool());
	}
	for (auto [spin, name]: {
			std::make_pair(ui_.simplify0Spin, "simplify0"),
			std::make_pair(ui_.simplify1Spin, "simplify1"),
			std::make_pair(ui_.simplify2Spin, "simplify2"),
			std::make_pair(ui_.simplify3Spin, "simplify3"),
			std::make_pair(ui_.norMaxAngleSpin, "norMaxAngle"),
			std::make_pair(ui_.norPenaltySpin, "norPenalty"),
			std::make_pair(ui_.valencePenaltySpin, "valencePenalty"),
			std::make_pair(ui_.areaPenaltySpin, "areaPenalty"),
			std::make_pair(ui_.depthOffsetSpin, "depthOffset")
	}) {
		spin->setValue(settings_.value(name, spin->value()).toFloat());
	}
	for (auto [spin, name]: {
			std::make_pair(ui_.longitudeSpin, "longitudeSteps"),
			std::make_pair(ui_.latitudeSpin, "latitudeSteps"),
			std::make_pair(ui_.impostorSizeSpin, "impostorSize")
	}) {
		spin->setValue(settings_.value(name, spin->value()).toFloat());
	}
	for (auto [button, name]: {
			std::make_pair(ui_.animateCarret, "animateCaret"),
			std::make_pair(ui_.simplifyCarret, "simplifyCaret"),
			std::make_pair(ui_.impostorCarret, "impostorCaret"),
	}) {
		button->setChecked(settings_.value(name, button->isChecked()).toBool());
	}
	auto lastPath = settings_.value("lastPath", "").toString();
	if (!lastPath.isEmpty()) {
		assetFilePath_ = lastPath.toStdString();
		auto assetFilename = boost::filesystem::path(assetFilePath_).filename().string();
		ui_.assetNameEdit->setText(QString::fromStdString(assetFilename));
		ui_.loadMeshButton->setEnabled(true);
	}
	ui_.glWidgetLayout->addWidget(app_->glContainer(), 0, 0, 1, 1);

	fullscreenLayout_ = new QVBoxLayout();
	fullscreenLayout_->setObjectName(QString::fromUtf8("fullscreenLayout"));
	ui_.gridLayout_2->addLayout(fullscreenLayout_, 0, 0, 1, 1);
	hideLayout(fullscreenLayout_);

	resize(1600, 1200);
	ui_.splitter->setSizes(QList<int>({1200, 400}));
	updateSize();
	app_->withGLContext([this]() { gl_loadScene(); });
}

void MeshViewerWidget::simplifyMesh_GL(const Vec4f &thresholds) {
	for (auto &mesh: meshes_) {
		MeshSimplifier simplifier(mesh);
		simplifier.setThresholds(thresholds);
		simplifier.setNormalMaxAngle(static_cast<float>(ui_.norMaxAngleSpin->value()));
		simplifier.setNormalPenalty(static_cast<float>(ui_.norPenaltySpin->value()));
		simplifier.setValencePenalty(static_cast<float>(ui_.valencePenaltySpin->value()));
		simplifier.setAreaPenalty(static_cast<float>(ui_.areaPenaltySpin->value()));
		if (ui_.strictBoundaryCheck->isChecked()) {
			simplifier.setUseStrictBoundary(true);
		} else {
			simplifier.setUseStrictBoundary(false);
		}
		simplifier.simplifyMesh();
	}
}

void MeshViewerWidget::billboardMeshes_GL() {
	for (auto &mesh: meshes_) {
		auto billboard = ref_ptr<ImpostorBillboard>::alloc();
		billboard->addMesh(mesh);
		billboard->setLongitudeSteps(ui_.longitudeSpin->value());
		billboard->setLatitudeSteps(ui_.latitudeSpin->value());
		billboard->setHemispherical(ui_.hemisphericalCheck->isChecked());
		billboard->setHasTopView(ui_.topViewCheck->isChecked());
		billboard->setHasBottomView(ui_.bottomViewCheck->isChecked());
		billboard->setDepthOffset(static_cast<float>(ui_.depthOffsetSpin->value()));
		billboard->setUseNormalCorrection(ui_.normalCorrectionCheck->isChecked());
		billboard->setSnapshotTextureSize(ui_.impostorSizeSpin->value(), ui_.impostorSizeSpin->value());
		billboard->setShaderKey("regen.models.impostor");
		billboard->setSnapshotShaderKey("regen.models.impostor.update");
		billboard->updateSnapshotViews();
		billboard->createSnapshot();

		// add a hidden node for the UI
		auto meshNode = ref_ptr<StateNode>::alloc(billboard);
		meshNode->set_name("billboard");
		meshNode->set_isHidden(true);
		meshRoot_->addChild(meshNode);

		mesh->addMeshLOD(Mesh::MeshLOD(billboard));
	}
}

void MeshViewerWidget::updateLoDButtons() {
	if (meshes_.empty()) {
		ui_.lodControls->setEnabled(false);
		return;
	}
	ui_.lodControls->setEnabled(true);
	auto &firstMesh = meshes_[0];
	auto numLODs = firstMesh->numLODs();
	for (auto &mesh: meshes_) {
		if (mesh->numLODs() > numLODs) {
			numLODs = mesh->numLODs();
		}
	}
	REGEN_INFO("Number of LODs: " << numLODs);
	ui_.lod0Button->setEnabled(true);
	ui_.lod1Button->setEnabled(numLODs > 1);
	ui_.lod2Button->setEnabled(numLODs > 2);
	ui_.lod3Button->setEnabled(numLODs > 3);
}

void MeshViewerWidget::loadMeshes_GL(const std::string &assetPath) {
	static const BufferFlags bufferCfg(ARRAY_BUFFER, BufferUpdateFlags::NEVER);
	auto p = resourcePath(assetPath);
	meshRoot_->clear();
	lodMeshRoot_->clear();
	meshNodes_.clear();
	meshes_.clear();

	// Read values from UI spinner
	bool simplify = ui_.simplifyCheckBox->isChecked();
	bool useBillboard = ui_.impostorCheck->isChecked();
	// Apply transform to model on import
	Mat4f transform = Mat4f::identity();
	asset_ = ref_ptr<AssetImporter>::alloc(p);
	asset_->setImportFlag(AssetImporter::IGNORE_NORMAL_MAP);
	if (ui_.animateCheck->isChecked()) {
		asset_->setAnimationConfig(AssimpAnimationConfig(ui_.tpsSpin->value()));
	}
	setAssImpFlags();
	asset_->importAsset();
	meshes_ = asset_->loadAllMeshes(transform, bufferCfg);
	if (meshes_.empty()) {
		REGEN_WARN("No meshes loaded from " << p);
		// remove all elements from the mesh index combo box and disable it
		ui_.meshIndexCombo->clear();
		ui_.meshIndexCombo->setEnabled(false);
	} else {
		// compute mesh scale based on its bounding box. scale such that it is 1 unit in y direction.
		auto &firstMesh = meshes_[0];
		Bounds<Vec3f> bounds;
		bounds.min = firstMesh->minPosition();
		bounds.max = firstMesh->maxPosition();
		for (uint32_t i = 1; i < meshes_.size(); ++i) {
			auto &mesh = meshes_[i];
			bounds.min.setMin(mesh->minPosition());
			bounds.max.setMax(mesh->maxPosition());
		}
		meshOrigin_ = -bounds.center();
		meshScale_ = 1.0f / std::max(
				bounds.max.x - bounds.min.x, std::max(
				bounds.max.y - bounds.min.y,
				bounds.max.z - bounds.min.z));
		// Add mesh indexes to the combo box, select "0" by default
		ui_.meshIndexCombo->clear();
		ui_.meshIndexCombo->addItem(QString::fromStdString(REGEN_STRING(-1)));
		for (uint32_t i = 0; i < meshes_.size(); ++i) {
			ui_.meshIndexCombo->addItem(QString::fromStdString(REGEN_STRING(i)));
		}
		ui_.meshIndexCombo->setEnabled(true);
		ui_.meshIndexCombo->setCurrentIndex(0);

		// load the mesh materials
		for (auto &mesh: meshes_) {
			auto material = asset_->getMeshMaterial(mesh.get());
			if (material.get() != nullptr) {
				mesh->setMaterial(material);
			}
		}
		transformMesh(0.0f);

		if (simplify) {
			auto lod0 = static_cast<float>(ui_.simplify0Spin->value());
			auto lod1 = static_cast<float>(ui_.simplify1Spin->value());
			auto lod2 = static_cast<float>(ui_.simplify2Spin->value());
			auto lod3 = static_cast<float>(ui_.simplify3Spin->value());
			simplifyMesh_GL(Vec4f(
					lod0,
					lod1 < lod0 ? lod1 : 0.0f,
					lod2 < lod1 ? lod2 : 0.0f,
					lod3 < lod2 ? lod3 : 0.0f));
		}
		if (useBillboard) {
			billboardMeshes_GL();
		}
		loadResources_GL();
		updateLoDButtons();
		selectMesh(-1, 0);
	}
}

void MeshViewerWidget::loadResources_GL() {
	uint32_t index = 0;
	for (auto &mesh: meshes_) {
		// load bone animation, if any
		if (ui_.animateCheck->isChecked()) {
			loadAnimation(mesh, index++);
		}

		auto shaderState = ref_ptr<ShaderState>::alloc();
		mesh->joinStates(shaderState);
		auto meshNode = ref_ptr<StateNode>::alloc(mesh);
		meshNode->set_isHidden(true);
		meshRoot_->addChild(meshNode);
		meshRoot_->state()->shaderDefine("OUTPUT_TYPE", "DIRECT");
		meshNodes_.push_back(meshNode);
		mesh->createShader(meshRoot_);
	}
}

void MeshViewerWidget::loadAnimation(const ref_ptr<Mesh> &mesh, uint32_t index) {
	std::vector<ref_ptr<BoneNode> > meshBones;
	uint32_t numBoneWeights = asset_->numBoneWeights(mesh.get());

	// Find bones influencing this mesh
	auto nodeAnim = asset_->getNodeAnimation();
	auto boneNodes = asset_->loadMeshBones(mesh.get(), nodeAnim.get());
	meshBones.insert(meshBones.end(), boneNodes.begin(), boneNodes.end());
	nodeAnim->startAnimation();

	// Create Bones state that is responsible for uploading animation data to GL.
	if (!meshBones.empty()) {
		ref_ptr<Bones> bonesState = ref_ptr<Bones>::alloc(nodeAnim, meshBones, numBoneWeights);
		bonesState->setAnimationName(REGEN_STRING("bones-mesh-viewer-" << index));
		bonesState->startAnimation();
		mesh->joinStates(bonesState);
	}

	ref_ptr<EventHandler> animStopped = ref_ptr<RandomAnimationRangeUpdater>::alloc(nodeAnim);
	nodeAnim->connect(Animation::ANIMATION_STOPPED, animStopped);
	{
		EventData evData;
		evData.eventID = Animation::ANIMATION_STOPPED;
		animStopped->call(nodeAnim.get(), &evData);
	}
}

void MeshViewerWidget::selectMesh_(uint32_t meshIndex, uint32_t lodIndex) {
	auto &mesh = meshes_[meshIndex];
	auto numLODs = mesh->numLODs();
	if (numLODs > 1) {
		if (lodIndex < numLODs) {
			mesh->activateLOD(lodIndex);
		} else {
			REGEN_WARN("Invalid LOD index " << lodIndex << " for mesh " << meshIndex);
			mesh->activateLOD(0);
		}
	}
	meshNodes_[meshIndex]->set_isHidden(false);
}

void MeshViewerWidget::selectMesh(int32_t meshIndex, uint32_t lodIndex) {
	for (auto &meshNode: meshNodes_) {
		meshNode->set_isHidden(true);
	}
	if (meshIndex < 0) {
		for (uint32_t i = 0u; i < meshNodes_.size(); ++i) {
			selectMesh_(i, lodIndex);
		}
	} else if (meshIndex < static_cast<int32_t>(meshNodes_.size())) {
			selectMesh_(meshIndex, lodIndex);
	}
	currentMeshIndex_ = meshIndex;
	currentLodLevel_ = lodIndex;
}

static ref_ptr<Camera> createUserCamera(const Vec2i &viewport) {
	auto cam = ref_ptr<Camera>::alloc(1);
	float aspect = (float) viewport.x / (float) viewport.y;
	cam->setPosition(0, Vec3f(0.0f, 0.0f, -3.0f));
	cam->setDirection(0, Vec3f(0.0f, 0.0f, 1.0f));
	cam->setPerspective(aspect, 45.0f, 0.1f, 100.0f);
	cam->updateCamera();
	cam->updateShaderData(0.0f);
	return cam;
}

void MeshViewerWidget::createCameraController() {
	cameraController_ = ref_ptr<CameraController>::alloc(userCamera_);
	cameraController_->set_moveAmount(0.01f);
	cameraController_->setHorizontalOrientation(0.0);
	cameraController_->setVerticalOrientation(0.0);
	cameraController_->setCameraMode(CameraController::FIRST_PERSON);
	cameraController_->initCameraController();
	cameraController_->startAnimation();

	std::vector<CameraCommandMapping> keyMappings;
	ref_ptr<QtCameraEventHandler> cameraEventHandler =
			ref_ptr<QtCameraEventHandler>::alloc(cameraController_, keyMappings);
	cameraEventHandler->set_sensitivity(0.005f);
	app_->connect(Scene::KEY_EVENT, cameraEventHandler);
	app_->connect(Scene::BUTTON_EVENT, cameraEventHandler);
	app_->connect(Scene::MOUSE_MOTION_EVENT, cameraEventHandler);
}

void MeshViewerWidget::gl_loadScene() {
	AnimationManager::get().pause(true);
	AnimationManager::get().setRootState(app_->renderTree()->state());

	// create render target
	auto fbo = ref_ptr<FBO>::alloc(ui_.glWidget->width(), ui_.glWidget->height());
	fbo->addTexture(1,
		GL_TEXTURE_2D,
		GL_RGBA,
		GL_RGBA8,
		GL_UNSIGNED_BYTE,
		4);
	fbo->createDepthTexture(
		GL_TEXTURE_2D,
		GL_DEPTH_COMPONENT24,
		GL_UNSIGNED_BYTE,
		4);
	fbo->checkStatus();

	auto fboState = ref_ptr<FBOState>::alloc(fbo);
	fboState->setClearDepth();
	fboState->setClearColor({
		Vec4f(0.26, 0.26, 0.36, 1.0),
		GL_COLOR_ATTACHMENT0 });
	fboState->setDrawBuffers({GL_COLOR_ATTACHMENT0});

	// create a root node
	sceneRoot_->state()->joinStates(fboState);
	app_->renderTree()->addChild(sceneRoot_);

	// enable user camera
	userCamera_ = createUserCamera(app_->screen()->viewport().r);
	sceneRoot_->state()->joinStates(userCamera_);

	// enable model transformation
	modelTransform_ = ref_ptr<ModelTransformation>::alloc();
	modelTransform_->setModelMat(0, Mat4f::identity());
	modelTransform_->tfBuffer()->updateBuffer();
	sceneRoot_->state()->joinStates(modelTransform_);

	// enable wireframe mode
	wireframeState_ = ref_ptr<FillModeState>::alloc(GL_FILL);
	sceneRoot_->state()->joinStates(wireframeState_);

	// setup MSAA + alpha coverage for mesh rendering
	sceneRoot_->state()->joinStates(ref_ptr<ToggleState>::alloc(RenderState::BLEND, false));
	sceneRoot_->state()->joinStates(ref_ptr<ToggleState>::alloc(RenderState::MULTISAMPLE, true));
	sceneRoot_->state()->joinStates(ref_ptr<ToggleState>::alloc(RenderState::SAMPLE_ALPHA_TO_COVERAGE, true));

	// enable light, and use it in direct shading
	auto shadingState = ref_ptr<DirectShading>::alloc();
	shadingState->ambientLight()->setVertex(0, Vec3f(0.3f, 0.3f, 0.3f));

	sceneLight_[0] = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
	sceneLight_[0]->setDirection(0, Vec3f(0.0f, 1.0f, 0.0f).normalize());
	sceneLight_[0]->setDiffuse(0, Vec3f(0.3f, 0.3f, 0.3f));
	sceneLight_[0]->setSpecular(0, Vec3f::zero());

	sceneLight_[1] = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
	sceneLight_[1]->setDirection(0, Vec3f(-1.0f, 0.0f, 0.0f).normalize());
	sceneLight_[1]->setDiffuse(0, Vec3f(0.4f, 0.4f, 0.4f));
	sceneLight_[1]->setSpecular(0, Vec3f::zero());

	sceneLight_[2] = ref_ptr<Light>::alloc(Light::DIRECTIONAL);
	sceneLight_[2]->setDirection(0, Vec3f(1.0f, 1.0f, 0.0f).normalize());
	sceneLight_[2]->setDiffuse(0, Vec3f(0.4f, 0.4f, 0.4f));
	sceneLight_[2]->setSpecular(0, Vec3f::zero());

	shadingState->addLight(sceneLight_[0]);
	shadingState->addLight(sceneLight_[1]);
	shadingState->addLight(sceneLight_[2]);
	sceneRoot_->state()->joinStates(shadingState);

	// finally, enable a blit state to copy the framebuffer to the screen
	auto blit = ref_ptr<BlitToScreen>::alloc(
			fbo, app_->screen(), GL_COLOR_ATTACHMENT0);
	// NOTE: must use nearest with MSAA
	blit->set_filterMode(GL_NEAREST);
	sceneRoot_->state()->joinStates(blit);
	GL_ERROR_LOG();

	// resize fbo with window
	app_->connect(Scene::RESIZE_EVENT, ref_ptr<FBOResizer>::alloc(fboState));
	// Update frustum when window size changes
	app_->connect(Scene::RESIZE_EVENT,
				  ref_ptr<ProjectionUpdater>::alloc(userCamera_, app_->screen()));

	AnimationManager::get().resume();
	REGEN_INFO("Scene Loaded.");
	createCameraController();
}

void MeshViewerWidget::transformMesh(double dt) {
	// rotate the mesh around the Y axis
	meshOrientation_ += dt * 0.001f;
	if (meshOrientation_ > M_PI * 2.0f) {
		meshOrientation_ -= M_PI * 2.0f;
	}
	meshQuaternion_.setAxisAngle(Vec3f::up(), meshOrientation_);
	auto mat = meshQuaternion_.calculateMatrix();
	mat.translate(meshOrigin_);
	mat.scale(Vec3f(meshScale_, meshScale_, meshScale_));
	modelTransform_->setModelMat(0, mat);
}

void MeshViewerWidget::toggleInputsDialog() {
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

//////////////////////////////
//////// Slots
//////////////////////////////

void MeshViewerWidget::loadMeshes() {
	app_->withGLContext([this]() {
		loadMeshes_GL(assetFilePath_);
	});
	// store the settings
	for (auto [spin, name]: {
			std::make_pair(ui_.simplify0Spin, "simplify0"),
			std::make_pair(ui_.simplify1Spin, "simplify1"),
			std::make_pair(ui_.simplify2Spin, "simplify2"),
			std::make_pair(ui_.simplify3Spin, "simplify3"),
			std::make_pair(ui_.tpsSpin, "tps"),
			std::make_pair(ui_.norMaxAngleSpin, "norMaxAngle"),
			std::make_pair(ui_.norPenaltySpin, "norPenalty"),
			std::make_pair(ui_.valencePenaltySpin, "valencePenalty"),
			std::make_pair(ui_.areaPenaltySpin, "areaPenalty"),
			std::make_pair(ui_.depthOffsetSpin, "depthOffset")
	}) {
		settings_.setValue(name, spin->value());
	}
	for (auto [spin, name]: {
			std::make_pair(ui_.longitudeSpin, "longitudeSteps"),
			std::make_pair(ui_.latitudeSpin, "latitudeSteps"),
			std::make_pair(ui_.impostorSizeSpin, "impostorSize")
	}) {
		settings_.setValue(name, spin->value());
	}
	for (auto [button, name]: {
			std::make_pair(ui_.animateCarret, "animateCaret"),
			std::make_pair(ui_.simplifyCarret, "simplifyCaret"),
			std::make_pair(ui_.impostorCarret, "impostorCaret"),
	}) {
		settings_.setValue(name, button->isChecked());
	}
	for (auto [check, name]: {
			std::make_pair(ui_.simplifyCheckBox, "simplify"),
			std::make_pair(ui_.genNorCheck, "genNorCheck"),
			std::make_pair(ui_.fixNorCheck, "fixNorCheck"),
			std::make_pair(ui_.genUvCheck, "genUvCheck"),
			std::make_pair(ui_.flipUvCheck, "flipUvCheck"),
			std::make_pair(ui_.deboneCheck, "deboneCheck"),
			std::make_pair(ui_.limitBonesCheck, "limitBonesCheck"),
			std::make_pair(ui_.optimizeMeshesCheck, "optimizeMeshesCheck"),
			std::make_pair(ui_.optimizeGraphCheck, "optimizeGraphCheck"),
			std::make_pair(ui_.animateCheck, "animateCheck"),
			std::make_pair(ui_.strictBoundaryCheck, "strictBoundaryCheck"),
			std::make_pair(ui_.impostorCheck, "impostorCheck"),
			std::make_pair(ui_.hemisphericalCheck, "hemisphericalCheck"),
			std::make_pair(ui_.topViewCheck, "topViewCheck"),
			std::make_pair(ui_.bottomViewCheck, "bottomViewCheck"),
			std::make_pair(ui_.depthCorrectionCheck, "depthCorrectionCheck"),
			std::make_pair(ui_.normalCorrectionCheck, "normalCorrectionCheck"),
	}) {
		settings_.setValue(name, check->isChecked());
	}
	settings_.setValue("lastPath", QString::fromStdString(assetFilePath_));
	settings_.sync();
}

void MeshViewerWidget::openAssetFile() {
	QFileDialog dialog(nullptr);
	dialog.setFileMode(QFileDialog::AnyFile);
	dialog.setViewMode(QFileDialog::Detail);
	if (!dialog.exec()) { return; }
	auto fileNames = dialog.selectedFiles();
	assetFilePath_ = fileNames.first().toStdString();
	auto assetFilename = boost::filesystem::path(assetFilePath_).filename().string();
	ui_.assetNameEdit->setText(QString::fromStdString(assetFilename));
	ui_.loadMeshButton->setEnabled(true);
}

void MeshViewerWidget::activateLoD(int lodLevel, const ref_ptr<Mesh> &mesh) {
	auto numLODs = mesh->numLODs();
	if (static_cast<uint32_t>(lodLevel) > numLODs) {
		REGEN_WARN("Invalid LOD level " << lodLevel);
		lodLevel = 0;
	}
	mesh->activateLOD(static_cast<uint32_t>(lodLevel));
}

void MeshViewerWidget::activateLoD(int lodLevel) {
	if (meshes_.empty()) {
		REGEN_WARN("No meshes loaded.");
		return;
	}
	if (currentMeshIndex_ == -1) {
		for (auto &mesh: meshes_) {
			activateLoD(lodLevel, mesh);
		}
	} else {
		activateLoD(lodLevel, meshes_[currentMeshIndex_]);
	}
}

void MeshViewerWidget::activateLoD_0() {
	activateLoD(0);
}

void MeshViewerWidget::activateLoD_1() {
	activateLoD(1);
}

void MeshViewerWidget::activateLoD_2() {
	activateLoD(2);
}

void MeshViewerWidget::activateLoD_3() {
	activateLoD(3);
}

void MeshViewerWidget::activateMeshIndex(int index) {
	selectMesh(index-1, currentLodLevel_);
}

void MeshViewerWidget::updateSize() {
	auto w = ui_.blackBackground->width();
	auto h = ui_.blackBackground->height();
	ui_.glWidget->setMinimumSize(QSize(max(2, w), max(2, h)));
}

void MeshViewerWidget::toggleControls() {
	controlsShown_ = !controlsShown_;
	if (controlsShown_) {
		ui_.buttonFrameBar->show();
		ui_.statusbar->show();

		hideLayout(fullscreenLayout_);
		ui_.splitter->addWidget(ui_.blackBackground);
		ui_.splitter->addWidget(ui_.controls);
		showLayout(ui_.mainLayout);
		ui_.blackBackground->show();
		ui_.splitter->setSizes(splitterSizes_);
	} else {
		splitterSizes_ = ui_.splitter->sizes();

		ui_.buttonFrameBar->hide();
		ui_.statusbar->hide();

		hideLayout(ui_.mainLayout);
		fullscreenLayout_->addWidget(ui_.blackBackground);
		showLayout(fullscreenLayout_);
		ui_.blackBackground->show();
	}
}

void MeshViewerWidget::toggleFullscreen() {
	app_->toggleFullscreen();
	// hide controls when the window is fullscreen
	if (isFullScreen()) {
		wereControlsShown_ = controlsShown_;
		if (controlsShown_) toggleControls();
	} else {
		if (wereControlsShown_) toggleControls();
	}
}

void MeshViewerWidget::toggleWireframe(bool isEnabled) {
	wireframeState_->set_fillMode(isEnabled ? GL_LINE : GL_FILL);
}

void MeshViewerWidget::toggleRotate(bool isEnabled) {
	if (isEnabled) {
		rotateAnim_ = ref_ptr<RotateAnimation>::alloc(this);
		rotateAnim_->startAnimation();
	} else {
		rotateAnim_->stopAnimation();
	}
}

void MeshViewerWidget::setAssImpFlags() {
	asset_->setAiProcessFlag(aiProcess_GenSmoothNormals);
	if (ui_.genNorCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_ForceGenNormals);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_ForceGenNormals);
	}
	if (ui_.fixNorCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_FixInfacingNormals);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_FixInfacingNormals);
	}
	if (ui_.genUvCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_GenUVCoords);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_GenUVCoords);
	}
	if (ui_.flipUvCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_FlipUVs);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_FlipUVs);
	}
	if (ui_.deboneCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_Debone);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_Debone);
	}
	if (ui_.limitBonesCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_LimitBoneWeights);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_LimitBoneWeights);
	}
	if (ui_.optimizeMeshesCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_OptimizeMeshes);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_OptimizeMeshes);
	}
	if (ui_.optimizeGraphCheck->checkState() == Qt::Checked) {
		asset_->setAiProcessFlag(aiProcess_OptimizeGraph);
	} else {
		asset_->unsetAiProcessFlag(aiProcess_OptimizeGraph);
	}
}
