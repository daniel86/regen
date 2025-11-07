#include "behavior-tree.h"
#include "behavior-actions.h"
#include "behavior-conditions.h"
#include "regen/utility/xml.h"

using namespace regen;

static std::unique_ptr<BehaviorTree::Condition> makeCondition(scene::SceneInputNode *xmlCond) {
	const auto category = xmlCond->getValue("type");
	bool negate = xmlCond->getValue("negate", false);
	bool exclusive = xmlCond->getValue("exclusive", false);
	std::unique_ptr<BehaviorTree::Condition> cond;

	if (!xmlCond->hasAttribute("type")) {
		REGEN_WARN("Ignoring " << xmlCond->getDescription() << ", missing 'type' attribute.");
		return {};
	}

	if (category == "predicate") {
		auto predicateName = xmlCond->getName();
		if (predicateName == "IsAtTargetPlace") {
			cond = std::make_unique<IsAtTargetPlace>();
		}
		else if (predicateName == "IsAtDesiredLocation") {
			cond = std::make_unique<IsAtDesiredLocation>();
		}
		else if (predicateName == "HasDesiredPlaceType") {
			const PlaceType placeType = xmlCond->getValue<PlaceType>("value", PlaceType::HOME);
			cond = std::make_unique<HasDesiredPlaceType>(placeType);
		}
		else if (predicateName == "HasDesiredActivity") {
			const ActionType actionType = xmlCond->getValue<ActionType>("value", ActionType::IDLE);
			cond = std::make_unique<HasDesiredActivity>(actionType);
		}
		else if (predicateName == "HasDesire") {
			cond = std::make_unique<HasDesire>();
		}
		else if (predicateName == "IsPartOfGroup") {
			cond = std::make_unique<IsPartOfGroup>();
		}
		else if (predicateName == "IsLastGroupMember") {
			cond = std::make_unique<IsLastGroupMember>();
		}
		else {
			REGEN_WARN("Ignoring " << xmlCond->getDescription() << ", unknown predicate '" << predicateName << "'.");
			return {};
		}
	} else if (category == "attribute") {
		CharacterAttribute attr = xmlCond->getValue<CharacterAttribute>(
			"name", CharacterAttribute::HEALTH);
		if (xmlCond->hasAttribute("value")) {
			float v = xmlCond->getValue<float>("value", 0.0f);
			cond = std::make_unique<HasAttributeValue>(attr, v);
		}
		else if (xmlCond->hasAttribute("min-value")) {
			float v = xmlCond->getValue<float>("min-value", 0.0f);
			cond = std::make_unique<IsAttributeAbove>(attr, v);
		}
		else if (xmlCond->hasAttribute("max-value")) {
			float v = xmlCond->getValue<float>("max-value", 0.0f);
			cond = std::make_unique<IsAttributeBelow>(attr, v);
		} else {
			REGEN_WARN("Ignoring " << xmlCond->getDescription() << ", missing value, min-value or max-value.");
		}
	}
	else if (category == "trait") {
		Trait trait = xmlCond->getValue<Trait>("name", Trait::BRAVERY);
		if (xmlCond->hasAttribute("value")) {
			float v = xmlCond->getValue<float>("value", 0.0f);
			cond = std::make_unique<HasTraitValue>(trait, v);
		}
		else if (xmlCond->hasAttribute("min-value")) {
			float v = xmlCond->getValue<float>("min-value", 0.0f);
			cond = std::make_unique<IsTraitAbove>(trait, v);
		}
		else if (xmlCond->hasAttribute("max-value")) {
			float v = xmlCond->getValue<float>("max-value", 0.0f);
			cond = std::make_unique<IsTraitBelow>(trait, v);
		} else {
			REGEN_WARN("Ignoring " << xmlCond->getDescription() << ", missing value, min-value or max-value.");
		}
	} else if (category == "disjunction") {
		auto selection = std::make_unique<BehaviorConditionSelection>();
		for (auto &childNode : xmlCond->getChildren("condition")) {
			auto childCond = makeCondition(childNode.get());
			if (childCond) {
				selection->addCondition(std::move(childCond));
			} else {
				REGEN_WARN("Ignoring child condition in " << xmlCond->getDescription() << ".");
			}
		}
		cond = std::move(selection);
	} else {
		REGEN_WARN("Ignoring " << xmlCond->getDescription() << ", unknown condition type '" << category << "'.");
	}
	if (cond) {
		cond->setNegated(negate);
		cond->setExclusive(exclusive);
		if (xmlCond->hasAttribute("on-failure")) {
			auto status = xmlCond->getValue<BehaviorStatus>("on-failure", BehaviorStatus::FAILURE);
			cond->setFailureStatus(status);
		}
	}
	return cond;
}

