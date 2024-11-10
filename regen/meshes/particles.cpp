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
#include <random>
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

	updateState_ = ref_ptr<ShaderState>::alloc();
}

void Particles::begin() {
	begin(ShaderInputContainer::INTERLEAVED);
}

void Particles::begin(ShaderInputContainer::DataLayout layout) {
	HasInput::begin(layout);

	GLuint numParticles = inputContainer()->numVertices();

	// Initialize the random number generator and distribution
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, std::numeric_limits<int>::max());

	// get a random seed for each particle
	ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::alloc("randomSeed");
	randomSeed_->setVertexData(numParticles, nullptr);
	for (GLuint i = 0u; i < numParticles; ++i) {
		randomSeed_->setVertex(i, dis(gen));
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

	for (auto & particleInput : particleInputs) {
		const ref_ptr<ShaderInput> in = particleInput.in_;
		if (!in->isVertexAttribute()) continue;
		GLint loc = updateState_->shader()->attributeLocation(particleInput.in_->name());
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
	for (const auto & input : inputs) {
		if (!input.in_->isVertexAttribute()) continue;
		shaderCfg.feedbackAttributes_.push_back(input.in_->name());
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
	for (auto & particleAttribute : particleAttributes_) {
		particleAttribute.input->enableAttribute(particleAttribute.location);
	}

	bufferRange_.buffer_ = feedbackRef_->bufferID();
	bufferRange_.offset_ = feedbackRef_->address();
	rs->feedbackBufferRange().push(0, bufferRange_);
	rs->beginTransformFeedback(GL_POINTS);

	glDrawArrays(primitive_, 0, inputContainer_->numVertices());

	rs->endTransformFeedback();
	rs->feedbackBufferRange().pop(0);

	rs->vao().pop();
	updateState_->disable(rs);
	rs->toggles().pop(RenderState::RASTARIZER_DISCARD);

	// Update particle attribute layout.
	GLuint currOffset = bufferRange_.offset_;
	for (auto & particleAttribute : particleAttributes_) {
		particleAttribute.input->set_buffer(bufferRange_.buffer_, feedbackRef_);
		particleAttribute.input->set_offset(currOffset);
		currOffset += particleAttribute.input->elementSize();
	}
	// And update the VAO so that next drawing uses last feedback result.
	std::set<Mesh *> particleMeshes;
	getMeshViews(particleMeshes);
	for (auto particleMesh : particleMeshes) { particleMesh->updateVAO(rs); }

	// Ping-Pong VBO references so that next feedback goes to other buffer.
	VBOReference buf = particleRef_;
	particleRef_ = feedbackRef_;
	feedbackRef_ = buf;

	GL_ERROR_LOG();
}
