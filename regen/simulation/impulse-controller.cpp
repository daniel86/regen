#include "impulse-controller.h"
#include "model-matrix-motion.h"
#include "regen/scene/resource-manager.h"

using namespace regen;

ImpulseController::ImpulseController(
			const ref_ptr<Camera> &cam,
			const ref_ptr<PhysicalObject> &physicalObject) :
		CameraController(cam),
		physicalObject_(physicalObject),
		physicsSpeedFactor_(1.0) {
	// read transform from physical object
	btTransform transform;
	physicalObject->motionState()->getWorldTransform(transform);
	// create a custom motion state where we can do synchronization with
	// updating the camera transform which depends on the physical object.
	auto motionState = ref_ptr<Mat4fMotion>::alloc(&matVal_);
	transform.getOpenGLMatrix((btScalar *) matVal_.x);
	physicalObject->setMotionState(motionState);
	physicalObject->rigidBody()->setLinearVelocity(btVector3(0, 0, 0));
}

void ImpulseController::applyStep(float dt, const Vec3f &offset) {
	// update orientation of the physical object: set it to the camera orientation
	btTransform transform;
	btQuaternion rotation;
	physicalObject_->rigidBody()->getMotionState()->getWorldTransform(transform);
	rotation.setRotation(btVector3(0, 1, 0), horizontalOrientation_);
	transform.setRotation(rotation);
	physicalObject_->rigidBody()->getMotionState()->setWorldTransform(transform);

	transform.getOpenGLMatrix((btScalar *) &matVal_);

	if (!isMoving_) return;

	// apply impulse to the physical object
	btVector3 impulse(
		step_.x * physicsSpeedFactor_,
		step_.y * physicsSpeedFactor_,
		step_.z * physicsSpeedFactor_);
	physicalObject_->rigidBody()->activate(true);
	physicalObject_->rigidBody()->applyCentralImpulse(impulse);
}

ref_ptr<ImpulseController> ImpulseController::load(
		LoadingContext &ctx, scene::SceneInputNode &node,
		const ref_ptr<Camera> &userCamera) {
	auto controllerMode = node.getValue<std::string>("mode", "first-person");
	ref_ptr<ImpulseController> controller;
	auto *scene = ctx.scene();

	ref_ptr<Mesh> mesh;
	auto compositeMesh = scene->getResources()->getMesh(scene, node.getValue("mesh"));
	if (compositeMesh.get() != nullptr && !compositeMesh->meshes().empty()) {
		auto meshIndex = node.getValue<GLuint>("mesh-index", 0u);
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

	if (mesh->physicalObjects().empty()) {
		REGEN_WARN("Impulse controller requires a physical object.");
		return {};
	}
	auto &physicalObject = mesh->physicalObjects().front();

	auto impulseController =
		ref_ptr<ImpulseController>::alloc(userCamera, physicalObject);
	controller = impulseController;
	controller->setMeshEyeOffset(node.getValue<Vec3f>("eye-offset", Vec3f(0.0)));
	controller->set_moveAmount(node.getValue<GLfloat>("speed", 0.01f));
	controller->setMeshDistance(node.getValue<GLfloat>("mesh-distance", 10.0f));
	controller->setHorizontalOrientation(node.getValue<GLfloat>("horizontal-orientation", 0.0));
	controller->setVerticalOrientation(node.getValue<GLfloat>("vertical-orientation", 0.0));
	controller->setMeshHorizontalOrientation(node.getValue<GLfloat>("mesh-horizontal-orientation", 0.0));
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
