#include <random>
#include "animal-controller.h"
#include "skeleton/bone-controller.h"
#include "world/character-object.h"

using namespace regen;

AnimalController::AnimalController(
		const ref_ptr<Mesh> &mesh,
		const Indexed<ref_ptr<ModelTransformation> > &tfIndexed,
		const ref_ptr<BoneAnimationItem> &animItem,
		const ref_ptr<WorldModel> &world)
		: NonPlayerCharacterController(mesh, tfIndexed, animItem, world),
		  territoryBounds_(Vec2f::zero(), Vec2f::zero()) {
	// Try to get the world object representing this character.
	if (mesh->hasIndexedShapes()) {
		auto i_shape = mesh->indexedShape(tfIndexed.index);
		Resource *objRes = i_shape->worldObject();
		if (objRes) {
			WorldObject *wo = dynamic_cast<WorldObject *>(objRes);
			if (!wo) {
				REGEN_WARN("World object resource is not a WorldObject in NPC controller.");
			} else {
				wo->setObjectType(ObjectType::ANIMAL);
				wo->setStatic(false);
				knowledgeBase_.setCharacterObject(wo);
			}
		}
	}
	// Try to get the world object representing this character.
	if (!knowledgeBase_.characterObject()) {
		// auto create a world object for this character
		auto wo = ref_ptr<AnimalObject>::alloc(REGEN_STRING("npc-" << tfIndexed.index));
		wo->setPosition(Vec3f::zero());
		wo->setKnowledgeBase(&knowledgeBase_);
		wo->setNPCController(this);
		knowledgeBase_.setCharacterObject(wo.get());
		worldModel_->addWorldObject(wo);
	} else {
		auto *wo = dynamic_cast<CharacterObject *>(knowledgeBase_.characterObject());
		if (wo) {
			wo->setKnowledgeBase(&knowledgeBase_);
			wo->setNPCController(this);
		} else {
			REGEN_WARN("World object in NPC controller is not a CharacterObject.");
		}
	}
	// create a custom place representing the home territory of this animal
	animalTerritory_ = ref_ptr<Place>::alloc(
		REGEN_STRING("territory-npc-" << tfIndexed.index), PlaceType::HOME);
	knowledgeBase_.setCurrentPlace(animalTerritory_);
	animalTerritory_->setRadius(50.0f);
	animalTerritory_->setPosition(Vec3f::zero());
}

void AnimalController::setTerritoryBounds(const Vec2f &center, const Vec2f &size) {
	auto halfSize = size * 0.5f;
	territoryBounds_.min = center - halfSize;
	territoryBounds_.max = center + halfSize;
	animalTerritory_->setRadius(sqrtf(halfSize.x * halfSize.x + halfSize.y * halfSize.y));
	animalTerritory_->setPosition(Vec3f(center.x, 0.0f, center.y));
}

void intersectWithBounds(Vec2f& point, const Vec2f& origin, const Bounds<Vec2f>& territoryBounds) {
        if (point.x < territoryBounds.min.x) {
            float t = (territoryBounds.min.x - origin.x) / (point.x - origin.x);
            point.x = territoryBounds.min.x;
            point.y = origin.y + t * (point.y - origin.y);
        } else if (point.x > territoryBounds.max.x) {
            float t = (territoryBounds.max.x - origin.x) / (point.x - origin.x);
            point.x = territoryBounds.max.x;
            point.y = origin.y + t * (point.y - origin.y);
        }
        if (point.y < territoryBounds.min.y) {
            float t = (territoryBounds.min.y - origin.y) / (point.y - origin.y);
            point.y = territoryBounds.min.y;
            point.x = origin.x + t * (point.x - origin.x);
        } else if (point.y > territoryBounds.max.y) {
            float t = (territoryBounds.max.y - origin.y) / (point.y - origin.y);
            point.y = territoryBounds.max.y;
            point.x = origin.x + t * (point.x - origin.x);
        }
}

void AnimalController::updateController(double dt) {
	NonPlayerCharacterController::updateController(dt);
}