static BehaviorTree::Node* addConditionDecorator(
	LoadingContext &ctx,
	scene::SceneInputNode &xmlNode,
	BehaviorTree::Node *node) {
	std::vector<std::unique_ptr<BehaviorTree::Condition>> conditions;

	auto conditionChildren = xmlNode.getChildren("condition");
	for (auto &xmlCond : conditionChildren) {
		auto cond = makeCondition(xmlCond.get());
		if (cond) {
			conditions.push_back(std::move(cond));
		} else {
			REGEN_WARN("Ignoring child condition in " << xmlNode.getDescription() << ".");
		}
	}
	if (conditions.size() > 1) {
		BehaviorConditionSequence *seq = new BehaviorConditionSequence();
		for (auto &c : conditions) {
			seq->addCondition(std::move(c));
		}
		std::unique_ptr<BehaviorTree::Condition> seqPtr(seq);
		std::unique_ptr<BehaviorTree::Node> nodePtr(node);
		return new BehaviorConditionNode(std::move(seqPtr), std::move(nodePtr));
	} else if (conditions.size() == 1) {
		std::unique_ptr<BehaviorTree::Node> nodePtr(node);
		return new BehaviorConditionNode(std::move(conditions[0]), std::move(nodePtr));
	} else {
		return node;
	}
}

static BehaviorTree::Node* loadNode(LoadingContext &ctx, scene::SceneInputNode &xmlNode, BehaviorTree::Node *parent) {
	const std::string category = xmlNode.getCategory();
	BehaviorTree::Node *node = nullptr;

	if (category == "sequence") {
		auto seqNode = new BehaviorSequenceNode();
		node = addConditionDecorator(ctx, xmlNode, seqNode);
		for (auto &child : xmlNode.getChildren()) {
			if (child->getCategory() == "condition") continue; // already processed
			if (!loadNode(ctx, *child.get(), seqNode)) {
				REGEN_WARN("Failed to load child node in '" << child->getDescription() << "'.");
			}
		}
	}
	else if (category == "selector") {
		if (xmlNode.getValue("type") == "priority") {
			auto priNode = new BehaviorPriorityNode();
			node = addConditionDecorator(ctx, xmlNode, priNode);
			for (auto &child : xmlNode.getChildren()) {
				if (child->getCategory() == "condition") continue;
				if (!loadNode(ctx, *child.get(), priNode)) {
					REGEN_WARN("Failed to load child node in '" << child->getDescription() << "'.");
				}
			}
		}
		else if (xmlNode.getValue("type") == "random") {
			auto selNode = new BehaviorRandomSelectorNode();
			node = addConditionDecorator(ctx, xmlNode, selNode);
			for (auto &child : xmlNode.getChildren()) {
				if (child->getCategory() == "condition") continue;
				if (!loadNode(ctx, *child.get(), selNode)) {
					REGEN_WARN("Failed to load child node in '" << child->getDescription() << "'.");
				}
			}
		}
		else {
			auto selNode = new BehaviorPriorityNode();
			node = addConditionDecorator(ctx, xmlNode, selNode);
			for (auto &child : xmlNode.getChildren()) {
				if (child->getCategory() == "condition") continue;
				if (!loadNode(ctx, *child.get(), selNode)) {
					REGEN_WARN("Failed to load child node in '" << child->getDescription() << "'.");
				}
			}
		}
	}
	else if (category == "parallel") {
		auto successMode = xmlNode.getValue<BehaviorParallelNode::StatusMode>(
			"success", BehaviorParallelNode::ALL);
		auto failureMode = xmlNode.getValue<BehaviorParallelNode::StatusMode>(
			"failure", BehaviorParallelNode::ANY);
		auto parNode = new BehaviorParallelNode(successMode, failureMode);
		node = addConditionDecorator(ctx, xmlNode, parNode);
		for (auto &child : xmlNode.getChildren()) {
			if (child->getCategory() == "condition") continue;
			if (!loadNode(ctx, *child.get(), parNode)) {
				REGEN_WARN("Failed to load child node in '" << child->getDescription() << "'.");
			}
		}
	}
	else if (category == "action") {
		auto actionType = xmlNode.getValue("type");
		BehaviorActionNode *actNode = nullptr;
		if (actionType == "SelectTargetPlace") {
			auto placeType = xmlNode.getValue<PlaceType>("place-type", PlaceType::LAST);
			actNode = new SelectTargetPlace(placeType);
		}
		else if (actionType == "SelectPlaceActivity") {
			actNode = new SelectPlaceActivity();
		}
		else if (actionType == "SetDesiredAction" || actionType == "SetDesiredActivity") {
			auto action = xmlNode.getValue<ActionType>("value", ActionType::IDLE);
			auto n = new SetDesiredActivity(action);
			if (xmlNode.hasAttribute("min-duration") || xmlNode.hasAttribute("max-duration")) {
				float minDur = xmlNode.getValue<float>("min-duration", 0.0f);
				float maxDur = xmlNode.getValue<float>("max-duration", minDur);
				n->setDurationRange(minDur, maxDur);
			}
			else if (xmlNode.hasAttribute("duration")) {
				float dur = xmlNode.getValue<float>("duration", 0.0f);
				n->setFixedDuration(dur);
			}
			actNode = n;
		}
		else if (actionType == "SelectPlacePatient") {
			actNode = new SelectPlacePatient();
		}
		else if (actionType == "SelectPlaceLocation") {
			actNode = new SelectPlaceLocation();
		}
		else if (actionType == "UnsetPatient") {
			actNode = new UnsetPatient();
		}
		else if (actionType == "MoveToTargetPoint") {
			auto *n = new MoveToTargetPoint();
			actNode = n;
			if (xmlNode.hasAttribute("reach-radius")) {
				n->setReachRadius(xmlNode.getValue<float>("reach-radius", 0.5f));
			}
		}
		else if (actionType == "MoveToTargetPlace") {
			actNode = new MoveToTargetPlace();
		}
		else if (actionType == "MoveToLocation") {
			actNode = new MoveToLocation();
		}
		else if (actionType == "MoveToGroup") {
			actNode = new MoveToGroup();
		}
		else if (actionType == "FormLocationGroup") {
			actNode = new FormLocationGroup();
		}
		else if (actionType == "LeaveGroup") {
			actNode = new LeaveGroup();
		}
		else if (actionType == "LeaveLocation") {
			actNode = new LeaveLocation();
		}
		else if (actionType == "MoveToPatient") {
			actNode = new MoveToPatient();
		}
		else if (actionType == "PerformAction") {
			auto performNode = new PerformAction(xmlNode.getValue("value", ActionType::IDLE));
			if (xmlNode.hasAttribute("max-duration")) {
				performNode->setMaxDuration(xmlNode.getValue<float>("max-duration", 0.0f));
			}
			actNode = performNode;
		}
		else if (actionType == "PerformDesiredAction") {
			actNode = new PerformDesiredAction();
		}
		else if (actionType == "PerformAffordedAction") {
			actNode = new PerformAffordedAction();
		}
		else if (actionType == "SetRoamingTarget") {
			actNode = new SetRoamingTarget();
		}
		else if (actionType == "SetTargetPlace") {
			auto wo = ctx.scene()->getResource<WorldObjectVec>(xmlNode.getValue("value"));
			if (!wo || wo->empty()) {
				REGEN_WARN("Cannot find world object in '" << xmlNode.getDescription() << "'.");
				return nullptr;
			}
			auto place = ref_ptr<Place>::dynamicCast(wo->front());
			if (!place) {
				REGEN_WARN("Ignoring " << xmlNode.getDescription() <<
					", object '" << xmlNode.getValue("value") << "' is not a place.");
				return nullptr;
			}
			node = new SetTargetPlace(place);
		} else if (actionType == "SetPatient") {
			auto wo = ctx.scene()->getResource<WorldObjectVec>(xmlNode.getValue("value"));
			if (!wo || wo->empty()) {
				REGEN_WARN("Cannot find world object in '" << xmlNode.getDescription() << "'.");
				return nullptr;
			}
			node = new SetPatient(wo.get(), xmlNode.getValue("affordance"));
		}
		else {
			REGEN_WARN("Ignoring " << xmlNode.getDescription() << ", unknown action type '" << actionType << "'.");
		}
		if (actNode != nullptr) {
			node = addConditionDecorator(ctx, xmlNode, actNode);
		}
	}

	if (node == nullptr) {
		REGEN_WARN("Unknown behavior tree node type '" << category << "' in '" << xmlNode.getDescription() << "'.");
		return nullptr;
	}
	if (parent != nullptr) {
		parent->addChild(std::unique_ptr<BehaviorTree::Node>(node));
	}
	return node;
}

