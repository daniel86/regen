#include "shape-processor.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"
#include "regen/meshes/mesh-vector.h"
#include "regen/shapes/spatial-index.h"

using namespace regen::scene;
using namespace regen;

static ref_ptr<btCollisionShape> createSphere(SceneInputNode &input) {
	auto radius = input.getValue<GLfloat>("radius", 1.0f) * 0.5;
	return ref_ptr<btSphereShape>::alloc(radius);
}

static ref_ptr<btCollisionShape> createWall(SceneInputNode &input) {
	auto size = input.getValue<Vec2f>("size", Vec2f(1.0f));
	// TODO: allow configuration of orientation and position
	btVector3 halfExtend(size.x * 0.5f, 0.001f, size.y * 0.5f);
	return ref_ptr<btBoxShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createInfiniteWall(SceneInputNode &input) {
	auto planeNormal = input.getValue<Vec3f>("normal", Vec3f(0.0f, 1.0f, 0.0f));
	auto planeConstant = input.getValue<GLfloat>("constant", GLfloat(0.0f));
	return ref_ptr<btStaticPlaneShape>::alloc(
			btVector3(planeNormal.x, planeNormal.y, planeNormal.z),
			planeConstant);
}

static ref_ptr<btCollisionShape> createBox(SceneInputNode &input) {
	auto size = input.getValue<Vec3f>("size", Vec3f(1.0f));
	btVector3 halfExtend(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
	return ref_ptr<btBoxShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createCylinder(SceneInputNode &input) {
	auto size = input.getValue<Vec3f>("size", Vec3f(1.0f));
	btVector3 halfExtend(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
	return ref_ptr<btCylinderShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createCapsule(SceneInputNode &input) {
	return ref_ptr<btCapsuleShape>::alloc(
			input.getValue<GLfloat>("radius", 1.0f),
			input.getValue<GLfloat>("height", 1.0f));
}

static ref_ptr<btCollisionShape> createCone(SceneInputNode &input) {
	return ref_ptr<btConeShape>::alloc(
			input.getValue<GLfloat>("radius", 1.0f),
			input.getValue<GLfloat>("height", 1.0f));
}

static ref_ptr<btCollisionShape>
createConvexHull(SceneInputNode &input, const ref_ptr<Mesh> &mesh) {
	ref_ptr<ShaderInput> pos = mesh->positions();
	if (pos.get() == nullptr) return {};
	if (!pos->hasClientData() && !pos->hasServerData()) {
		REGEN_WARN("Mesh '" << input.getValue("mesh") <<
							"' has no position data available.");
		return {};
	}
	bool loadServerData = !pos->hasClientData();
	if (loadServerData) pos->readServerData();

	auto v_pos = pos->mapClientData<btScalar>(ShaderData::READ);
	//create a hull approximation
	//btShapeHull* hull = new btShapeHull(originalConvexShape);
	//btScalar margin = originalConvexShape->getMargin();
	//hull->buildHull(margin);
	//btConvexHullShape* simplifiedConvexShape = new btConvexHullShape(hull->getVertexPointer(),hull->numVertices());

	auto shape = ref_ptr<btConvexHullShape>::alloc(v_pos.r, pos->numVertices());
	if (loadServerData) pos->deallocateClientData();
	return shape;

}

static ref_ptr<btCollisionShape>
createTriangleMesh(SceneInputNode &input, const ref_ptr<Mesh> &mesh) {
	auto pos = (mesh.get() == nullptr ? ref_ptr<ShaderInput>() : mesh->positions());
	auto indices = (mesh.get() == nullptr ? ref_ptr<ShaderInput>() : mesh->inputContainer()->indices());

	if (indices.get() == nullptr) {
		REGEN_WARN("Ignoring physical shape for '" << input.getDescription() << "'. Mesh has no Indices.");
		return {};
	}
	if (pos.get() == nullptr) {
		REGEN_WARN("Ignoring physical shape for '" << input.getDescription() << "'. Mesh has no Positions.");
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
	auto v_pos = pos->mapClientDataRaw(ShaderData::READ);
	auto indices_data = indices->mapClientDataRaw(ShaderData::READ);
	btMesh.m_vertexBase = v_pos.r;
	btMesh.m_triangleIndexBase = indices_data.r;

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
		SceneInputNode &input,
		const ref_ptr<Mesh> &mesh,
		const ref_ptr<btMotionState> &motion) {
	const std::string shapeName(input.getValue("shape"));
	auto mass = input.getValue<GLfloat>("mass", 1.0f);

	// create a collision shape
	ref_ptr<btCollisionShape> shape;
	if (shapeName == "sphere") {
		shape = createSphere(input);
	} else if (shapeName == "wall") {
		shape = createWall(input);
		mass = 0.0;
	} else if (shapeName == "infinite-wall") {
		shape = createInfiniteWall(input);
		mass = 0.0;
	} else if (shapeName == "box") {
		shape = createBox(input);
	} else if (shapeName == "cylinder") {
		shape = createCylinder(input);
	} else if (shapeName == "capsule") {
		shape = createCapsule(input);
	} else if (shapeName == "cone") {
		shape = createCone(input);
	} else if (shapeName == "convex-hull") {
		shape = createConvexHull(input, mesh);
	} else if (shapeName == "triangle-mesh") {
		shape = createTriangleMesh(input, mesh);
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

static std::optional<float> getRadius(SceneInputNode &input) {
	if (input.hasAttribute("radius")) {
		return input.getValue<GLfloat>("radius", 1.0f);
	}
	REGEN_WARN("Ignoring sphere " << input.getDescription() << " without radius or mesh.");
	return std::nullopt;
}

static std::optional<Bounds<Vec3f>> getBoxBounds(SceneInputNode &input) {
	if (input.hasAttribute("box-size")) {
		auto size = input.getValue<Vec3f>("box-size", Vec3f(0.0));
		auto halfSize = size * 0.5;
		auto bounds = Bounds<Vec3f>{-halfSize, halfSize};
		if (input.hasAttribute("box-center")) {
			auto center = input.getValue<Vec3f>("box-center", Vec3f(0.0));
			bounds.min += center;
			bounds.max += center;
		}
		return bounds;
	}
	REGEN_WARN("Ignoring AABB " << input.getDescription() << " without size or mesh.");
	return std::nullopt;
}

static ref_ptr<BoundingShape> createShape(
		SceneInputNode &input,
		const ref_ptr<Mesh> &mesh,
		const std::vector<ref_ptr<Mesh>> &parts) {
	auto shapeType = input.getValue<std::string>("type", "aabb");

	// create a collision shape
	ref_ptr<BoundingShape> shape;
	if (shapeType == "sphere") {
		if (mesh.get()) {
			if (input.hasAttribute("radius")) {
				shape = ref_ptr<BoundingSphere>::alloc(mesh, input.getValue<float>("radius", 1.0f));
			} else {
				shape = ref_ptr<BoundingSphere>::alloc(mesh);
			}
		} else if (input.hasAttribute("radius")) {
			auto radius_opt = getRadius(input);
			if (radius_opt.has_value()) {
				auto center = input.getValue<Vec3f>("center", Vec3f(0.0f));
				shape = ref_ptr<BoundingSphere>::alloc(center, radius_opt.value());
			}
		}
	} else if (shapeType == "aabb") {
		if (mesh.get()) {
			shape = ref_ptr<AABB>::alloc(mesh);
		} else {
			auto size_opt = getBoxBounds(input);
			if (size_opt.has_value()) {
				shape = ref_ptr<AABB>::alloc(size_opt.value());
			}
		}
	} else if (shapeType == "obb") {
		if (mesh.get()) {
			shape = ref_ptr<OBB>::alloc(mesh);
		} else {
			auto size_opt = getBoxBounds(input);
			if (size_opt.has_value()) {
				shape = ref_ptr<OBB>::alloc(size_opt.value());
			}
		}
	}
	if (!shape.get()) {
		REGEN_WARN("Ignoring unknown shape '" << input.getDescription() << "'.");
		return {};
	}

	// handle input children with tag "has-part" which define additional
	// meshes to be included in the shape.
	for (auto &part: parts) {
		shape->addPart(part);
	}

	return shape;
}

static std::vector<ref_ptr<Mesh>> getParts(scene::SceneLoader *scene, SceneInputNode &input) {
	std::vector<ref_ptr<Mesh>> parts;
	for (auto &child: input.getChildren("has-part")) {
		auto meshID = child->getValue("mesh-id");
		auto meshVector = scene->getResource<MeshVector>(meshID);
		if (meshVector.get() == nullptr) {
			continue;
		}
		if (child->hasAttribute("mesh-index")) {
			auto meshIndex = child->getValue<unsigned int>("mesh-index", 0);
			if (meshIndex < meshVector->size()) {
				auto &part = (*meshVector.get())[meshIndex];
				parts.push_back(part);
			} else {
				REGEN_WARN("Ignoring part mesh with invalid index in node " << input.getDescription() << ".");
			}
		} else {
			for (auto &part: *meshVector.get()) {
				parts.push_back(part);
			}
		}
	}
	return parts;
}

static ref_ptr<Mesh> getMesh(scene::SceneLoader *scene, SceneInputNode &input) {
	auto meshID = input.getValue("mesh-id");
	auto meshVector = scene->getResource<MeshVector>(meshID);
	if (meshVector.get() == nullptr) {
		return {};
	}
	auto meshIndex = input.getValue<unsigned int>("mesh-index", 0);
	auto mesh = (*meshVector->begin());
	if (meshIndex > 0 && meshIndex < meshVector->size()) {
		mesh = (*meshVector.get())[meshIndex];
	}
	return mesh;
}

static ref_ptr<SpatialIndex> getSpatialIndex(scene::SceneLoader *scene, SceneInputNode &input) {
	ref_ptr<SpatialIndex> spatialIndex;
	if (input.hasAttribute("spatial-index-id")) {
		auto spatialIndexID = input.getValue("spatial-index-id");
		spatialIndex = scene->getResource<SpatialIndex>(spatialIndexID);
	} else {
		auto indexNode = scene->getRoot()->getFirstChild("index");
		if (indexNode.get()) {
			spatialIndex = scene->getResource<SpatialIndex>(indexNode->getName());
		}
	}
	return spatialIndex;
}

static ref_ptr<ShaderInput3f> getOffset(
		SceneInputNode &input,
		const ref_ptr<Mesh> &mesh,
		const std::vector<ref_ptr<Mesh>> &parts) {
	// try to find shader inputs of mesh
	ref_ptr<Mesh> m = mesh;
	if (m.get() == nullptr) {
		if (!parts.empty()) {
			m = parts[0];
		} else {
			return {};
		}
	}
	auto meshOffset = m->findShaderInput("modelOffset");
	if (meshOffset.has_value()) {
		auto upcasted = ref_ptr<ShaderInput3f>::dynamicCast(meshOffset.value().in);
		if (upcasted.get()) {
			return upcasted;
		} else {
			REGEN_WARN("Ignoring mesh offset with wrong type in node " << input.getDescription() << ".");
		}
	}
	return {};
}

void ShapeProcessor::processInput(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parentNode,
		const ref_ptr<State> &parentState) {
	auto transformID = input.getValue("transform-id");
	auto transform = scene->getResource<ModelTransformation>(transformID);
	auto mesh = getMesh(scene, input);
	auto parts = getParts(scene, input);
	auto offset = getOffset(input, mesh, parts);
	auto shapeMode = input.getValue<std::string>("mode", "index");

	auto numInstances = 1u;
	if (transform.get()) {
		numInstances = transform->get()->numInstances();
	}
	if (offset.get()) {
		numInstances = std::max(offset->numInstances(), numInstances);
	}
	if (input.hasAttribute("num-instances")) {
		numInstances = input.getValue<GLuint>("num-instances", numInstances);
	}

	if (shapeMode == "index") {
		// add shape to spatial index
		auto spatialIndex = getSpatialIndex(scene, input);
		if (spatialIndex.get()) {
			for (GLuint i = 0; i < numInstances; ++i) {
				auto shape = createShape(input, mesh, parts);
				if (!shape.get()) {
					REGEN_WARN("Skipping shape node " << input.getDescription() << " without shape.");
					continue;
				}
				shape->setName(input.getName());
				shape->setInstanceID(i);
				if (transform.get()) {
					shape->setTransform(transform, transform->get()->numInstances() > 1 ? i : 0);
				}
				if (offset.get()) {
					shape->setTransform(offset, offset->numInstances() > 1 ? i : 0);
				}
				spatialIndex->insert(shape);
			}
		} else {
			REGEN_WARN("Skipping shape node " << input.getDescription() << " without spatial index.");
			return;
		}
	} else if (shapeMode == "physics") {
		// add shape to physics engine
		if (numInstances == 1) {
			auto motion = ref_ptr<ModelMatrixMotion>::alloc(transform->get(), 0);
			auto physicalProps = createPhysicalProps(input, mesh, motion);
			auto physicalObject = ref_ptr<PhysicalObject>::alloc(physicalProps);
			mesh->addPhysicalObject(physicalObject);
			scene->getPhysics()->addObject(physicalObject);
		} else {
			auto motionAnim = ref_ptr<ModelMatrixUpdater>::alloc(transform->get());
			for (GLuint i = 0; i < numInstances; ++i) {
				auto motion = ref_ptr<Mat4fMotion>::alloc(motionAnim, i);
				auto physicalProps = createPhysicalProps(input, mesh, motion);
				auto physicalObject = ref_ptr<PhysicalObject>::alloc(physicalProps);
				mesh->addPhysicalObject(physicalObject);
				scene->getPhysics()->addObject(physicalObject);
			}
			motionAnim->startAnimation();
			mesh->addAnimation(motionAnim);
		}
	} else {
		REGEN_WARN("Ignoring unknown shape mode '" << input.getDescription() << "'.");
	}
}
