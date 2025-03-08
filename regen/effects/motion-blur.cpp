
#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>
#include <regen/states/depth-state.h>

#include "motion-blur.h"

using namespace regen;

MotionBlur::MotionBlur(const ref_ptr<Camera> &camera)
		: FullscreenPass("regen.filter.motion-blur"),
		  camera_(camera) {
	ref_ptr<DepthState> depthState = ref_ptr<DepthState>::alloc();
	depthState->set_useDepthTest(GL_FALSE);
	depthState->set_useDepthWrite(GL_FALSE);
	joinStates(depthState);

	lastViewProjectionMat_ = ref_ptr<ShaderInputMat4>::alloc("lastViewProjectionMatrix");
	lastViewProjectionMat_->setUniformData(Mat4f::identity());
	joinShaderInput(lastViewProjectionMat_);
}

void MotionBlur::enable(RenderState *rs) {
	State::enable(rs);
}

void MotionBlur::disable(RenderState *rs) {
	State::disable(rs);

	// remember last view projection
	auto m = camera_->viewProjection()->getVertex(0);
	lastViewProjectionMat_->setVertex(0, m.r);
}

ref_ptr<MotionBlur> MotionBlur::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	// get the camera from input
	auto camera = ctx.scene()->getResource<Camera>(input.getValue("camera"));
	if (camera.get() == nullptr) {
		REGEN_WARN("No Camera found for " << input.getDescription() << ".");
		return {};
	}

	auto fs = ref_ptr<MotionBlur>::alloc(camera);

	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(ctx.parent().get());
	shaderConfigurer.addState(fs.get());
	fs->createShader(shaderConfigurer.cfg());
	return fs;
}