std::vector<ref_ptr<AnimalController>> AnimalController::load(
		LoadingContext &ctx, scene::SceneInputNode &node) {
	std::vector<ref_ptr<AnimalController> > controllerList;
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
		REGEN_WARN("Unable to find mesh for NPC controller in '" << node.getDescription() << "'.");
		return controllerList;
	}

	ref_ptr<ModelTransformation> tf;
	if (node.hasAttribute("transform")) {
		tf = scene->getResources()->getTransform(scene, node.getValue("transform"));
	} else if (node.hasAttribute("tf")) {
		tf = scene->getResources()->getTransform(scene, node.getValue("tf"));
	}
	if (tf.get() == nullptr) {
		REGEN_WARN("Unable to find transform for '" << node.getDescription() << "'.");
		return controllerList;
	}

	ref_ptr<SpatialIndex> spatialIndex;
	if (node.hasAttribute("spatial-index")) {
		spatialIndex = scene->getResources()->getIndex(scene, node.getValue("spatial-index"));
		if (!spatialIndex.get()) {
			REGEN_WARN("Unable to find spatial index for controller '" << node.getDescription() << "'.");
		}
	}
	std::string indexedShapeName = node.getValue("indexed-shape");
	if (spatialIndex.get() && indexedShapeName.empty()) {
		REGEN_WARN("Spatial index specified but no indexed shape name given for controller '"
			<< node.getDescription() << "'.");
		return controllerList;
	}

	auto animAssetName = node.getValue("animation-asset");
	auto animItem = scene->getAnimationRanges(animAssetName);
	if (!animItem) {
		REGEN_WARN("Unable to find animation asset for animation with name '"
			<< animAssetName << "' in controller '" << node.getDescription() << "'.");
		return controllerList;
	}

	controllerList.resize(tf->numInstances());
	for (int32_t tfIdx = 0; tfIdx < tf->numInstances(); tfIdx++) {
		Indexed<ref_ptr<ModelTransformation> > indexedTF(tf, tfIdx);
		auto controller = ref_ptr<AnimalController>::alloc(
			mesh, indexedTF, animItem, scene->worldModel());
		controllerList[tfIdx] = controller;

		controller->setWorldTime(&scene->application()->worldTime());
		if (spatialIndex.get()) {
			auto indexedShape = spatialIndex->getShape(indexedShapeName, tfIdx);
			if (!indexedShape) {
				REGEN_WARN("Unable to find indexed shape '" << indexedShapeName
					<< "' in spatial index for controller '" << node.getDescription() << "'.");
			} else {
				auto perceptionSystem = std::make_unique<PerceptionSystem>(spatialIndex, indexedShape);
				perceptionSystem->setCollisionBit(node.getValue<uint32_t>("collision-bit", 0));
				controller->setPerceptionSystem(std::move(perceptionSystem));
			}
		}
		if (node.hasAttribute("decision-interval")) {
			controller->setDecisionInterval(node.getValue<float>("decision-interval", 0.25f));
		}
		if (node.hasAttribute("perception-interval")) {
			controller->setPerceptionInterval(node.getValue<float>("perception-interval", 0.1f));
		}
		controller->setWalkSpeed(node.getValue<float>("walk-speed", 0.05f));
		controller->setRunSpeed(node.getValue<float>("run-speed", 0.1f));
		controller->setMaxTurnDegPerSecond(node.getValue<float>("max-turn-angle", 90.0f));
		if (node.hasAttribute("personal-space")) {
			controller->setPersonalSpace(node.getValue<float>("personal-space", 4.5f));
		}
		if (node.hasAttribute("wall-avoidance")) {
			controller->setAvoidanceWeight(node.getValue<float>("avoidance-weight", 0.5f));
		}
		if (node.hasAttribute("wall-avoidance")) {
			controller->setWallAvoidance(node.getValue<float>("wall-avoidance", 1.0f));
		}
		if (node.hasAttribute("character-avoidance")) {
			controller->setCharacterAvoidance(node.getValue<float>("character-avoidance", 10.0f));
		}
		if (node.hasAttribute("cohesion-weight")) {
			controller->setCohesionWeight(node.getValue<float>("cohesion-weight", 1.0f));
		}
		if (node.hasAttribute("member-separation-weight")) {
			controller->setMemberSeparationWeight(node.getValue<float>("member-separation-weight", 1.0f));
		}
		if (node.hasAttribute("group-separation-weight")) {
			controller->setGroupSeparationWeight(node.getValue<float>("group-separation-weight", 5.0f));
		}
		if (node.hasAttribute("turn-personal-space")) {
			controller->setTurnFactorPersonalSpace(node.getValue<float>("turn-personal-space", 3.0f));
		}
		controller->setPushThroughDistance(node.getValue<float>("push-through-distance", 0.5f));
		controller->setLookAheadThreshold(node.getValue<float>("look-ahead-threshold", 6.0f));
		controller->setAvoidanceDecay(node.getValue<float>("avoidance-decay", 0.5f));
		controller->setWallTangentWeight(node.getValue<float>("wall-tangent-weight", 0.5f));
		controller->setVelOrientationWeight(node.getValue<float>("velocity-orientation-weight", 0.5f));
		controller->setFloorHeight(node.getValue<float>("floor-height", 0.0f));
		controller->setTerritoryBounds(
			node.getValue<Vec2f>("territory-center", Vec2f(0.0)),
			node.getValue<Vec2f>("territory-size", Vec2f(10.0)));
		if (node.hasAttribute("base-orientation")) {
			controller->setBaseOrientation(node.getValue<float>("base-orientation", 0.0f));
		}

		auto &kb = controller->knowledgeBase();
		kb.setWorldTime(&scene->application()->worldTime());
		// Set base time for actions/staying at a place
		kb.setBaseTimeActivity(node.getValue<float>("base-time-activity", 120.0f));
		kb.setBaseTimePlace(node.getValue<float>("base-time-place", 1200.0f));
		// Set randomized character traits.
		kb.setTraitStrength(Trait::LAZINESS, math::randomize(
			node.getValue<float>("laziness", 0.5f), 0.25f));
		kb.setTraitStrength(Trait::SPIRITUALITY, math::randomize(
			node.getValue<float>("spirituality", 0.5f), 0.25f));
		kb.setTraitStrength(Trait::ALERTNESS, math::randomize(
			node.getValue<float>("alertness", 0.5f), 0.25f));
		kb.setTraitStrength(Trait::BRAVERY, math::randomize(
			node.getValue<float>("bravery", 0.5f), 0.25f));
		kb.setTraitStrength(Trait::SOCIALABILITY, math::randomize(
			node.getValue<float>("sociability", 0.5f), 0.25f));

		if (node.hasAttribute("height-map")) {
			auto heightMap2d = scene->getResources()->getTexture2D(scene, node.getValue("height-map"));
			if (!heightMap2d) {
				REGEN_WARN("Unable to find height map in '" << node.getDescription() << "'.");
			} else {
				auto heightMap = ref_ptr<HeightMap>::dynamicCast(heightMap2d);
				if (!heightMap) {
					REGEN_WARN("Height map texture is not a height map in '" << node.getDescription() << "'.");
				} else {
					controller->setHeightMap(heightMap);
				}
			}
		}

		// Load a behavior tree.
		// NOTE: XML scene may define different behavior trees for different instances.
		for (const auto &btNodeXML: node.getChildren("behavior-tree")) {
			std::list<scene::IndexRange> indices = btNodeXML->getIndexSequence(tf->numInstances());
			for (auto &range: indices) {
				if (range.isWithinRange(tfIdx)) {
					auto importKey = btNodeXML->getValue("import");
					if (importKey.empty()) {
						if (!btNodeXML->getChildren().empty()) {
							auto btRootXML = btNodeXML->getChildren().front();
							auto btRoot = BehaviorTree::load(ctx, *btRootXML.get());
							controller->setBehaviorTree(std::move(btRoot));
						}
					} else {
						// Load behavior tree node as child of root node.
						auto btRoots = scene->getRoot()->getFirstChild("behavior-tree", importKey);
						if (!btRoots || btRoots->getChildren().empty()) {
							REGEN_WARN("Unable to find imported behavior tree '" << importKey
								<< "' for controller '" << node.getDescription() << "'.");
						} else {
							auto btRootXML = btRoots->getChildren().front();
							auto btRoot = BehaviorTree::load(ctx, *btRootXML.get());
							controller->setBehaviorTree(std::move(btRoot));
						}
					}
					break;
				}
			}
		}

		controller->initializeController();
	}

	return controllerList;
}
