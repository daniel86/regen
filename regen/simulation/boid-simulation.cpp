#include "boid-simulation.h"
#include "boids-cpu.h"
#include "regen/scene/resource-manager.h"
#include "regen/textures/collision-map.h"

using namespace regen;

BoidSimulation::BoidSimulation(const ref_ptr<ModelTransformation> &tf) : tf_(tf) {
	boidsScale_ = ref_ptr<ShaderInput3f>::alloc("scaleFactor");
	if (tf->hasModelMat()) {
		boidsScale_->setUniformData(tf_->modelMat()->getVertex(0).r.scaling());
	} else {
		boidsScale_->setUniformData(Vec3f::one());
	}
	numBoids_ = tf->numInstances();
	initBoidSimulation0();
}

BoidSimulation::BoidSimulation(const ref_ptr<ShaderInput4f> &modelOffset) : modelOffset_(modelOffset) {
	boidsScale_ = ref_ptr<ShaderInput3f>::alloc("scaleFactor");
	boidsScale_->setUniformData(Vec3f::one());
	numBoids_ = modelOffset->numInstances();
	initBoidSimulation0();
}

void BoidSimulation::initBoidSimulation0() {
	baseOrientation_ = createUniform<ShaderInput1f,float>("baseOrientation", 0.0f);
	coherenceWeight_ = createUniform<ShaderInput1f,float>("coherenceWeight", 1.0f);
	alignmentWeight_ = createUniform<ShaderInput1f,float>("alignmentWeight", 1.0f);
	separationWeight_ = createUniform<ShaderInput1f,float>("separationWeight", 1.0f);
	avoidanceWeight_ = createUniform<ShaderInput1f,float>("avoidanceWeight", 1.0f);
	avoidanceDistance_ = createUniform<ShaderInput1f,float>("avoidanceDistance", 0.1f);
	visualRange_ = createUniform<ShaderInput1f,float>("visualRange", 1.6f);
	attractionRange_ = createUniform<ShaderInput1f,float>("attractionRange", 160.0f);
	lookAheadDistance_ = createUniform<ShaderInput1f,float>("lookAheadDistance", 0.1f);
	repulsionFactor_ = createUniform<ShaderInput1f,float>("repulsionFactor", 20.0f);
	maxNumNeighbors_ = createUniform<ShaderInput1ui,unsigned int>("maxNumNeighbors", 20);
	maxBoidSpeed_ = createUniform<ShaderInput1f,float>("maxBoidSpeed", 1.0f);
	maxAngularSpeed_ = createUniform<ShaderInput1f,float>("maxAngularSpeed", 0.05f);
	gridSize_ = createUniform<ShaderInput3ui,Vec3ui>("gridSize", Vec3ui::create(0));
	cellSize_ = createUniform<ShaderInput1f,float>("cellSize", 3.2f);
	simulationBoundsMin_ = createUniform<ShaderInput3f,Vec3f>("simulationBoundsMin", Vec3f::create(-10.0f));
	simulationBoundsMax_ = createUniform<ShaderInput3f,Vec3f>("simulationBoundsMax", Vec3f::create(10.0f));
}

void BoidSimulation::setVisualRange(float range) {
	visualRange_->setVertex(0, range);
	cellSize_->setVertex(0, range * 2.0f );
}

void BoidSimulation::setAttractionRange(float range) {
	attractionRange_->setVertex(0, range);
}

void BoidSimulation::setSimulationBounds(const Bounds<Vec3f> &bounds) {
	simulationBoundsMin_->setVertex(0, bounds.min);
	simulationBoundsMax_->setVertex(0, bounds.max);
}

void BoidSimulation::setBaseOrientation(float orientation) {
	baseOrientation_->setVertex(0, orientation);
}

