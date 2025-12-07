#include "shape-processor.h"

#include "../scene/resource-manager.h"
#include "regen/shapes/aabb.h"
#include "regen/shapes/obb.h"
#include "regen/objects/composite-mesh.h"
#include "regen/shapes/spatial-index.h"
#include "regen/shapes/cull-shape.h"
#include "regen/textures/height-map.h"

using namespace regen::scene;
using namespace regen;

static ref_ptr<btCollisionShape> createSphere(SceneInputNode &input) {
	auto radius = input.getValue<float>("radius", 1.0f) * 0.5;
	return ref_ptr<btSphereShape>::alloc(radius);
}

static ref_ptr<btCollisionShape> createWall(SceneInputNode &input) {
	auto size = input.getValue<Vec2f>("size", Vec2f::one());
	btVector3 halfExtend(size.x * 0.5f, 0.001f, size.y * 0.5f);
	return ref_ptr<btBoxShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createInfiniteWall(SceneInputNode &input) {
	auto planeNormal = input.getValue<Vec3f>("normal", Vec3f(0.0f, 1.0f, 0.0f));
	auto planeConstant = input.getValue<float>("constant", float(0.0f));
	return ref_ptr<btStaticPlaneShape>::alloc(
			btVector3(planeNormal.x, planeNormal.y, planeNormal.z),
			planeConstant);
}

static ref_ptr<btCollisionShape> createBox(SceneInputNode &input) {
	auto size = input.getValue<Vec3f>("size", Vec3f::one());
	btVector3 halfExtend(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
	return ref_ptr<btBoxShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createCylinder(SceneInputNode &input) {
	auto size = input.getValue<Vec3f>("size", Vec3f::one());
	btVector3 halfExtend(size.x * 0.5f, size.y * 0.5f, size.z * 0.5f);
	return ref_ptr<btCylinderShape>::alloc(halfExtend);
}

static ref_ptr<btCollisionShape> createCapsule(SceneInputNode &input) {
	return ref_ptr<btCapsuleShape>::alloc(
			input.getValue<float>("radius", 1.0f),
			input.getValue<float>("height", 1.0f));
}

static ref_ptr<btCollisionShape> createCone(SceneInputNode &input) {
	return ref_ptr<btConeShape>::alloc(
			input.getValue<float>("radius", 1.0f),
			input.getValue<float>("height", 1.0f));
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

	auto v_pos = pos->mapClientData<btScalar>(BUFFER_GPU_READ);
	//create a hull approximation
	//btShapeHull* hull = new btShapeHull(originalConvexShape);
	//btScalar margin = originalConvexShape->getMargin();
	//hull->buildHull(margin);
	//btConvexHullShape* simplifiedConvexShape = new btConvexHullShape(hull->getVertexPointer(),hull->numVertices());

	auto shape = ref_ptr<btConvexHullShape>::alloc(v_pos.r.data(), pos->numVertices());
	if (loadServerData) pos->deallocateClientData();
	return shape;

}

static ref_ptr<btCollisionShape>
createTriangleMesh(SceneInputNode &input, const ref_ptr<Mesh> &mesh) {
	auto pos = (mesh.get() == nullptr ? ref_ptr<ShaderInput>() : mesh->positions());
	auto indices = (mesh.get() == nullptr ? ref_ptr<ShaderInput>() : mesh->indices());

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
	auto v_pos = pos->mapClientDataRaw(BUFFER_GPU_READ);
	auto indices_data = indices->mapClientDataRaw(BUFFER_GPU_READ);
	btMesh.m_vertexBase = v_pos.r;
	btMesh.m_triangleIndexBase = indices_data.r;

	PHY_ScalarType indexType;
	switch (indices->baseType()) {
		case GL_UNSIGNED_SHORT:
			indexType = PHY_SHORT;
			break;
		case GL_UNSIGNED_BYTE:
			indexType = PHY_UCHAR;
			break;
		case GL_UNSIGNED_INT:
			indexType = PHY_INTEGER;
			break;
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
	auto mass = input.getValue<float>("mass", 1.0f);

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
	auto inertia = input.getValue<Vec3f>("inertia", Vec3f::zero());
	props->setMassProps(mass, btVector3(inertia.x, inertia.x, inertia.z));
	auto gravity = input.getValue<Vec3f>("gravity", Vec3f(0.0f, -9.8f, 0.0f));
	props->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

	props->setRestitution(
			input.getValue<float>("restitution", 0.0f));

	props->setLinearSleepingThreshold(
			input.getValue<float>("linear-sleeping-threshold", 0.8f));
	props->setAngularSleepingThreshold(
			input.getValue<float>("angular-sleeping-threshold", 1.0f));

	props->setFriction(
			input.getValue<float>("friction", 0.5f));
	props->setRollingFriction(
			input.getValue<float>("rolling-friction", 0.0f));

	props->setAdditionalDamping(
			input.getValue<bool>("additional-damping", false));
	props->setAdditionalDampingFactor(
			input.getValue<float>("additional-damping-factor", 0.005f));

	props->setLinearDamping(
			input.getValue<float>("linear-damping", 0.0f));
	props->setAdditionalLinearDampingThresholdSqr(
			input.getValue<float>("additional-linear-damping-threshold", 0.01f));

	props->setAngularDamping(
			input.getValue<float>("angular-damping", 0.0f));
	props->setAdditionalAngularDampingFactor(
			input.getValue<float>("additional-angular-damping-factor", 0.01f));
	props->setAdditionalAngularDampingThresholdSqr(
			input.getValue<float>("additional-angular-damping-threshold", 0.01f));

	if (mass > 0) props->calculateLocalInertia();

	return props;
}

static std::optional<float> getRadius(SceneInputNode &input) {
	if (input.hasAttribute("radius")) {
		return input.getValue<float>("radius", 1.0f);
	}
	REGEN_WARN("Ignoring sphere " << input.getDescription() << " without radius or mesh.");
	return std::nullopt;
}

static std::optional<Bounds<Vec3f>> getBoxBounds(SceneInputNode &input) {
	if (input.hasAttribute("box-size")) {
		auto size = input.getValue<Vec3f>("box-size", Vec3f::zero());
		auto halfSize = size * 0.5;
		auto bounds = Bounds<Vec3f>::create(-halfSize, halfSize);
		if (input.hasAttribute("box-center")) {
			auto center = input.getValue<Vec3f>("box-center", Vec3f::zero());
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
		ref_ptr<BoundingSphere> sphere;
		if (mesh.get()) {
			if (input.hasAttribute("radius")) {
				sphere = ref_ptr<BoundingSphere>::alloc(mesh, parts, input.getValue<float>("radius", 1.0f));
			} else {
				sphere = ref_ptr<BoundingSphere>::alloc(mesh, parts);
			}
			if (input.hasAttribute("base-offset"))  {
				sphere->setBasePosition(
					input.getValue<Vec3f>("base-offset", Vec3f::zero()));
			}
		} else if (input.hasAttribute("radius")) {
			auto radius_opt = getRadius(input);
			if (radius_opt.has_value()) {
				auto center = input.getValue<Vec3f>("center", Vec3f::zero());
				sphere = ref_ptr<BoundingSphere>::alloc(center, radius_opt.value());
				for (auto &part: parts) { sphere->addPart(part); }
			}
		}
		shape = sphere;
	} else if (shapeType == "aabb") {
		if (mesh.get()) {
			shape = ref_ptr<AABB>::alloc(mesh, parts);
		} else {
			auto size_opt = getBoxBounds(input);
			if (size_opt.has_value()) {
				shape = ref_ptr<AABB>::alloc(size_opt.value());
				for (auto &part: parts) { shape->addPart(part); }
			}
		}
	} else if (shapeType == "obb") {
		if (mesh.get()) {
			shape = ref_ptr<OBB>::alloc(mesh, parts);
		} else {
			auto size_opt = getBoxBounds(input);
			if (size_opt.has_value()) {
				shape = ref_ptr<OBB>::alloc(size_opt.value());
				for (auto &part: parts) { shape->addPart(part); }
			}
		}
	}
	if (!shape.get()) {
		REGEN_WARN("Ignoring unknown shape '" << input.getDescription() << "'.");
		return {};
	}

	return shape;
}

static std::vector<ref_ptr<Mesh>> getParts(scene::SceneLoader *scene, SceneInputNode &input) {
	std::vector<ref_ptr<Mesh>> parts;
	for (auto &child: input.getChildren("has-part")) {
		auto meshID = child->getValue("mesh-id");
		auto compositeMesh = scene->getResource<CompositeMesh>(meshID);
		if (compositeMesh.get() == nullptr) {
			continue;
		}
		if (child->hasAttribute("mesh-index")) {
			auto meshIndex = child->getValue<unsigned int>("mesh-index", 0);
			if (meshIndex < compositeMesh->meshes().size()) {
				auto &part = compositeMesh->meshes()[meshIndex];
				parts.push_back(part);
			} else {
				REGEN_WARN("Ignoring part mesh with invalid index in node " << input.getDescription() << ".");
			}
		} else {
			for (auto &part: compositeMesh->meshes()) {
				parts.push_back(part);
			}
		}
	}
	return parts;
}

static ref_ptr<Mesh> getMesh(scene::SceneLoader *scene, SceneInputNode &input) {
	auto meshID = input.getValue("mesh-id");
	auto compositeMesh = scene->getResource<CompositeMesh>(meshID);
	if (compositeMesh.get() == nullptr) {
		if (!meshID.empty()) {
			REGEN_WARN("Unable to find MeshVector with ID '" << meshID << "' for input node " << input.getDescription() << ".");
		}
		return {};
	}
	auto meshIndex = input.getValue<unsigned int>("mesh-index", 0);
	auto mesh = (*compositeMesh->meshes().begin());
	if (meshIndex > 0 && meshIndex < compositeMesh->meshes().size()) {
		mesh = compositeMesh->meshes()[meshIndex];
	}
	return mesh;
}

static ref_ptr<SpatialIndex> getSpatialIndex(scene::SceneLoader *scene, SceneInputNode &input) {
	ref_ptr<SpatialIndex> spatialIndex;
	if (input.hasAttribute("spatial-index")) {
		auto spatialIndexID = input.getValue("spatial-index");
		spatialIndex = scene->getResource<SpatialIndex>(spatialIndexID);
	} else {
		auto indexNode = scene->getRoot()->getFirstChild("index");
		if (indexNode.get()) {
			spatialIndex = scene->getResource<SpatialIndex>(indexNode->getName());
		}
	}
	return spatialIndex;
}

void ShapeProcessor::processInput(
		scene::SceneLoader *scene,
		SceneInputNode &input,
		const ref_ptr<StateNode> &parentNode,
		const ref_ptr<State> &parentState) {
	auto transformID = input.getValue("transform-id");
	auto mesh = getMesh(scene, input);
	auto parts = getParts(scene, input);
	// read flags for target of shape
	auto isMeshShape = input.getValue<uint32_t>("mesh", 0u);
	auto isIndexShape = input.getValue<uint32_t>("index", 0u);
	auto isGPUShape = input.getValue<uint32_t>("gpu", 0u);
	auto isPhysicalShape = input.getValue<uint32_t>("physics", 0u);
	auto isCullShape = input.getValue<uint32_t>("cull", 0u);
	auto useLocalStamp = input.getValue<bool>("local-stamp", false);
	if (!isIndexShape && !isGPUShape && !isPhysicalShape) {
		isMeshShape = 1;
	}
	auto numInstances = 1u;
	auto shapeName = input.getName();

	ref_ptr<HeightMap> heightMap;
	if (input.hasAttribute("height-map")) {
		auto heightMap2D = scene->getResources()->getTexture2D(scene, input.getValue("height-map"));
		if (heightMap2D.get()) {
			heightMap = ref_ptr<HeightMap>::dynamicCast(heightMap2D);
			if (!heightMap) {
				REGEN_WARN("Texture for height map is not a height map for " << input.getDescription() << ".");
			} else {
				heightMap->ensureTextureData();
			}
		} else if (input.hasAttribute("height-map")) {
			REGEN_WARN("Ignoring height map for " << input.getDescription() << " without texture.");
		}
	}

	uint32_t traversalMask = 1;
	for (auto &child: input.getChildren("traversal-bit")) {
		auto bit = child->getValue<uint32_t>("value", 0u);
		if (child->getValue<bool>("enabled", true)) {
			traversalMask |= (1 << bit);
		} else {
			traversalMask &= ~(1 << bit);
		}
	}

	auto transform = scene->getResource<ModelTransformation>(transformID);
	if (transform.get()) {
		numInstances = transform->numInstances();
	}
	if (input.hasAttribute("num-instances")) {
		numInstances = input.getValue<uint32_t>("num-instances", numInstances);
	}

	if (isPhysicalShape) {
		// add shape to physics engine
		if (numInstances == 1) {
			auto motion = ref_ptr<ModelMatrixMotion>::alloc(transform, 0);
			auto physicalProps = createPhysicalProps(input, mesh, motion);
			auto physicalObject = ref_ptr<PhysicalObject>::alloc(physicalProps);
			mesh->addPhysicalObject(physicalObject);
			scene->getPhysics()->addObject(physicalObject);
		} else {
			auto motionAnim = ref_ptr<ModelMatrixUpdater>::alloc(transform);
			for (uint32_t i = 0; i < numInstances; ++i) {
				auto motion = ref_ptr<Mat4fMotion>::alloc(motionAnim, i);
				auto physicalProps = createPhysicalProps(input, mesh, motion);
				auto physicalObject = ref_ptr<PhysicalObject>::alloc(physicalProps);
				mesh->addPhysicalObject(physicalObject);
				scene->getPhysics()->addObject(physicalObject);
			}
			motionAnim->startAnimation();
			mesh->addAnimation(motionAnim);
		}
	}

	if (isIndexShape) {
		// add shape to spatial index
		auto spatialIndex = getSpatialIndex(scene, input);
		if (spatialIndex.get()) {
			for (uint32_t i = 0; i < numInstances; ++i) {
				auto shape = createShape(input, mesh, parts);
				if (!shape.get()) {
					REGEN_WARN("Skipping shape node " << input.getDescription() << " without shape.");
					continue;
				}
				shape->setUseLocalStamp(useLocalStamp);
				shape->setName(shapeName);
				shape->setInstanceID(i);
				shape->setTraversalMask(traversalMask);
				if (transform.get()) {
					shape->setTransform(transform, transform->numInstances() > 1 ? i : 0);
				}
				if (heightMap.get()) {
					// translate to half height of height map
					shape->setBaseOffset(Vec3f(0.0f, 0.5f*heightMap->mapFactor(), 0.0f));
				}
				spatialIndex->insert(shape);
			}

			auto indexedShapes = spatialIndex->getShapes(shapeName);
			if (indexedShapes.get()) {
				if (mesh.get()) {
					mesh->setIndexedShapes(indexedShapes);
				}
				for (auto &part: parts) {
					part->setIndexedShapes(indexedShapes);
				}
			}

			if (isCullShape) {
				REGEN_DEBUG("Creating CPU cull shape for " << input.getDescription() << ".");
				auto cullShape = ref_ptr<CullShape>::alloc(spatialIndex, input.getName());
				if (mesh.get()) {
					mesh->setCullShape(cullShape);
				}
				for (auto &part: parts) {
					part->setCullShape(cullShape);
				}
			}
		} else {
			REGEN_WARN("Skipping shape node " << input.getDescription() << " without spatial index.");
		}
		if (mesh.get() && !mesh->hasBoundingShape()) {
			// also create a shape without instances for the mesh bounding box
			isMeshShape = 1;
		}
	}

	if (isGPUShape || isMeshShape) {
		auto shape = createShape(input, mesh, parts);
		if (shape.get()) {
			shape->setName(shapeName);
			if (mesh.get()) {
				mesh->setBoundingShape(shape);
			}
			for (auto &part: parts) {
				part->setBoundingShape(shape);
			}
		} else {
			REGEN_WARN("Skipping shape node " << input.getDescription() << " without shape.");
		}
		if (transform.get()) {
			shape->setTransform(transform);
		}
		if (isGPUShape && isCullShape) {
			REGEN_DEBUG("Creating GPU cull shape for " << input.getDescription() << ".");
			auto cullShape = ref_ptr<CullShape>::alloc(shape, input.getName());
			if (mesh.get()) {
				mesh->setCullShape(cullShape);
			}
			for (auto &part: parts) {
				part->setCullShape(cullShape);
			}
		}
	}
}
