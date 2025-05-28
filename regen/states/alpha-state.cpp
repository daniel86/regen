#include "alpha-state.h"

using namespace regen;

AlphaState::AlphaState() : ServerSideState() {
}

void AlphaState::setDiscardThreshold(float discardThreshold) {
	if (!discardThreshold_.get()) {
		discardThreshold_ = createUniform<ShaderInput1f>("alphaDiscardThreshold", 0.25f);
		joinShaderInput(discardThreshold_);
		if (forceEarlyDepthTest_) {
			shaderDefine("FS_EARLY_FRAGMENT_TEST", "TRUE");
		}
	}
	discardThreshold_->setVertex(0, discardThreshold);
}

void AlphaState::setClipThreshold(float edge) {
	if (!clipThreshold_.get()) {
		clipThreshold_ = createUniform<ShaderInput1f>("alphaClipThreshold", 0.5f);
		joinShaderInput(clipThreshold_);
	}
	clipThreshold_->setVertex(0, edge);
}

void AlphaState::setClipThreshold(float edgeMin, float edgeMax) {
	if (!clipMin_.get()) {
		clipMin_ = createUniform<ShaderInput1f>("alphaClipMin", 0.0f);
		joinShaderInput(clipMin_);
	}
	if (!clipMax_.get()) {
		clipMax_ = createUniform<ShaderInput1f>("alphaClipMax", 1.0f);
		joinShaderInput(clipMax_);
	}
	clipMin_->setVertex(0, edgeMin);
	clipMax_->setVertex(0, edgeMax);
}

void AlphaState::setForceEarlyDepthTest(bool forceEarlyDepthTest) {
	forceEarlyDepthTest_ = forceEarlyDepthTest;
	if (forceEarlyDepthTest_) {
		shaderDefine("FS_EARLY_FRAGMENT_TEST", "TRUE");
	} else {
		shaderDefine("FS_EARLY_FRAGMENT_TEST", "FALSE");
	}
}

ref_ptr<AlphaState> AlphaState::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	ref_ptr<AlphaState> alpha = ref_ptr<AlphaState>::alloc();
	if (input.hasAttribute("discard-threshold")) {
		alpha->setDiscardThreshold(input.getValue<float>("discard-threshold", 0.25f));
	}
	if (input.hasAttribute("clip-threshold")) {
		alpha->setClipThreshold(input.getValue<float>("clip-threshold", 0.5f));
	}
	if (input.hasAttribute("clip-min") || input.hasAttribute("clip-max")) {
		alpha->setClipThreshold(input.getValue<float>("clip-min", 0.0f),
				input.getValue<float>("clip-max", 1.0f));
	}
	if (input.hasAttribute("force-early-depth-test")) {
		alpha->setForceEarlyDepthTest(input.getValue<bool>("force-early-depth-test", false));
	}
	return alpha;
}
