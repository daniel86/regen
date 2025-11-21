#include "world-object.h"
#include <regen/objects/mesh.h>

#include "character-object.h"
#include "regen/scene/resource-manager.h"

using namespace regen;

WorldObject::WorldObject(std::string_view name) : name_(name) {
	getPosition2D_ = nullptr;
	getPosition3D_ = nullptr;
}

WorldObject::~WorldObject() {
	if (shape_.get()) {
		shape_->unsetWorldObject(this);
	}
}

void WorldObject::setShape(const ref_ptr<BoundingShape> &shape, uint32_t instanceIdx) {
	shape_ = shape;
	instanceIdx_ = instanceIdx;
	getPosition2D_ = &WorldObject::position2D_shape;
	getPosition3D_ = &WorldObject::position3D_shape;
	shape_->setWorldObject(this);
	for (auto &aff: affordances_) {
		aff->initialize();
	}
}

void WorldObject::setPosition(const ref_ptr<ShaderInput3f> &position) {
	pos_ = position;
	getPosition2D_ = &WorldObject::position2D_local;
	getPosition3D_ = &WorldObject::position3D_local;
	if (shape_.get()) {
		shape_->unsetWorldObject(this);
		shape_ = {};
	}
	for (auto &aff: affordances_) {
		aff->initialize();
	}
}

void WorldObject::setPosition(const Vec3f &position) {
	if (!pos_) {
		pos_ = createUniform<ShaderInput3f>("pos", position);
	} else {
		pos_->setVertex(0, position);
	}
	getPosition2D_ = &WorldObject::position2D_local;
	getPosition3D_ = &WorldObject::position3D_local;
	if (shape_.get()) {
		shape_->unsetWorldObject(this);
		shape_ = {};
	}
	for (auto &aff: affordances_) {
		aff->initialize();
	}
}

Vec2f WorldObject::position2D() const {
	return (this->*getPosition2D_)();
}

Vec3f WorldObject::position3D() const {
	return (this->*getPosition3D_)();
}

Vec2f WorldObject::position2D_shape() const {
	const Vec3f &p = shape_->tfOrigin();
	return { p.x, p.z };
}

Vec3f WorldObject::position3D_shape() const {
	return shape_->tfOrigin();
}

Vec2f WorldObject::position2D_local() const {
	const Vec3f &p = pos_->getVertex(0).r;
	return { p.x, p.z };
}

Vec3f WorldObject::position3D_local() const {
	return pos_->getVertex(0).r;
}

void WorldObject::addAffordance(const ref_ptr<Affordance> &affordance) {
	affordances_.push_back(affordance);
	if (getPosition2D_) {
		affordance->initialize();
	}
}

bool WorldObject::hasAffordance(ActionType actionType) const {
	for (const auto &aff: affordances_) {
		if (aff->type == actionType) {
			return true;
		}
	}
	return false;
}

ref_ptr<Affordance> WorldObject::getAffordance(ActionType actionType, std::string_view affordanceName) const {
	for (const auto &aff: affordances_) {
		if (aff->type != actionType) continue;
		if (!affordanceName.empty() && aff->name != affordanceName) {
			continue;
		}
		return aff;
	}
	return {};
}

bool WorldObject::isFriendly(const WorldObject &other) const {
	return (factionMask_ != 0 && other.factionMask_ != 0 &&
		(factionMask_ & other.factionMask_) != 0);
}

void WorldObject::setCurrentLocation(const ref_ptr<Location> &location) {
	currentLocation_ = location;
}

void WorldObject::unsetCurrentLocation() {
	currentLocation_ = {};
	currentLocationIdx_ = -1;
	leaveCurrentGroup();
}

void WorldObject::setCurrentLocationIndex(int32_t locIdx) {
	currentLocationIdx_ = locIdx;
}

void WorldObject::leaveCurrentGroup() {
	if (currentGroup_.get()) {
		auto group = currentGroup_;
		currentGroup_->removeMember(this);
		currentGroup_ = {};
		if (group->numMembers() == 0) {
			// remove empty group from location
			if (group->currentLocation().get()) {
				group->currentLocation()->removeGroup(group.get());
				group->setCurrentLocation({});
			}
		}
	}
}

