/*
 * filter.cpp
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>
#include <regen/utility/string-util.h>

#include "filter.h"

using namespace regen;

Filter::Filter(const std::string &shaderKey, GLfloat scaleFactor)
		: FullscreenPass(shaderKey), scaleFactor_(scaleFactor) {
	format_ = GL_NONE;
	internalFormat_ = GL_NONE;
	pixelType_ = GL_NONE;
	bindInput_ = GL_TRUE;

	inputState_ = ref_ptr<TextureState>::alloc();
	inputState_->set_name("inputTexture");
	joinStatesFront(inputState_);
}

void Filter::set_bindInput(GLboolean v) {
	if (v == bindInput_) { return; }
	bindInput_ = v;

	if (bindInput_) {
		joinStatesFront(inputState_);
	} else {
		disjoinStates(inputState_);
	}
}

void Filter::set_input(const ref_ptr<Texture> &input) {
	input_ = input;
	inputState_->set_texture(input_);
	inputState_->set_name("inputTexture");
}

ref_ptr<Texture> Filter::createTexture() {
	GLenum format =
			(format_ == GL_NONE ? input_->format() : format_);
	GLenum internalFormat =
			(internalFormat_ == GL_NONE ? input_->internalFormat() : internalFormat_);
	GLenum pixelType =
			(pixelType_ == GL_NONE ? input_->pixelType() : pixelType_);
	return out_->fbo_->addTexture(1, input_->targetType(), format, internalFormat, pixelType);
}

void Filter::setInput(const ref_ptr<Texture> &input) {
	set_input(input);

	//GLuint inputSize = min(input_->width(), input_->height());
	//GLuint bufferSize  = scaleFactor_*inputSize;
	GLuint bufferW = scaleFactor_ * input_->width();
	GLuint bufferH = scaleFactor_ * input_->height();

	GLuint inputDepth = 1;
	if (dynamic_cast<Texture3D *>(input_.get())) {
		inputDepth = (dynamic_cast<Texture3D *>(input_.get()))->depth();
	}

	// create the render target. As this is the first filter with this target
	// the attachment point is GL_COLOR_ATTACHMENT0 and only a single texture
	// is added to the fbo.
	out_ = ref_ptr<Output>::alloc();
	out_->fbo_ = ref_ptr<FBO>::alloc(bufferW, bufferH, inputDepth);
	out_->tex0_ = createTexture();

	// call drawBuffer( GL_COLOR_ATTACHMENT0 )
	outputAttachment_ = GL_COLOR_ATTACHMENT0;
	if (drawBufferState_.get()) {
		disjoinStates(drawBufferState_);
	}
	drawBufferState_ = ref_ptr<DrawBufferState>::alloc(out_->fbo_);
	drawBufferState_->colorBuffers.buffers_.push_back(outputAttachment_);
	joinStatesFront(drawBufferState_);
}

void Filter::setInput(
		const ref_ptr<Output> &lastOutput, GLenum lastAttachment) {
	// take last output as input
	if (lastAttachment == GL_COLOR_ATTACHMENT0) {
		input_ = lastOutput->tex0_;
	} else {
		input_ = lastOutput->tex1_;
	}
	set_input(input_);

	out_ = lastOutput;
	// make sure two textures created for ping pong rendering
	if (!out_->tex1_.get()) {
		out_->tex1_ = createTexture();
	}

	// call drawBuffer( outputAttachment_ )
	outputAttachment_ = GL_COLOR_ATTACHMENT0 + (GL_COLOR_ATTACHMENT1 - lastAttachment);
	if (drawBufferState_.get()) {
		disjoinStates(drawBufferState_);
	}
	drawBufferState_ = ref_ptr<DrawBufferState>::alloc(out_->fbo_);
	drawBufferState_->colorBuffers.buffers_.push_back(outputAttachment_);
	joinStatesFront(drawBufferState_);
}

/////////////////
/////////////////


FilterSequence::FilterSequence(const ref_ptr<Texture> &input, GLboolean bindInput)
		: State(),
		  input_(input),
		  clearFirstFilter_(GL_FALSE),
		  clearColor_(Vec4f(0.0f)),
		  lastWidth_(0u),
		  lastHeight_(0u),
		  bindInput_(bindInput) {
	format_ = GL_NONE;
	internalFormat_ = GL_NONE;
	pixelType_ = GL_NONE;

	viewport_ = ref_ptr<ShaderInput2f>::alloc("viewport");
	viewport_->setUniformData(Vec2f(
			(GLfloat) input->width(), (GLfloat) input->height()));
	joinShaderInput(viewport_);

	inverseViewport_ = ref_ptr<ShaderInput2f>::alloc("inverseViewport");
	inverseViewport_->setUniformData(Vec2f(
			1.0f / (GLfloat) input->width(), 1.0f / (GLfloat) input->height()));
	joinShaderInput(inverseViewport_);

	ref_ptr<ShaderInput2f> inverseViewport;

	// use layered geometry shader for 3d textures
	if (dynamic_cast<TextureCube *>(input_.get())) {
		shaderDefine("RENDER_LAYER", "6");
		shaderDefine("RENDER_TARGET", "CUBE");
	} else if (dynamic_cast<Texture3D *>(input_.get())) {
		auto *tex3D = (Texture3D *) input_.get();
		shaderDefine("RENDER_LAYER", REGEN_STRING(tex3D->depth()));
		shaderDefine("RENDER_TARGET", "2D_ARRAY");
	} else {
		shaderDefine("RENDER_LAYER", "1");
		shaderDefine("RENDER_TARGET", "2D");
	}
}

void FilterSequence::setClearColor(const Vec4f &clearColor) {
	clearFirstFilter_ = GL_TRUE;
	clearColor_ = clearColor;
}

const ref_ptr<Texture> &FilterSequence::output() const {
	if (filterSequence_.empty()) {
		// no filter added yet, return identity
		return input_;
	}
	ref_ptr<Filter> lastFilter = *filterSequence_.rbegin();
	GLenum last = lastFilter->outputAttachment();
	if (last == GL_COLOR_ATTACHMENT0) {
		return lastFilter->output()->tex0_;
	} else {
		return lastFilter->output()->tex1_;
	}
}

void FilterSequence::addFilter(const ref_ptr<Filter> &f) {
	if (filterSequence_.empty()) {
		f->set_bindInput(bindInput_);
		f->set_format(format_);
		f->set_internalFormat(internalFormat_);
		f->set_pixelType(pixelType_);
		// no filter was added before, create a new framebuffer
		f->setInput(input_);
	} else {
		ref_ptr<Filter> lastFilter = *filterSequence_.rbegin();
		// another filter was added before
		if (math::isApprox(f->scaleFactor(), 1.0)) {
			// filter does not apply scaling. We gonna ping pong render
			// to the framebuffer provided by previous filter.
			f->setInput(lastFilter->output(), lastFilter->outputAttachment());
		} else {
			// create a new framebuffer for this filter
			GLenum last = lastFilter->outputAttachment();
			if (last == GL_COLOR_ATTACHMENT0) {
				f->setInput(lastFilter->output()->tex0_);
			} else {
				f->setInput(lastFilter->output()->tex1_);
			}
		}
	}

	filterSequence_.push_back(f);
}

void FilterSequence::createShader(StateConfig &cfg) {
	for (auto & it : filterSequence_) {
		auto *f = (Filter *) it.get();
		StateConfigurer _cfg(cfg);
		_cfg.addState(f);
		f->createShader(_cfg.cfg());
	}
}

void FilterSequence::resize() {
	//GLuint size = min(input_->width(), input_->height());
	GLuint width = input_->width();
	GLuint height = input_->height();
	if (width == lastWidth_ && height == lastHeight_) return;

	FBO *last = nullptr;
	for (auto & it : filterSequence_) {
		auto *f = (Filter *) it.get();
		FBO *fbo = f->output()->fbo_.get();

		if (last != fbo) {
			width *= f->scaleFactor();
			height *= f->scaleFactor();
			fbo->resize(width, height, fbo->depth());
		}

		last = fbo;
	}

	lastWidth_ = width;
	lastHeight_ = height;
}

void FilterSequence::enable(RenderState *rs) {
	State::enable(rs);

	if (filterSequence_.empty()) { return; }
	// auto-resize if input texture size changed
	resize();

	if (clearFirstFilter_) {
		Filter *firstFilter = (Filter *) (*filterSequence_.begin()).get();
		rs->drawFrameBuffer().push(firstFilter->output()->fbo_->id());
		rs->clearColor().push(clearColor_);
		glClear(GL_COLOR_BUFFER_BIT);
		rs->clearColor().pop();
		rs->drawFrameBuffer().pop();
	}
	for (auto & it : filterSequence_) {
		auto *f = (Filter *) it.get();

		FBO *fbo = f->output()->fbo_.get();
		rs->drawFrameBuffer().push(fbo->id());
		rs->viewport().push(fbo->glViewport());
		viewport_->setVertex(0, fbo->viewport()->getVertex(0));
		inverseViewport_->setVertex(0, fbo->inverseViewport()->getVertex(0));

		f->enable(rs);
		f->disable(rs);

		rs->viewport().pop();
		rs->drawFrameBuffer().pop();
	}
}