std::unique_ptr<BehaviorTree::Node> BehaviorTree::load(LoadingContext &ctx, scene::SceneInputNode &xmlNode) {
	BehaviorTree::Node *node = loadNode(ctx, xmlNode, nullptr);
	if (node == nullptr) {
		return {};
	} else {
		return std::unique_ptr<BehaviorTree::Node>(node);
	}
}


std::ostream &regen::operator<<(std::ostream &out, const BehaviorParallelNode::StatusMode &v) {
	switch (v) {
		case BehaviorParallelNode::ALL:
			return out << "ALL";
		case BehaviorParallelNode::ANY:
			return out << "ANY";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BehaviorParallelNode::StatusMode &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "ALL") v = BehaviorParallelNode::ALL;
	else if (val == "ANY") v = BehaviorParallelNode::ANY;
	else {
		REGEN_WARN("Unknown parallel status mode '" << val << "'. Using ALL.");
		v = BehaviorParallelNode::ALL;
	}
	return in;
}


std::ostream &regen::operator<<(std::ostream &out, const BehaviorStatus &v) {
	switch (v) {
		case BehaviorStatus::SUCCESS:
			return out << "SUCCESS";
		case BehaviorStatus::FAILURE:
			return out << "FAILURE";
		case BehaviorStatus::RUNNING:
			return out << "RUNNING";
	}
	return out;
}

std::istream &regen::operator>>(std::istream &in, BehaviorStatus &v) {
	std::string val;
	in >> val;
	boost::to_upper(val);
	if (val == "SUCCESS") v = BehaviorStatus::SUCCESS;
	else if (val == "FAILURE") v = BehaviorStatus::FAILURE;
	else if (val == "RUNNING") v = BehaviorStatus::RUNNING;
	else {
		REGEN_WARN("Unknown behavior status '" << val << "'. Using FAILURE.");
		v = BehaviorStatus::FAILURE;
	}
	return in;
}
