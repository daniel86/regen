#include "bloom-pass.h"

using namespace regen;

BloomPass::BloomPass(
			const ref_ptr<Texture> &inputTexture,
			const ref_ptr<BloomTexture> &bloomTexture)
		: inputTexture_(inputTexture),
		  bloomTexture_(bloomTexture)
{
	fullscreenMesh_ = Rectangle::getUnitQuad();

	inverseInputSize_ = ref_ptr<ShaderInput2f>::alloc("inverseInputSize");
	inverseInputSize_->setUniformData(Vec2f(0.0f));
	state()->joinShaderInput(inverseInputSize_);

	inverseViewport_ = ref_ptr<ShaderInput2f>::alloc("inverseViewport");
	inverseViewport_->setUniformData(Vec2f(0.0f));
	state()->joinShaderInput(inverseViewport_);

 	// create downsample state
 	downsampleState_ = ref_ptr<State>::alloc();
 	downsampleShader_ = ref_ptr<ShaderState>::alloc();
 	downsampleState_->joinStates(downsampleShader_);

 	// create upsample state
 	upsampleState_ = ref_ptr<State>::alloc();
	upsampleShader_ = ref_ptr<ShaderState>::alloc();
 	// Enable additive blending
 	auto upsampleBlending = ref_ptr<BlendState>::alloc(GL_ONE, GL_ONE);
 	upsampleBlending->setBlendEquation(GL_FUNC_ADD);
 	upsampleState_->joinStates(upsampleBlending);
 	upsampleState_->joinStates(upsampleShader_);

	fbo_ = ref_ptr<FBO>::alloc(bloomTexture->width(),bloomTexture->height());
	RenderState::get()->drawFrameBuffer().push(fbo_->id());
	glFramebufferTexture(GL_DRAW_FRAMEBUFFER,
						 GL_COLOR_ATTACHMENT0,
						 bloomTexture->id(), 0);
	RenderState::get()->drawFrameBuffer().pop();
}

void BloomPass::createShader(const StateConfig &cfg) {
	auto *rs = RenderState::get();
	upsampleShader_->createShader(cfg, "regen.filter.bloom.upsample");
	fullscreenMesh_->updateVAO(rs, cfg, upsampleShader_->shader());
 	inverseViewportLocUS_ = upsampleShader_->shader()->uniformLocation("inverseViewport");

	downsampleShader_->createShader(cfg, "regen.filter.bloom.downsample");
	fullscreenMesh_->updateVAO(rs, cfg, downsampleShader_->shader());
 	inverseViewportLocDS_ = downsampleShader_->shader()->uniformLocation("inverseViewport");
 	inverseInputSizeLocDS_ = downsampleShader_->shader()->uniformLocation("inverseInputSize");
}

void BloomPass::downsample(RenderState *rs) {
	// Downsampling of the scene color buffer into a mip chain.
	auto nextInputTexture = inputTexture_.get();

	downsampleState_->enable(rs);
	glUniform2f(inverseInputSizeLocDS_,
			static_cast<GLfloat>(1.0 / nextInputTexture->width()),
			static_cast<GLfloat>(1.0 / nextInputTexture->height()));

    for (auto &mip : bloomTexture_->mips()) {
		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,
							 GL_COLOR_ATTACHMENT0,
							 mip.texture->id(), 0);
        glUniform2f(inverseViewportLocDS_, mip.sizeInverse.x, mip.sizeInverse.y);

        // render a quad that fills the selected mip level
        nextInputTexture->begin(rs, 0);
        rs->viewport().push(mip.glViewport);
        fullscreenMesh_->enable(rs);
        fullscreenMesh_->disable(rs);
        rs->viewport().pop();
        nextInputTexture->end(rs, 0);

        nextInputTexture = mip.texture;
		glUniform2f(inverseInputSizeLocDS_, mip.sizeInverse.x, mip.sizeInverse.y);
    }
	downsampleState_->disable(rs);
}

void BloomPass::upsample(RenderState *rs) {
	auto &mips = bloomTexture_->mips();
	upsampleState_->enable(rs);
    for (auto i = mips.size() - 1; i > 0; i--) {
    	// upsample mip level i into mip level i-1
        auto& mip = mips[i];
        auto& nextMip = mips[i-1];

		glFramebufferTexture(GL_DRAW_FRAMEBUFFER,
							 GL_COLOR_ATTACHMENT0,
							 nextMip.texture->id(), 0);
		glUniform2f(inverseViewportLocUS_,
				nextMip.sizeInverse.x, nextMip.sizeInverse.y);

        // set next mip texture as render target, and render a quad that fills it
        rs->viewport().push(nextMip.glViewport);
        mip.texture->begin(rs, 0);
        fullscreenMesh_->enable(rs);
        fullscreenMesh_->disable(rs);
        mip.texture->end(rs, 0);
        rs->viewport().pop();
    }
	upsampleState_->disable(rs);
}

void BloomPass::traverse(RenderState *rs) {
	state()->enable(rs);
	rs->drawFrameBuffer().push(fbo_->id());
	downsample(rs);
	upsample(rs);
	rs->drawFrameBuffer().pop();
	state()->disable(rs);
	GL_ERROR_LOG();
}
