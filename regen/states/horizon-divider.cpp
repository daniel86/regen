#include "horizon-divider.h"
#include "regen/scene/node-processor.h"

using namespace regen;

HorizonDividerNode::HorizonDividerNode(const ref_ptr<Camera> &camera)
		: StateNode(), camera_(camera) {
}

bool HorizonDividerNode::isBelowHorizon() const {
	return (camera_->position()[0].y < surfaceHeight_);
}

void HorizonDividerNode::setBelowHorizonNode(const ref_ptr<StateNode> &node) {
	belowHorizonNode_ = node;
	addChild(node);
}

void HorizonDividerNode::setAboveHorizonNode(const ref_ptr<StateNode> &node) {
	aboveHorizonNode_ = node;
	addChild(node);
}

void HorizonDividerNode::traverse(RenderState *rs) {
	state_->enable(rs);
	if (isBelowHorizon()) {
		belowHorizonNode_->traverse(rs);
	} else {
		aboveHorizonNode_->traverse(rs);
	}
	state_->disable(rs);
}

ref_ptr<HorizonDividerNode> HorizonDividerNode::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	auto userCamera = scene->getResource<Camera>(input.getValue("camera"));
	if (userCamera.get() == nullptr) {
		REGEN_WARN("Unable to find Camera for '" << input.getDescription() << "'.");
		return {};
	}

	auto belowHorizonCfg = input.getFirstChild("below");
	auto aboveHorizonCfg = input.getFirstChild("above");
	if (!belowHorizonCfg || !aboveHorizonCfg) {
		REGEN_WARN("Both 'below' and 'above' nodes must be specified for '"
			<< input.getDescription() << "'.");
		return {};
	}

	auto node = ref_ptr<HorizonDividerNode>::alloc(userCamera);
	ctx.parent()->addChild(node);

	if (input.hasAttribute("level")) {
		node->setSurfaceHeight(input.getValue<float>("level", 0.0f));
	}

	// load children nodes
	auto belowNode = ref_ptr<StateNode>::alloc();
	belowNode->set_name(belowHorizonCfg->getName());
	node->setBelowHorizonNode(belowNode);
	scene::SceneNodeProcessor::handleChildren(scene, *belowHorizonCfg.get(), belowNode);

	auto aboveNode = ref_ptr<StateNode>::alloc();
	aboveNode->set_name(aboveHorizonCfg->getName());
	node->setAboveHorizonNode(aboveNode);
	scene::SceneNodeProcessor::handleChildren(scene, *aboveHorizonCfg.get(), aboveNode);

	return node;
}