void WorldObject::joinGroup(const ref_ptr<ObjectGroup> &group) {
	if (currentGroup_.get() == group.get()) {
		return; // already in this group
	}
	leaveCurrentGroup();
	if (group.get()) {
		// note: the group sets group index on world object.
		//   it moves around items and then needs to call the set index function.
		group->addMember(this);
	}
	currentGroup_ = group;
}

static void loadChildren(LoadingContext &ctx, scene::SceneInputNode &n, const ref_ptr<WorldObject> &wo) {
	auto world = ctx.scene()->getResources()->getWorldModel();
	// load child objects
	for (const auto &childNode: n.getChildren("object")) {
		auto childObj = WorldObject::load(ctx, *childNode.get());
		switch (wo->objectType()) {
			case ObjectType::LOCATION:
				childObj->setCurrentLocation(ref_ptr<Location>::dynamicCast(wo));
				if (wo->placeOfObject().get()) {
					ref_ptr<Place> place = ref_ptr<Place>::dynamicCast(wo->placeOfObject());
					if (place.get()) {
						place->addWorldObject(childObj);
						childObj->setPlaceOfObject(place);
					}
				}
				break;
			case ObjectType::PLACE:
				childObj->setPlaceOfObject(wo);
				break;
			case ObjectType::COLLECTION: {
				auto group = ref_ptr<ObjectGroup>::dynamicCast(wo);
				if (group.get()) {
					childObj->joinGroup(group);
				}
				break;
			}
			default:
				break;
		}
		world->addWorldObject(childObj);
	}
}

ref_ptr<WorldObject> WorldObject::load(LoadingContext &ctx, scene::SceneInputNode &n) {
	ObjectType type = ObjectType::THING;
	if (n.hasAttribute("type")) {
		type = n.getValue<ObjectType>("type", ObjectType::THING);
	}
	ref_ptr<WorldObject> wo;
	if (type == ObjectType::CHARACTER) {
		wo = ref_ptr<PersonObject>::alloc(n.getName());
	} else if (type == ObjectType::ANIMAL) {
		wo = ref_ptr<AnimalObject>::alloc(n.getName());
	} else if (type == ObjectType::PLAYER) {
		wo = ref_ptr<PlayerObject>::alloc(n.getName());
	} else if (type == ObjectType::LOCATION) {
		wo = ref_ptr<Location>::alloc(n.getName());
	} else {
		wo = ref_ptr<WorldObject>::alloc(n.getName());
		wo->setObjectType(type);
	}
	if (n.hasAttribute("static")) {
		wo->setStatic(n.getValue<bool>("static", true));
	}
	wo->setDanger(n.getValue<float>("danger", 0.0f));
	wo->setRadius(n.getValue<float>("radius", 1.0f));

	// Load objects that are part of this world object.
	loadChildren(ctx, n, wo);

	// load affordances
	for (const auto &a: n.getChildren("affordance")) {
		ref_ptr<Affordance> affordance = ref_ptr<Affordance>::alloc(wo);
		affordance->type = a->getValue<ActionType>("type", ActionType::IDLE);
		affordance->minDistance = a->getValue<float>("min-distance", 0.0f);
		if (a->hasAttribute("max-participants")) {
			affordance->slotCount = a->getValue<int>("max-participants", 1);
		} else {
			affordance->slotCount = a->getValue<int>("num-slots", 1);
		}
		affordance->spacing = a->getValue<float>("spacing", 2.0f);
		affordance->layout = a->getValue<SlotLayout>("slot-layout", SlotLayout::CIRCULAR);
		affordance->radius = a->getValue<float>("radius", 1.0f);
		affordance->baseOffset = a->getValue<Vec3f>("base-offset", Vec3f(0.0f, 0.0f, 0.0f));
		affordance->name = a->getName();
		wo->addAffordance(affordance);
	}

	// load factions
	if (n.hasAttribute("factions")) {
		const auto indicesAtt = n.getValue<std::string>("factions", "0");
		std::vector<std::string> indicesStr;
		boost::split(indicesStr, indicesAtt, boost::is_any_of(","));
		for (auto & it : indicesStr) {
			int i = atoi(it.c_str());
			if (i >= 0 && i < 32) {
				wo->addFaction(1u << i);
			} else {
				REGEN_WARN("Ignoring " << n.getDescription() << ", invalid faction index '" << it << "'.");
			}
		}
	} else if (n.hasAttribute("faction")) {
		int i = n.getValue<int>("faction", -1);
		if (i >= 0 && i < 32) {
			wo->addFaction(1u << i);
		} else {
			REGEN_WARN("Ignoring " << n.getDescription() << ", invalid faction index '" << i << "'.");
		}
	}

	ctx.scene()->getResources()->getWorldModel()->addWorldObject(wo);
	return wo;
}

