#include "physics.h"
#include "regen/physics/bullet-debug-drawer.h"
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

static ref_ptr<btCollisionShape> createSphere([[maybe_unused]] SceneParser *parser, SceneInputNode &input) {
	auto radius = input.getValue<GLfloat>("radius", 1.0f) * 0.5;
	return ref_ptr<btSphereShape>::alloc(radius);
}

static ref_ptr<btCollisionShape> createWall([[maybe_unused]] SceneParser *parser, SceneInputNode &input) {
	auto size = input.getValue<Vec2f>("size", Vec2f(1.0f));
	// TODO: allow configuration of orientation and position
	btVector3 halfExtend(size.x * 0.5f, 0.001f, size.y * 0.5f);
	return ref_ptr<btBoxShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createInfiniteWall([[maybe_unused]] SceneParser *parser, SceneInputNode &input) {
	auto planeNormal = input.getValue<Vec3f>("normal", Vec3f(0.0f, 1.0f, 0.0f));
	auto planeConstant = input.getValue<GLfloat>("constant", GLfloat(0.0f));
	return ref_ptr<btStaticPlaneShape>::alloc(
			btVector3(planeNormal.x, planeNormal.y, planeNormal.z),
			planeConstant);
}

static ref_ptr<btCollisionShape> createBox([[maybe_unused]] SceneParser *parser, SceneInputNode &input) {
	auto size = input.getValue<Vec3f>("size", Vec3f(1.0f));
	btVector3 halfExtend(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
	return ref_ptr<btBoxShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createCylinder([[maybe_unused]] SceneParser *parser, SceneInputNode &input) {
	auto size = input.getValue<Vec3f>("size", Vec3f(1.0f));
	btVector3 halfExtend(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
	return ref_ptr<btCylinderShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createCapsule([[maybe_unused]] SceneParser *parser, SceneInputNode &input) {
	return ref_ptr<btCapsuleShape>::alloc(
			input.getValue<GLfloat>("radius", 1.0f),
			input.getValue<GLfloat>("height", 1.0f));
}

static ref_ptr<btCollisionShape> createCone([[maybe_unused]] SceneParser *parser, SceneInputNode &input) {
	return ref_ptr<btConeShape>::alloc(
			input.getValue<GLfloat>("radius", 1.0f),
			input.getValue<GLfloat>("height", 1.0f));
}

static ref_ptr<btCollisionShape> createConvexHull(SceneParser *parser, SceneInputNode &input) {
	ref_ptr<ShaderInput> pos = getMeshPositions(parser, input);
	if (pos.get() == nullptr) return {};
	if (!pos->hasClientData() && !pos->hasServerData()) {
		REGEN_WARN("Mesh '" << input.getValue("mesh") <<
							"' has no position data available.");
		return {};
	}
	bool loadServerData = !pos->hasClientData();
	if (loadServerData) pos->readServerData();

	auto *points = (const btScalar *) pos->clientData();
	//create a hull approximation
	//btShapeHull* hull = new btShapeHull(originalConvexShape);
	//btScalar margin = originalConvexShape->getMargin();
	//hull->buildHull(margin);
	//btConvexHullShape* simplifiedConvexShape = new btConvexHullShape(hull->getVertexPointer(),hull->numVertices());

	auto shape = ref_ptr<btConvexHullShape>::alloc(points, pos->numVertices());
	if (loadServerData) pos->deallocateClientData();
	return shape;

}

static ref_ptr<btCollisionShape> createTriangleMesh(SceneParser *parser, SceneInputNode &input) {
	auto mesh = getMesh(parser, input);
	auto pos = (mesh.get() == nullptr ? ref_ptr<ShaderInput>() : mesh->positions());
	auto indices = (mesh.get() == nullptr ? ref_ptr<ShaderInput>() : mesh->inputContainer()->indices());

	if (indices.get() == nullptr) {
		REGEN_WARN("Ignoring physical shape for '" << input.getDescription() << "'. Mesh has no Indices.");
		return {};
	}
	if (pos.get() == nullptr) {
		REGEN_WARN("Ignoring physical shape for '" << input.getDescription()  << "'. Mesh has no Positions.");
		return {};
	}
	btIndexedMesh btMesh;
	btMesh.m_numVertices = static_cast<int>(pos->numVertices());
	btMesh.m_vertexStride = static_cast<int>(pos->elementSize());

	switch (mesh->primitive()) {
		case GL_TRIANGLES:
			btMesh.m_triangleIndexStride = 3 * static_cast<int>(indices->elementSize());
			btMesh.m_numTriangles = static_cast<int>(indices->numVertices()) / 3;
			break;
		case GL_TRIANGLE_STRIP:
			btMesh.m_triangleIndexStride = 1 * static_cast<int>(indices->elementSize());
			btMesh.m_numTriangles = static_cast<int>(indices->numVertices() - 2);
			break;
		default:
			btMesh.m_numTriangles = -1;
			btMesh.m_triangleIndexStride = -1;
			break;
	}

	if (btMesh.m_numTriangles <= 0) {
		REGEN_WARN("Unsupported primitive for btTriangleIndexVertexArray "
						   << "in input node " << input.getDescription() << "'.");
		return {};
	}

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
	return ref_ptr<btBvhTriangleMeshShape>::alloc(btMeshIface, useQuantizedAabbCompression);
}


static ref_ptr<PhysicalProps> createPhysicalProps(
		SceneParser *parser,
		SceneInputNode &input,
		const ref_ptr<btMotionState> &motion) {
	const std::string shapeName(input.getValue("shape"));
	auto mass = input.getValue<GLfloat>("mass", 1.0f);

	// create a collision shape
	ref_ptr<btCollisionShape> shape;
	if (shapeName == "sphere") {
		shape = createSphere(parser, input);
	} else if (shapeName == "wall") {
		shape = createWall(parser, input);
		mass = 0.0;
	} else if (shapeName == "infinite-wall") {
		shape = createInfiniteWall(parser, input);
		mass = 0.0;
	} else if (shapeName == "box") {
		shape = createBox(parser, input);
	} else if (shapeName == "cylinder") {
		shape = createCylinder(parser, input);
	} else if (shapeName == "capsule") {
		shape = createCapsule(parser, input);
	} else if (shapeName == "cone") {
		shape = createCone(parser, input);
	} else if (shapeName == "convex-hull") {
		shape = createConvexHull(parser, input);
	} else if (shapeName == "triangle-mesh") {
		shape = createTriangleMesh(parser, input);
	} else {
		REGEN_WARN("Ignoring unknown physical shape '" << input.getDescription() << "'.");
		return {};
	}
	if (shape.get() == nullptr) {
		REGEN_WARN("Failed to create shape '" << input.getDescription() << "'.");
		return {};
	}

	auto props = ref_ptr<PhysicalProps>::alloc(motion, shape);
	auto inertia = input.getValue<Vec3f>("inertia", Vec3f(0.0f));
	props->setMassProps(mass, btVector3(inertia.x, inertia.x, inertia.z));
	auto gravity = input.getValue<Vec3f>("gravity", Vec3f(0.0f, -9.8f, 0.0f));
	props->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

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

	auto meshIndex = input.getValue<unsigned int>("mesh-index", 0);
	auto mesh = (*meshVector->begin());
	if (meshIndex > 0 && meshIndex < meshVector->size()) {
		mesh = (*meshVector.get())[meshIndex];
	}

	auto numInstances = input.getValue<GLuint>("num-instances", 1u);
	for (GLuint i = 0; i < numInstances; ++i) {
		auto motion = ref_ptr<ModelMatrixMotion>::alloc(transform->get(), i);
		auto physicalProps = createPhysicalProps(parser, input, motion);
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

