#include "bloom-pass.h"

using namespace regen;

BloomPass::BloomPass(
		const ref_ptr<Texture> &inputTexture,
		const ref_ptr<BloomTexture> &bloomTexture)
		: inputTexture_(inputTexture),
		  bloomTexture_(bloomTexture) {
	fullscreenMesh_d_ = Rectangle::getUnitQuad();
	fullscreenMesh_u_ = Rectangle::getUnitQuad();

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

	fbo_ = ref_ptr<FBO>::alloc(bloomTexture->width(), bloomTexture->height());
	glNamedFramebufferTexture(fbo_->id(),
						 GL_COLOR_ATTACHMENT0,
						 bloomTexture->id(), 0);
}

void BloomPass::createShader(const StateConfig &cfg) {
	upsampleShader_->createShader(cfg, "regen.filter.bloom.upsample");
	fullscreenMesh_u_->updateVAO(cfg, upsampleShader_->shader());
	inverseViewportLocUS_ = upsampleShader_->shader()->uniformLocation("inverseViewport");

	downsampleShader_->createShader(cfg, "regen.filter.bloom.downsample");
	fullscreenMesh_d_->updateVAO(cfg, downsampleShader_->shader());
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

	for (auto &mip: bloomTexture_->mips()) {
		glNamedFramebufferTexture(fbo_->id(),
							 GL_COLOR_ATTACHMENT0,
							 mip.texture->id(), 0);
		glUniform2f(inverseViewportLocDS_, mip.sizeInverse.x, mip.sizeInverse.y);

		// render a quad that fills the selected mip level
		nextInputTexture->bind();
		rs->viewport().push(mip.glViewport);
		fullscreenMesh_d_->enable(rs);
		fullscreenMesh_d_->disable(rs);
		rs->viewport().pop();

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
		auto &mip = mips[i];
		auto &nextMip = mips[i - 1];

		glNamedFramebufferTexture(fbo_->id(),
							 GL_COLOR_ATTACHMENT0,
							 nextMip.texture->id(), 0);
		glUniform2f(inverseViewportLocUS_,
					nextMip.sizeInverse.x, nextMip.sizeInverse.y);

		// set next mip texture as render target, and render a quad that fills it
		rs->viewport().push(nextMip.glViewport);
		mip.texture->bind();
		fullscreenMesh_u_->enable(rs);
		fullscreenMesh_u_->disable(rs);
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

ref_ptr<BloomPass> BloomPass::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	auto scene = ctx.scene();

	if (!input.hasAttribute("texture") && !input.hasAttribute("fbo")) {
		REGEN_WARN("Ignoring " << input.getDescription() << " without input texture.");
		return {};
	}
	if (!input.hasAttribute("bloom-texture")) {
		REGEN_WARN("Ignoring " << input.getDescription() << " without bloom texture.");
		return {};
	}
	// get the bloom texture
	auto bloomTexture1 = scene->getResource<Texture>(input.getValue("bloom-texture"));
	if (bloomTexture1.get() == nullptr) {
		REGEN_WARN("No bloom-texture found for " << input.getDescription() << ".");
		return {};
	}
	auto bloomTexture = ref_ptr<BloomTexture>::dynamicCast(bloomTexture1);
	if (bloomTexture.get() == nullptr) {
		REGEN_WARN("Bloom texture has wrong type for " << input.getDescription() << ".");
		return {};
	}
	// get the input texture
	ref_ptr<Texture> inputTexture;
	if (input.hasAttribute("texture")) {
		inputTexture = scene->getResource<Texture>(input.getValue("texture"));
		if (inputTexture.get() == nullptr) {
			REGEN_WARN("Unable to find Texture for " << input.getDescription() << ".");
			return {};
		}
	} else {
		ref_ptr<FBO> fbo = scene->getResource<FBO>(input.getValue("fbo"));
		if (fbo.get() == nullptr) {
			REGEN_WARN("Unable to find FBO for " << input.getDescription() << ".");
			return {};
		}
		auto attachment = input.getValue<GLuint>("attachment", 0);
		std::vector<ref_ptr<Texture> > textures = fbo->colorTextures();
		if (attachment >= textures.size()) {
			REGEN_WARN("FBO " << input.getValue("fbo") <<
							  " has less then " << (attachment + 1) << " attachments.");
			return {};
		}
		inputTexture = textures[attachment];
	}

	auto bloomPass = ref_ptr<BloomPass>::alloc(inputTexture, bloomTexture);
	ctx.parent()->addChild(bloomPass);
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addNode(bloomPass.get());
	bloomPass->createShader(shaderConfigurer.cfg());

	return bloomPass;
}
