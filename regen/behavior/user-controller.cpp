#include "user-controller.h"

#include "regen/simulation/kinematic-controller.h"
#include "applications/scene-display/animation-events.h"
#include "regen/scene/resource-manager.h"
#include "regen/simulation/impulse-controller.h"

using namespace regen;

UserController::UserController(const ref_ptr<Camera> &cam) : CameraController(cam) {
}

void UserController::setBoneTree(const ref_ptr<BoneAnimationItem> &boneAnimation) {
	boneAnimation_ = boneAnimation;
	boneController_ = ref_ptr<BoneController>::alloc(0, boneAnimation_);
	// Build motion type to index map.
	uint32_t numAgentMotions = 0;
	for (const auto &clip: boneAnimation_->clips) {
		if (motionIndices_.find(clip.motion) == motionIndices_.end()) {
			motionIndices_[clip.motion] = numAgentMotions++;
			motionTypes_.push_back(clip.motion);
		}
	}
	// Initialize motion atomic flag array.
	motionState_ = std::vector<std::atomic<bool>>(numAgentMotions);
	for (auto &flag: motionState_) {
		flag.store(false);
	}
	// Make enough space for active motions.
	activeMotions_.resize(numAgentMotions);
}

void UserController::setMotionActive(MotionType type, bool active, bool reverse) {
	auto it = motionIndices_.find(type);
	if (it == motionIndices_.end()) return;
	auto &flag = motionState_[it->second];
	if (active != flag.load()) {
		flag.store(active, std::memory_order_relaxed);
	}
}

bool UserController::isMotionActive(MotionType type) const {
	const int motionIndex = motionIndices_.at(type);
	return motionState_[motionIndex].load();
}

void UserController::cpuUpdate(double dt) {
	CameraController::cpuUpdate(dt);
	if (boneController_.get()) {
		float dt_s = dt / 1000.0f;
		// Update the list of active motions from the motion state array.
		numActiveMotions_ = 0;
		for (uint32_t motionIdx = 0; motionIdx < motionTypes_.size(); ++motionIdx) {
			auto &motionFlag = motionState_[motionIdx];
			if (motionFlag.load()) {
				activeMotions_[numActiveMotions_++] = motionTypes_[motionIdx];
			}
		}
		if (numActiveMotions_ == 0) {
			// Ensure at least idle motion is active.
			activeMotions_[numActiveMotions_++] = MotionType::IDLE;
		}
		// Update the bone controller with the desired motions.
		boneController_->updateBoneController(dt_s,
			activeMotions_.data(), numActiveMotions_);
	}
}

ref_ptr<UserController> UserController::load(
		LoadingContext &ctx, scene::SceneInputNode &node,
		const ref_ptr<Camera> &userCamera) {
	auto controllerMode = node.getValue<std::string>("mode", "first-person");
	auto controllerType = node.getValue<std::string>("type", "default");
	ref_ptr<UserController> controller;
	auto *scene = ctx.scene();

	ref_ptr<Mesh> mesh;
	auto compositeMesh = scene->getResources()->getMesh(scene, node.getValue("mesh"));
	if (compositeMesh.get() != nullptr && !compositeMesh->meshes().empty()) {
		auto meshIndex = node.getValue<uint32_t>("mesh-index", 0u);
		if (meshIndex >= compositeMesh->meshes().size()) {
			REGEN_WARN("Invalid mesh index for '" << node.getDescription() << "'.");
			meshIndex = 0;
		}
		mesh = compositeMesh->meshes()[meshIndex];
	}
	if (mesh.get() == nullptr) {
		REGEN_WARN("Unable to find mesh for PlayerController in '" << node.getDescription() << "'.");
		return controller;
	}

	ref_ptr<ModelTransformation> tf;
	if (node.hasAttribute("transform")) {
		tf = scene->getResources()->getTransform(scene, node.getValue("transform"));
	} else if (node.hasAttribute("tf")) {
		tf = scene->getResources()->getTransform(scene, node.getValue("tf"));
	}
	if (tf.get() == nullptr) {
		REGEN_WARN("Unable to find transform for '" << node.getDescription() << "'.");
		return controller;
	}

	auto animAssetName = node.getValue("animation-asset");
	auto animItem = scene->getAnimationRanges(animAssetName);
	if (node.hasAttribute("animation-asset") && !animItem) {
		REGEN_WARN("Unable to find animation asset for animation with name '"
			<< animAssetName << "' in controller '" << node.getDescription() << "'.");
	}

	if (controllerType == "user-kinematic") {
		if (scene->getPhysics().get() == nullptr) {
			REGEN_WARN("No physics engine available for PlayerController in '" << node.getDescription() << "'.");
			return controller;
		}
		auto characterController =
				ref_ptr<KinematicPlayerController>::alloc(userCamera, scene->getPhysics());
		characterController->setCollisionHeight(node.getValue<float>("collision-height", 0.8));
		characterController->setCollisionRadius(node.getValue<float>("collision-radius", 0.8));
		characterController->setStepHeight(node.getValue<float>("step-height", 0.35));
		characterController->setMaxSlope(node.getValue<float>("max-slope", 0.8));
		characterController->setGravityForce(node.getValue<float>("gravity-force", 30.0));
		characterController->setJumpVelocity(node.getValue<float>("jump-velocity", 16.0f));
		if (animItem.get()) {
			characterController->setBoneTree(animItem);
		}
		controller = characterController;
	} else {
		REGEN_WARN("Unknown player controller type '" << controllerType
			<< "' in '" << node.getDescription() << "'. Using default PlayerController.");
		return controller;
	}

	controller->setMeshEyeOffset(node.getValue<Vec3f>("eye-offset", Vec3f::zero()));
	controller->set_moveAmount(node.getValue<float>("speed", 0.01f));
	controller->setMeshDistance(node.getValue<float>("mesh-distance", 10.0f));
	controller->setHorizontalOrientation(node.getValue<float>("horizontal-orientation", 0.0));
	controller->setVerticalOrientation(node.getValue<float>("vertical-orientation", 0.0));
	controller->setMeshHorizontalOrientation(node.getValue<float>("mesh-horizontal-orientation", 0.0));
	if (controllerMode == "third-person") {
		controller->setCameraMode(CameraController::THIRD_PERSON);
	} else {
		controller->setCameraMode(CameraController::FIRST_PERSON);
	}
	// attach the camera to a transform
	if (tf.get()) {
		controller->setAttachedTo(tf, mesh);
	}

	return controller;
}