void BoidSimulation::setMap(
		const Vec3f &mapCenter,
		const Vec2f &mapSize,
		const ref_ptr<Texture2D> &heightMap,
		float heightMapFactor) {
	mapCenter_ = mapCenter;
	mapSize_ = mapSize;
	heightMap_ = heightMap;
	heightMapFactor_ = heightMapFactor;
}

void BoidSimulation::setCollisionMap(
		const Vec3f &mapCenter,
		const Vec2f &mapSize,
		const ref_ptr<Texture2D> &collisionMap,
		CollisionMapType mapType) {
	collisionMap_ = collisionMap;
	collisionMapCenter_ = mapCenter;
	collisionMapSize_ = mapSize;
	collisionMapType_ = mapType;
}

void BoidSimulation::addHomePoint(const Vec3f &homePoint) {
	homePoints_.push_back(homePoint);
}

void BoidSimulation::addObject(
		ObjectType objectType,
		const ref_ptr<ShaderInputMat4> &tf,
		const ref_ptr<ShaderInput3f> &offset) {
	SimulationEntity *entity = nullptr;
	switch (objectType) {
		case ObjectType::ATTRACTOR:
			entity = &attractors_.emplace_back();
			break;
		case ObjectType::DANGER:
			entity = &dangers_.emplace_back();
			break;
	}
	if (!entity) { return; }
	entity->tf = tf;
	entity->pos = offset;
}

Vec3f BoidSimulation::getCellCenter(const Vec3i &gridIndex) const {
	Vec3f v(
		static_cast<float>(gridIndex.x),
		static_cast<float>(gridIndex.y),
		static_cast<float>(gridIndex.z));
	return gridBounds_.min +
		v * cellSize_->getVertex(0).r +
		Vec3f::create(visualRange_->getVertex(0).r);
}

Vec3f BoidSimulation::getCellCenter(const Vec3ui &gridIndex) const {
	Vec3f v(
		static_cast<float>(gridIndex.x),
		static_cast<float>(gridIndex.y),
		static_cast<float>(gridIndex.z));
	return gridBounds_.min +
		v * cellSize_->getVertex(0).r +
		Vec3f::create(visualRange_->getVertex(0).r);
}

Vec2f BoidSimulation::computeUV(const Vec3f &boidPosition, const Vec3f &mapCenter, const Vec2f &mapSize) {
	Vec2f boidCoord(
			boidPosition.x - mapCenter.x,
			boidPosition.z - mapCenter.z);
	return boidCoord / mapSize + Vec2f::create(0.5f);
}

void BoidSimulation::updateGridSize() {
	auto cs = cellSize_->getVertex(0).r;
	// ensure we stay in simulation bounds
	auto simMin = simulationBoundsMin_->getVertex(0);
	auto simMax = simulationBoundsMax_->getVertex(0);
#ifdef BOID_USE_FIXED_GRID
	gridBounds_.min = simMin.r;
	gridBounds_.max = simMax.r;
#else
	gridBounds_.min = Vec3f::min(gridBounds_.min, Vec3f::max(boidBounds_.min, simMin.r));
	gridBounds_.max = Vec3f::max(gridBounds_.max, Vec3f::min(boidBounds_.max, simMax.r));
#endif
	// Here we compute the grid based on cell size which is twice the visual range.
	// The idea is that we only need to consider neighbors in one direction per dimension.
	// Here, we define neighbourhood based on distance, and also compute avoidance influence based on distance.
	// Alternatively, it would be possible to skip distance computation entirely, and define both via cell adjacency.
	auto gridSize = (gridBounds_.max - gridBounds_.min) / cs;
	Vec3ui v_gridSize(
		static_cast<uint32_t>(ceil(gridSize.x)),
		static_cast<uint32_t>(ceil(gridSize.y)),
		static_cast<uint32_t>(ceil(gridSize.z)));
	uint32_t newNumCells = v_gridSize.x * v_gridSize.y * v_gridSize.z;
	// makes ure we will get at least one cell
	if (newNumCells == 0) {
		newNumCells = 1;
		v_gridSize.x = 1;
		v_gridSize.y = 1;
		v_gridSize.z = 1;
		gridBounds_.min -= Vec3f::create(cs) * 0.5f;
		gridBounds_.max += Vec3f::create(cs) * 0.5f;
	}
	if (newNumCells != numCells_) {
		numCells_ = newNumCells;
		gridSize_->setVertex(0, v_gridSize);
	}
}

