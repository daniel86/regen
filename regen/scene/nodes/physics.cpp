#include "physics.h"
#include "regen/physics/bullet-debug-drawer.h"
#include "regen/physics/character-controller.h"
#include <bullet/BulletCollision/CollisionDispatch/btGhostObject.h>
#include <bullet/BulletDynamics/Character/btKinematicCharacterController.h>

using namespace regen::scene;
using namespace regen;

static ref_ptr<ShaderInput> getMeshPositions(
		SceneParser *parser, SceneInputNode &input) {
	ref_ptr<MeshVector> meshes = parser->getResources()->getMesh(parser, input.getValue("mesh"));
	ref_ptr<ShaderInput> out;

	if (meshes.get() == nullptr || meshes->empty()) {
		REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
	} else if (meshes->size() > 1) {
		REGEN_WARN("Unable to handle multiple Meshes for '" << input.getDescription() << "'.");
	} else {
		out = (*meshes->begin())->positions();
	}
	return out;
}

static ref_ptr<Mesh> getMesh(
		SceneParser *parser, SceneInputNode &input) {
	ref_ptr<MeshVector> meshes = parser->getResources()->getMesh(parser, input.getValue("mesh-id"));
	ref_ptr<Mesh> out;

	if (meshes.get() == nullptr || meshes->empty()) {
		REGEN_WARN("Unable to find Mesh for '" << input.getDescription() << "'.");
	} else if (meshes->size() > 1) {
		REGEN_WARN("Unable to handle multiple Meshes for '" << input.getDescription() << "'.");
	} else {
		out = (*meshes->begin());
	}
	return out;
}