ref_ptr<WorldObjectVec> WorldObjectVec::load(LoadingContext &ctx, scene::SceneInputNode &n) {
	ref_ptr<WorldObjectVec> objVec = ref_ptr<WorldObjectVec>::alloc();

	if (n.hasAttribute("mesh")) {
		uint32_t meshIdx = n.getValue<uint32_t>("mesh-index", 0u);
		auto compositeMesh = ctx.scene()->getResources()->getMesh(ctx.scene(), n.getValue("mesh"));
		if (!compositeMesh || compositeMesh->meshes().empty()) {
			REGEN_WARN("Cannot find mesh in `" << n.getDescription() << "`.");
		} else if (meshIdx >= compositeMesh->meshes().size()) {
			REGEN_WARN("Invalid mesh index in `" << n.getDescription() << "`.");
		} else {
			auto mesh = compositeMesh->meshes()[meshIdx];
			auto shape = mesh->indexedShape(0);
			uint32_t numInstances = shape->transform()->numInstances();
			// load range of instances
			std::list<scene::IndexRange> instanceRange = n.getIndexSequence(numInstances);
			for (const auto &r: instanceRange) {
				for (uint32_t instance = r.from; instance <= r.to; instance += r.step) {
					auto wo = WorldObject::load(ctx, n);
					wo->setShape(mesh->indexedShape(instance), instance);

					// Add to semantic places.
					auto worldModel = ctx.scene()->getResources()->getWorldModel();
					for (const auto &place: worldModel->places) {
						if ((wo->position2D() - place->position2D()).lengthSquared() <= place->radius() * place->radius()) {
							place->addWorldObject(wo);
							wo->setPlaceOfObject(place);
							break;
						}
					}

					objVec->push_back(wo);
				}
			}
		}
	}
	if (objVec->empty()) {
		return {};
	}
	ctx.scene()->putResource<WorldObjectVec>(n.getName(), objVec);
	return objVec;
}

std::ostream &regen::operator<<(std::ostream &out, const ObjectType &v) {
	switch (v) {
		case ObjectType::THING:
			return out << "THING";
		case ObjectType::PLACE:
			return out << "PLACE";
		case ObjectType::LOCATION:
			return out << "LOCATION";
		case ObjectType::WAYPOINT:
			return out << "WAYPOINT";
		case ObjectType::CHARACTER:
			return out << "CHARACTER";
		case ObjectType::ANIMAL:
			return out << "ANIMAL";
		case ObjectType::PLAYER:
			return out << "PLAYER";
		case ObjectType::COLLECTION:
			return out << "COLLECTION";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, ObjectType &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "THING") v = ObjectType::THING;
	else if (val == "PLACE") v = ObjectType::PLACE;
	else if (val == "LOCATION") v = ObjectType::LOCATION;
	else if (val == "WAYPOINT") v = ObjectType::WAYPOINT;
	else if (val == "CHARACTER") v = ObjectType::CHARACTER;
	else if (val == "ANIMAL") v = ObjectType::ANIMAL;
	else if (val == "PLAYER") v = ObjectType::PLAYER;
	else if (val == "COLLECTION") v = ObjectType::COLLECTION;
	else {
		REGEN_WARN("Unknown object type '" << val << "'. Using THING.");
		v = ObjectType::THING;
	}
	return in;
}
