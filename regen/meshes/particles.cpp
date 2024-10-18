/*
 * particles.cpp
 *
 *  Created on: 03.11.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/blend-state.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>
#include <regen/gl-types/gl-enum.h>
#include "particles.h"

using namespace regen;

///////////

Particles::Particles(GLuint numParticles, const std::string &updateShaderKey)
		: Mesh(GL_POINTS, VBO::USAGE_STREAM),
		  Animation(GL_TRUE, GL_FALSE),
		  updateShaderKey_(updateShaderKey) {
	feedbackBuffer_ = ref_ptr<VBO>::alloc(VBO::USAGE_FEEDBACK);
	inputContainer_->set_numVertices(numParticles);

	deltaT_ = ref_ptr<ShaderInput1f>::alloc("deltaT");
	deltaT_->setUniformData(0.0);
	setInput(deltaT_);

	gravity_ = ref_ptr<ShaderInput3f>::alloc("gravity");
	gravity_->setUniformData(Vec3f(0.0, -9.81, 0.0));
	setInput(gravity_);

	dampingFactor_ = ref_ptr<ShaderInput1f>::alloc("dampingFactor");
	dampingFactor_->setUniformData(2.5);
	setInput(dampingFactor_);

	noiseFactor_ = ref_ptr<ShaderInput1f>::alloc("noiseFactor");
	noiseFactor_->setUniformData(0.5);
	setInput(noiseFactor_);

	updateState_ = ref_ptr<ShaderState>::alloc();
}

void Particles::begin() {
	begin(ShaderInputContainer::INTERLEAVED);
}

void Particles::begin(ShaderInputContainer::DataLayout layout) {
	HasInput::begin(layout);

	GLuint numParticles = inputContainer()->numVertices();

	srand(time(0));
	// get a random seed for each particle
	ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::alloc("randomSeed");
	randomSeed_->setVertexData(numParticles, nullptr);
	for (GLuint i = 0u; i < numParticles; ++i) {
		randomSeed_->setVertex(i, rand());
	}
	setInput(randomSeed_);

	// initially set lifetime to zero so that particles
	// get emitted in the first step
	ref_ptr<ShaderInput1f> lifetimeInput_ = ref_ptr<ShaderInput1f>::alloc("lifetime");
	lifetimeInput_->setVertexData(numParticles, nullptr);
	for (GLuint i = 0u; i < numParticles; ++i) {
		lifetimeInput_->setVertex(i, -1.0);
	}
	setInput(lifetimeInput_);
}

VBOReference Particles::end() {
	ShaderInputList particleInputs = inputContainer()->uploadInputs();

	particleRef_ = HasInput::end();
	feedbackRef_ = feedbackBuffer_->alloc(particleRef_->allocatedSize());
	bufferRange_.size_ = particleRef_->allocatedSize();

	// Create shader defines.
	GLuint counter = 0;
	for (auto it = particleInputs.begin(); it != particleInputs.end(); ++it) {
		if (!it->in_->isVertexAttribute()) continue;
		shaderDefine(
				REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_TYPE"),
				glenum::glslDataType(it->in_->dataType(), it->in_->valsPerElement()));
		shaderDefine(
				REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_NAME"),
				it->in_->name());
		counter += 1;
		REGEN_DEBUG("Particle attribute '" << it->in_->name() << "' added.");
	}
	shaderDefine("NUM_PARTICLE_ATTRIBUTES", REGEN_STRING(counter));
	createUpdateShader(particleInputs);

	for (auto it = particleInputs.begin(); it != particleInputs.end(); ++it) {
		const ref_ptr<ShaderInput> in = it->in_;
		if (!in->isVertexAttribute()) continue;
		GLint loc = updateState_->shader()->attributeLocation(it->in_->name());
		if (loc == -1) continue;
		particleAttributes_.emplace_back(in, loc);
	}

	return particleRef_;
}

void Particles::createUpdateShader(const ShaderInputList &inputs) {
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addState(this);

	StateConfig &shaderCfg = shaderConfigurer.cfg();
	shaderCfg.feedbackAttributes_.clear();
	for (auto it = inputs.begin(); it != inputs.end(); ++it) {
		if (!it->in_->isVertexAttribute()) continue;
		shaderCfg.feedbackAttributes_.push_back(it->in_->name());
	}
	shaderCfg.feedbackMode_ = GL_INTERLEAVED_ATTRIBS;
	shaderCfg.feedbackStage_ = GL_VERTEX_SHADER;
	updateState_->createShader(shaderCfg, updateShaderKey_);
	shaderCfg.feedbackAttributes_.clear();
}

void Particles::glAnimate(RenderState *rs, GLdouble dt) {
	deltaT_->setVertex(0, dt);

	rs->toggles().push(RenderState::RASTARIZER_DISCARD, GL_TRUE);
	updateState_->enable(rs);

	ref_ptr<VAO> particleVAO = ref_ptr<VAO>::alloc();
	rs->vao().push(particleVAO->id());
	glBindBuffer(GL_ARRAY_BUFFER, particleRef_->bufferID());
	for (auto it = particleAttributes_.begin(); it != particleAttributes_.end(); ++it) {
		it->input->enableAttribute(it->location);
	}

	bufferRange_.buffer_ = feedbackRef_->bufferID();
	bufferRange_.offset_ = feedbackRef_->address();
	rs->feedbackBufferRange().push(0, bufferRange_);
	rs->beginTransformFeedback(feedbackPrimitive_);

	glDrawArrays(primitive_, 0, inputContainer_->numVertices());

	rs->endTransformFeedback();
	rs->feedbackBufferRange().pop(0);

	rs->vao().pop();
	updateState_->disable(rs);
	rs->toggles().pop(RenderState::RASTARIZER_DISCARD);

	// Update particle attribute layout.
	GLuint currOffset = bufferRange_.offset_;
	for (auto it = particleAttributes_.begin(); it != particleAttributes_.end(); ++it) {
		it->input->set_buffer(bufferRange_.buffer_, feedbackRef_);
		it->input->set_offset(currOffset);
		currOffset += it->input->elementSize();
	}
	// And update the VAO so that next drawing uses last feedback result.
	std::set<Mesh *> particleMeshes;
	getMeshViews(particleMeshes);
	for (auto it = particleMeshes.begin(); it != particleMeshes.end(); ++it) { (*it)->updateVAO(rs); }

	// Ping-Pong VBO references so that next feedback goes to other buffer.
	VBOReference buf = particleRef_;
	particleRef_ = feedbackRef_;
	feedbackRef_ = buf;

	GL_ERROR_LOG();
}

const ref_ptr<ShaderInput3f> &Particles::gravity() const { return gravity_; }

const ref_ptr<ShaderInput1f> &Particles::dampingFactor() const { return dampingFactor_; }

const ref_ptr<ShaderInput1f> &Particles::noiseFactor() const { return noiseFactor_; }
