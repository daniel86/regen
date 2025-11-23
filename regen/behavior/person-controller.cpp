#include <random>
#include "person-controller.h"

#include "world/character-object.h"

using namespace regen;

PersonController::PersonController(
		const ref_ptr<Mesh> &mesh,
		const Indexed<ref_ptr<ModelTransformation> > &tfIndexed,
		const ref_ptr<BoneAnimationItem> &animItem,
		const ref_ptr<WorldModel> &world)
		: NonPlayerCharacterController(mesh, tfIndexed, animItem, world) {
	// Try to get the world object representing this character.
	if (mesh->hasIndexedShapes()) {
		auto i_shape = mesh->indexedShape(tfIndexed.index);
		Resource *objRes = i_shape->worldObject();
		if (objRes) {
			WorldObject *wo = dynamic_cast<WorldObject *>(objRes);
			if (!wo) {
				REGEN_WARN("World object resource is not a WorldObject in NPC controller.");
			} else {
				wo->setObjectType(ObjectType::CHARACTER);
				wo->setStatic(false);
				knowledgeBase_.setCharacterObject(wo);
			}
		}
	}
	// Try to get the world object representing this character.
	if (!knowledgeBase_.characterObject()) {
		// auto create a world object for this character
		auto wo = ref_ptr<PersonObject>::alloc(REGEN_STRING("npc-" << tfIndexed.index));
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
}

void PersonController::initializeController() {
	uint32_t initialIdleTime = math::random<float>() * 10.0f + 1.0f;
	knowledgeBase_.setLingerTime(initialIdleTime);
	knowledgeBase_.setActivityTime(initialIdleTime);
	// Start with idle animation
	setIdle();
	startAnimation();
}

void PersonController::setWeaponMesh(const ref_ptr<Mesh> &mesh) {
	weaponMesh_ = mesh;
	weaponShape_ = mesh->indexedShape(tfIdx_);
	if (!weaponShape_) {
		REGEN_WARN("Unable to find weapon shape in NPC controller.");
	}
	hasDrawnWeapon_ = true;
	hideWeapon();
}

void PersonController::hideWeapon() {
	if (hasDrawnWeapon_) {
		hasDrawnWeapon_ = false;
		if (weaponShape_.get()) {
			weaponMask_ = weaponShape_->traversalMask();
			weaponShape_->setTraversalMask(0);
		}
	}
}

void PersonController::drawWeapon() {
	if (!hasDrawnWeapon_) {
		hasDrawnWeapon_ = true;
		if (weaponShape_.get()) {
			weaponShape_->setTraversalMask(weaponMask_);
		}
	}
}

void PersonController::updateController(double dt) {
	auto &kb = knowledgeBase_;
	auto &anim = animItem_->boneTree;

	if (footstepTrail_.get() && isLastAnimationMovement_) {
		int32_t walkHandle = boneController_->getAnimationHandle(MotionType::WALK);
		int32_t runHandle = boneController_->getAnimationHandle(MotionType::RUN);
		int32_t rangeIdx = std::max(walkHandle, runHandle);
		if (rangeIdx != -1) {
			float movementTime = (walkHandle != -1 ? boneController_->walkTime() : boneController_->runTime());
			auto elapsed = anim->elapsedTime(tfIdx_, rangeIdx);
			if (elapsed < footLastElapsed_) {
				// animation looped, reset footstep flags
				footDown_[0] = false;
				footDown_[1] = false;
			}
			footLastElapsed_ = elapsed;
			for (int i = 0; i < 2; i++) {
				if (!footDown_[i] && elapsed > footTime_[i] * movementTime) {
					footDown_[i] = true;
					footstepTrail_->insertBlanket(currentPos_, currentDir_, footIdx_[i]);
				}
			}
		}
	}

	NonPlayerCharacterController::updateController(dt);

	if (weaponMesh_.get()) {
		bool useWeapon = kb.isWeaponRequired();
		if (useWeapon && !hasDrawnWeapon_) {
			drawWeapon();
		} else if (!useWeapon && hasDrawnWeapon_) {
			hideWeapon();
		}
	}
}

std::vector<ref_ptr<PersonController>> PersonController::load(
		LoadingContext &ctx, scene::SceneInputNode &node) {
	std::vector<ref_ptr<PersonController> > controllerList;
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

	// load mesh for footsteps, if any
	auto footstepVec = scene->getResources()->getMesh(
		scene, node.getValue("footstep-mesh"));
	ref_ptr<BlanketTrail> footstepTrail;
	if (footstepVec.get() && !footstepVec->meshes().empty()) {
		auto footstepMesh = footstepVec->meshes()[0];
		footstepTrail = ref_ptr<BlanketTrail>::dynamicCast(footstepMesh);
		if (!footstepTrail) {
			REGEN_WARN("Footstep mesh is not a BlanketTrail in '" << node.getDescription() << "'.");
		}
	} else if (node.hasAttribute("footstep-mesh")) {
		REGEN_WARN("Unable to find footstep mesh for NPC controller in '" << node.getDescription() << "'.");
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
		auto controller = ref_ptr<PersonController>::alloc(
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
		if (footstepTrail.get()) {
			float leftFootTime = node.getValue<float>("left-foot-time", 0.25f);
			float rightFootTime = node.getValue<float>("right-foot-time", 0.8f);
			controller->setFootstepTrail(footstepTrail, leftFootTime, rightFootTime);
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

		// Set the weapon mesh is any.
		auto weaponMeshVec = scene->getResources()->getMesh(scene, node.getValue("weapon-mesh"));
		if (weaponMeshVec.get() != nullptr && !weaponMeshVec->meshes().empty()) {
			uint32_t weaponMeshIdx = node.getValue<uint32_t>("weapon-mesh-index", 0u);
			if (weaponMeshIdx >= weaponMeshVec->meshes().size()) {
				REGEN_WARN("Invalid weapon mesh index for '" << node.getDescription() << "'.");
				weaponMeshIdx = 0;
			}
			auto weaponMesh = weaponMeshVec->meshes()[weaponMeshIdx];
			if (weaponMesh.get()) {
				controller->setWeaponMesh(weaponMesh);
			} else {
				REGEN_WARN("Unable to find weapon mesh in '" << node.getDescription() << "'.");
			}
		} else if (node.hasAttribute("weapon-mesh")) {
			REGEN_WARN("Unable to find weapon mesh in '" << node.getDescription() << "'.");
		}

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
			bool success = false;
			for (auto &range: indices) {
				if (range.isWithinRange(tfIdx)) {
					auto importKey = btNodeXML->getValue("import");
					if (importKey.empty()) {
						if (!btNodeXML->getChildren().empty()) {
							auto btRootXML = btNodeXML->getChildren().front();
							auto btRoot = BehaviorTree::load(ctx, *btRootXML.get());
							controller->setBehaviorTree(std::move(btRoot));
							success = true;
							break;
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
							success = true;
							break;
						}
					}
				}
			}
			if (success) {
				break;
			}
		}

		controller->initializeController();
	}

	return controllerList;
}