namespace regen {
	std::ostream &operator<<(std::ostream &out, const BoidSimulation::ObjectType &mode) {
		switch (mode) {
			case BoidSimulation::ObjectType::ATTRACTOR:
				out << "attractor";
				break;
			case BoidSimulation::ObjectType::DANGER:
				out << "danger";
				break;
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, BoidSimulation::ObjectType &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "attractor") mode = BoidSimulation::ObjectType::ATTRACTOR;
		else if (val == "danger") mode = BoidSimulation::ObjectType::DANGER;
		else {
			REGEN_WARN("Unknown Boid Object Type '" << val <<
													"'. Using default ATTRACTOR.");
			mode = BoidsCPU::ObjectType::ATTRACTOR;
		}
		return in;
	}
}


//////////////////////////
/////////////////////////

void BoidSimulation::loadSettings(LoadingContext &ctx, scene::SceneInputNode &input) {
	// set the bounds of the boids simulation
	if (input.hasAttribute("boids-area") && input.hasAttribute("boids-center")) {
		auto boidsArea = input.getValue<Vec3f>("boids-area", Vec3f::create(10.0f));
		auto boidsCenter = input.getValue<Vec3f>("boids-center", Vec3f::create(0.0f));
		Bounds<Vec3f> bounds = Bounds<Vec3f>::create(boidsCenter - boidsArea * 0.5f, boidsCenter + boidsArea * 0.5f);
		setSimulationBounds(bounds);
	} else {
		auto boidBounds = Bounds<Vec3f>::create(
				input.getValue<Vec3f>("bounds-min", Vec3f::create(-5.0f)),
				input.getValue<Vec3f>("bounds-max", Vec3f::create(5.0f))
		);
		setSimulationBounds(boidBounds);
	}

	if (input.hasAttribute("base-orientation")) {
		setBaseOrientation(input.getValue<float>("base-orientation", 0.0f));
	}
	if (input.hasAttribute("look-ahead")) {
		setLookAheadDistance(input.getValue<float>("look-ahead", 1.0f));
	}
	if (input.hasAttribute("repulsion")) {
		setRepulsionFactor(input.getValue<float>("repulsion", 1.0f));
	}
	if (input.hasAttribute("max-speed")) {
		setMaxBoidSpeed(input.getValue<float>("max-speed", 1.0f));
	}
	if (input.hasAttribute("max-neighbors")) {
		setMaxNumNeighbors(input.getValue<int>("max-neighbors", 100));
	}
	if (input.hasAttribute("visual-range")) {
		setVisualRange(input.getValue<float>("visual-range", 1.0f));
	}
	if (input.hasAttribute("attraction-range")) {
		setAttractionRange(input.getValue<float>("attraction-range", 10.0f));
	}
	if (input.hasAttribute("coherence-weight")) {
		setCoherenceWeight(input.getValue<float>("coherence-weight", 0.5f));
	}
	if (input.hasAttribute("alignment-weight")) {
		setAlignmentWeight(input.getValue<float>("alignment-weight", 0.5f));
	}
	if (input.hasAttribute("avoidance-weight")) {
		setAvoidanceWeight(input.getValue<float>("avoidance-weight", 0.5f));
	}
	if (input.hasAttribute("avoidance-distance")) {
		setAvoidanceDistance(input.getValue<float>("avoidance-distance", 1.0f));
	}
	if (input.hasAttribute("separation-weight")) {
		setSeparationWeight(input.getValue<float>("separation-weight", 0.5f));
	}

	if (input.hasAttribute("height-map")) {
		auto heightMap = ctx.scene()->getResource<Texture2D>(input.getValue("height-map"));
		if (heightMap.get() != nullptr) {
			heightMap->ensureTextureData();
			auto heightScale = input.getValue<float>("height-map-factor", 1.0f);
			auto mapCenter = input.getValue<Vec3f>("map-center", Vec3f::create(0.0f));
			auto mapSize = input.getValue<Vec2f>("map-size", Vec2f::create(10.0f));
			setMap(mapCenter, mapSize, heightMap, heightScale);
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load height map textures.");
		}
	}

	if (input.hasAttribute("collision-map")) {
		auto collisionMap = ctx.scene()->getResource<Texture2D>(input.getValue("collision-map"));
		if (collisionMap.get() != nullptr) {
			collisionMap->ensureTextureData();
			auto mapCenter = input.getValue<Vec3f>("collision-map-center", Vec3f::create(0.0f));
			auto mapSize = input.getValue<Vec2f>("collision-map-size", Vec2f::create(10.0f));
			auto mapType = input.getValue<CollisionMapType>("collision-map-mode", COLLISION_SCALAR);
			setCollisionMap(mapCenter, mapSize, collisionMap, mapType);
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load collision map textures.");
		}
	}
	if (input.hasAttribute("collision-map-fbo")) {
		auto fbo = ctx.scene()->getResource<FBO>(input.getValue("collision-map-fbo"));
		uint32_t attachmentIdx = input.getValue<uint32_t>("collision-map-attachment", 0);
		auto collisionMap = ref_ptr<Texture2D>::dynamicCast(fbo->colorTextures()[attachmentIdx]);
		if (collisionMap.get() != nullptr) {
			collisionMap->ensureTextureData();
			auto mapCenter = input.getValue<Vec3f>("collision-map-center", Vec3f::create(0.0f));
			auto mapSize = input.getValue<Vec2f>("collision-map-size", Vec2f::create(10.0f));
			auto mapType = input.getValue<CollisionMapType>("collision-map-mode", COLLISION_SCALAR);
			setCollisionMap(mapCenter, mapSize, collisionMap, mapType);
		} else {
			REGEN_WARN("Ignoring " << input.getDescription() << ", failed to load collision map textures.");
		}
	}

	for (auto &homePointNode: input.getChildren("home-point")) {
		addHomePoint(homePointNode->getValue<Vec3f>("value", Vec3f::create(0.0f)));
	}
	for (auto &objectNode: input.getChildren("object")) {
		auto objectType = objectNode->getValue<ObjectType>("type", ObjectType::ATTRACTOR);
		ref_ptr<ShaderInputMat4> entityTF;
		ref_ptr<ShaderInput3f> entityPos;
		if (objectNode->hasAttribute("tf")) {
			auto transformID = objectNode->getValue("tf");
			auto transform = ctx.scene()->getResource<ModelTransformation>(transformID);
			if (transform.get() != nullptr) {
				entityTF = transform->modelMat();
			}
		} else if (objectNode->hasAttribute("point")) {
			entityTF = ref_ptr<ShaderInputMat4>::alloc("attractorPoint");
			entityTF->setUniformData(Mat4f::translationMatrix(
					objectNode->getValue<Vec3f>("point", Vec3f::zero())));
		} else if (objectNode->hasAttribute("value")) {
			auto attractorPosValue = objectNode->getValue<Vec3f>("value", Vec3f::zero());
			entityPos = ref_ptr<ShaderInput3f>::alloc("attractorPos");
			entityPos->setUniformData(attractorPosValue);
		} else {
			REGEN_WARN("No position or transform specified for boid object.");
			continue;
		}
		addObject(objectType, entityTF, entityPos);
	}

	initBoidSimulation();
}