static ref_ptr<PhysicalProps> createPhysicalProps(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<btMotionState> &motion,
		const ref_ptr<ModelTransformation> &transform) {
	const std::string shapeName(input.getValue("shape"));
	auto mass = input.getValue<GLfloat>("mass", 1.0f);

	ref_ptr<PhysicalProps> props;
	// Primitives
	// TODO: Support characters. bullet has something special fo them
	// where a primitive is attached to a capsule in order to avoid
	// problems with collisions
	if (shapeName == "sphere") {
		GLfloat radius = input.getValue<GLfloat>("radius", 1.0f) * 0.5;
		props = ref_ptr<PhysicalProps>::alloc(
				motion, ref_ptr<btSphereShape>::alloc(radius));
	} else if (shapeName == "wall") {
		auto size = input.getValue<Vec2f>("size", Vec2f(1.0f));
		// TODO: allow configuration of orientation and position
		btVector3 halfExtend(size.x * 0.5, 0.001, size.y * 0.5);
		props = ref_ptr<PhysicalProps>::alloc(
				motion, ref_ptr<btBoxShape>::alloc(halfExtend));
		mass = 0.0;
	} else if (shapeName == "infinite-wall") {
		auto planeNormal = input.getValue<Vec3f>("normal", Vec3f(0.0f, 1.0f, 0.0f));
		auto planeConstant = input.getValue<GLfloat>("constant", GLfloat(0.0f));
		btVector3 planeNormal_(planeNormal.x, planeNormal.y, planeNormal.z);
		props = ref_ptr<PhysicalProps>::alloc(
				motion, ref_ptr<btStaticPlaneShape>::alloc(planeNormal_, planeConstant));
		mass = 0.0;
	} else if (shapeName == "box") {
		auto size = input.getValue<Vec3f>("size", Vec3f(1.0f));
		btVector3 halfExtend(size.x * 0.5, size.y * 0.5, size.z * 0.5);
		props = ref_ptr<PhysicalProps>::alloc(
				motion, ref_ptr<btBoxShape>::alloc(halfExtend));
	} else if (shapeName == "cylinder") {
		auto size = input.getValue<Vec3f>("size", Vec3f(1.0f));
		btVector3 halfExtend(size.x * 0.5, size.y * 0.5, size.z * 0.5);
		props = ref_ptr<PhysicalProps>::alloc(
				motion, ref_ptr<btCylinderShape>::alloc(halfExtend));
	} else if (shapeName == "capsule") {
		auto radius = input.getValue<GLfloat>("radius", 1.0f);
		auto height = input.getValue<GLfloat>("height", 1.0f);
		props = ref_ptr<PhysicalProps>::alloc(
				motion, ref_ptr<btCapsuleShape>::alloc(radius, height));
	} else if (shapeName == "cone") {
		auto radius = input.getValue<GLfloat>("radius", 1.0f);
		auto height = input.getValue<GLfloat>("height", 1.0f);
		props = ref_ptr<PhysicalProps>::alloc(
				motion, ref_ptr<btConeShape>::alloc(radius, height));
	} else if (shapeName == "character") {
		auto radius = input.getValue<GLfloat>("radius", 1.0f);
		auto height = input.getValue<GLfloat>("height", 1.0f);
		auto stepHeight = input.getValue<GLfloat>("step-height", 0.35f);
		// Create the capsule shape
		ref_ptr<btCapsuleShape> capsuleShape = ref_ptr<btCapsuleShape>::alloc(radius, height);
		// Create the ghost object
		ref_ptr<btPairCachingGhostObject> ghostObject = ref_ptr<btPairCachingGhostObject>::alloc();
		ghostObject->setWorldTransform(btTransform(
				btQuaternion(0, 0, 0, 1),
				btVector3(0, 0, 0)));
		ghostObject->setCollisionShape(capsuleShape.get());
		ghostObject->setCollisionFlags(btCollisionObject::CF_CHARACTER_OBJECT);
		// Create the character controller
		auto characterController = ref_ptr<CharacterController>::alloc(
				ghostObject.get(), capsuleShape.get(), stepHeight);
		// Set gravity for the character controller
		characterController->setGravity(btVector3(0, -9.81, 0));
		// Ensure no unintended forces
		characterController->setLinearVelocity(btVector3(0, 0, 0));
		characterController->setAngularVelocity(btVector3(0, 0, 0));
		// Create PhysicalProps for the character
		props = ref_ptr<PhysicalProps>::alloc(motion, capsuleShape);
		//props->setCharacterController(characterController);
		//props->addCollisionObject(ghostObject);
	} else if (shapeName == "convex-hull") {
		ref_ptr<ShaderInput> pos = getMeshPositions(parser, input);
		if (pos.get() != nullptr) {
			if (!pos->hasClientData() && !pos->hasServerData()) {
				REGEN_WARN("Mesh '" << input.getValue("mesh") <<
									"' has no position data available.");
			} else {
				bool loadServerData = !pos->hasClientData();
				if (loadServerData) pos->readServerData();

				const btScalar *points = (const btScalar *) pos->clientData();
				//create a hull approximation
				/*
				btShapeHull* hull = new btShapeHull(originalConvexShape);
				btScalar margin = originalConvexShape->getMargin();
				hull->buildHull(margin);
				btConvexHullShape* simplifiedConvexShape = new btConvexHullShape(hull->getVertexPointer(),hull->numVertices());
				*/

				props = ref_ptr<PhysicalProps>::alloc(
						motion, ref_ptr<btConvexHullShape>::alloc(points, pos->numVertices()));

				if (loadServerData) pos->deallocateClientData();
			}
		}
	} else if (shapeName == "triangle-mesh") {
		ref_ptr<Mesh> mesh = getMesh(parser, input);
		ref_ptr<ShaderInput> pos = (mesh.get() == nullptr ?
									ref_ptr<ShaderInput>() : mesh->positions());
		ref_ptr<ShaderInput> indices = (mesh.get() == nullptr ?
										ref_ptr<ShaderInput>() : mesh->inputContainer()->indices());

		if (pos.get() != nullptr && indices.get() != nullptr) {
			btIndexedMesh btMesh;
			btMesh.m_numVertices = pos->numVertices();
			btMesh.m_vertexStride = pos->elementSize();

			switch (mesh->primitive()) {
				case GL_TRIANGLES:
					btMesh.m_triangleIndexStride = 3 * indices->elementSize();
					btMesh.m_numTriangles = indices->numVertices() / 3;
					break;
				case GL_TRIANGLE_STRIP:
					btMesh.m_triangleIndexStride = 1 * indices->elementSize();
					btMesh.m_numTriangles = indices->numVertices() - 2;
					break;
				default:
					btMesh.m_numTriangles = -1;
					btMesh.m_triangleIndexStride = -1;
					break;
			}

			if (btMesh.m_numTriangles > 0) {
				if (!pos->hasClientData()) pos->readServerData();
				if (!indices->hasClientData()) indices->readServerData();
				btMesh.m_vertexBase = pos->clientData();
				btMesh.m_triangleIndexBase = indices->clientData();

				PHY_ScalarType indexType;
				switch (indices->dataType()) {
					case GL_FLOAT:
						indexType = PHY_FLOAT;
						break;
					case GL_DOUBLE:
						indexType = PHY_DOUBLE;
						break;
					case GL_SHORT:
						indexType = PHY_SHORT;
						break;
					case GL_INT:
					case GL_UNSIGNED_INT:
					default:
						indexType = PHY_INTEGER;
						break;
				}

				const bool useQuantizedAabbCompression = true;

				auto *btMeshIface = new btTriangleIndexVertexArray;
				btMeshIface->addIndexedMesh(btMesh, indexType);
				ref_ptr<btBvhTriangleMeshShape> shape =
						ref_ptr<btBvhTriangleMeshShape>::alloc(btMeshIface, useQuantizedAabbCompression);
				props = ref_ptr<PhysicalProps>::alloc(motion, shape);
			} else {
				REGEN_WARN("Unsupported primitive for btTriangleIndexVertexArray "
								   << "in input node " << input.getDescription() << "'.");
			}
		} else if (indices.get() == nullptr) {
			REGEN_WARN(
					"Ignoring physical shape for '" << input.getDescription() << "'. Mesh has no Indices.");
			return {};
		} else if (pos.get() == nullptr) {
			REGEN_WARN("Ignoring physical shape for '" << input.getDescription()
													   << "'. Mesh has no Positions.");
			return {};
		}
	}
	if (props.get() == nullptr) {
		REGEN_WARN("Ignoring unknown physical shape '" << input.getDescription() << "'.");
		return {};
	}

	auto inertia = input.getValue<Vec3f>("inertia", Vec3f(0.0f));
	props->setMassProps(mass, btVector3(inertia.x, inertia.x, inertia.z));

	props->setRestitution(
			input.getValue<GLfloat>("restitution", 0.0f));

	props->setLinearSleepingThreshold(
			input.getValue<GLfloat>("linear-sleeping-threshold", 0.8f));
	props->setAngularSleepingThreshold(
			input.getValue<GLfloat>("angular-sleeping-threshold", 1.0f));

	props->setFriction(
			input.getValue<GLfloat>("friction", 0.5f));
	props->setRollingFriction(
			input.getValue<GLfloat>("rolling-friction", 0.0f));

	props->setAdditionalDamping(
			input.getValue<bool>("additional-damping", false));
	props->setAdditionalDampingFactor(
			input.getValue<GLfloat>("additional-damping-factor", 0.005f));

	props->setLinearDamping(
			input.getValue<GLfloat>("linear-damping", 0.0f));
	props->setAdditionalLinearDampingThresholdSqr(
			input.getValue<GLfloat>("additional-linear-damping-threshold", 0.01f));

	props->setAngularDamping(
			input.getValue<GLfloat>("angular-damping", 0.0f));
	props->setAdditionalAngularDampingFactor(
			input.getValue<GLfloat>("additional-angular-damping-factor", 0.01f));
	props->setAdditionalAngularDampingThresholdSqr(
			input.getValue<GLfloat>("additional-angular-damping-threshold", 0.01f));

	if (mass > 0) props->calculateLocalInertia();

	return props;
}

