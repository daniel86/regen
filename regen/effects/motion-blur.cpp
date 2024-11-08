
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
    auto m = camera_->viewProjection();
    lastViewProjectionMat_->setUniformData(m->getVertex(0));
}