void PhysicsStateProvider::processInput(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<State> &parent) {
	auto meshID = input.getValue("mesh-id");
	auto shapeName = input.getValue("shape");
	auto meshVector = parser->getResources()->getMesh(parser, meshID);
	if (meshVector.get() == nullptr) {
		REGEN_WARN("Skipping physics node with unknown mesh ID '" << meshID << "'.");
		return;
	}
	auto transformID = input.getValue("transform-id");
	auto transform = parser->getResources()->getTransform(parser, transformID);
	if (transform.get() == nullptr) {
		REGEN_WARN("Skipping physics node with unknown transform ID '" << transformID << "'.");
		return;
	}

	auto meshIndex = input.getValue<int>("mesh-index", 0);
	auto mesh = (*meshVector->begin());
	if (meshIndex > 0 && meshIndex < meshVector->size()) {
		mesh = (*meshVector.get())[meshIndex];
	}

	auto numInstances = input.getValue<GLuint>("num-instances", 1u);
	for (GLuint i = 0; i < numInstances; ++i) {
		ref_ptr<ModelMatrixMotion> motion;
		if (shapeName == "character") {
			motion = ref_ptr<CharacterMotion>::alloc(transform->get(), i);
		} else {
			motion = ref_ptr<ModelMatrixMotion>::alloc(transform->get(), i);
		}
		auto physicalProps = createPhysicalProps(parser, input, motion, transform);
		auto physicalObject = ref_ptr<PhysicalObject>::alloc(physicalProps);

		mesh->addPhysicalObject(physicalObject);
		parser->getPhysics()->addObject(physicalObject);
	}
}

void BulletDebuggerProvider::processInput(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parent) {
	auto &physics = parser->getPhysics();
	auto debugDrawer = ref_ptr<BulletDebugDrawer>::alloc(physics);
	debugDrawer->setDebugMode(btIDebugDraw::DBG_DrawWireframe);
	physics->dynamicsWorld()->setDebugDrawer(debugDrawer.get());
	parent->addChild(debugDrawer);

	StateConfig shaderConfig = StateConfigurer::configure(debugDrawer.get());
	shaderConfig.setVersion(330);
	debugDrawer->createShader(shaderConfig);
}

