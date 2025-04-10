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
		: Mesh(GL_POINTS, USAGE_STREAM),
		  Animation(true, false),
		  updateShaderKey_(updateShaderKey),
		  maxEmits_(100u) {
	setAnimationName("particles");
	feedbackBuffer_ = ref_ptr<VBO>::alloc(TRANSFORM_FEEDBACK_BUFFER, USAGE_STREAM);
	inputContainer_->set_numVertices(numParticles);
	updateState_ = ref_ptr<ShaderState>::alloc();
	numParticles_ = numParticles;
	// create an atomic counter for computing the bounding box
	//boundingBoxCounter_ = ref_ptr<BoundingBoxCounter>::alloc();
}

void Particles::begin() {
	begin(InputContainer::INTERLEAVED);
}

void Particles::begin(InputContainer::DataLayout layout) {
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

ref_ptr<BufferReference> Particles::end() {
	ShaderInputList particleInputs = inputContainer()->uploadInputs();

	particleRef_ = HasInput::end();
	feedbackRef_ = feedbackBuffer_->allocBytes(particleRef_->allocatedSize());
	if (feedbackRef_.get() == nullptr) {
		REGEN_WARN("Unable to allocate VBO for particles. Particles will not work.");
		return particleRef_;
	}
	bufferRange_.size_ = particleRef_->allocatedSize();

	// Create shader defines.
	GLuint counter = 0;
	for (auto it = particleInputs.begin(); it != particleInputs.end(); ++it) {
		if (!it->in_->isVertexAttribute()) continue;
		shaderDefine(
				REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_TYPE"),
				glenum::glslDataType(it->in_->baseType(), it->in_->valsPerElement()));
		shaderDefine(
				REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_NAME"),
				it->in_->name());
		if (advanceModes_.find(it->in_->name()) != advanceModes_.end()) {
			configureAdvancing(it->in_, counter, advanceModes_[it->in_->name()]);
		}
		counter += 1;
		REGEN_DEBUG("Particle attribute '" << it->in_->name() << "' added.");
	}
	shaderDefine("NUM_PARTICLE_ATTRIBUTES", REGEN_STRING(counter));
	createUpdateShader(particleInputs);

	for (auto &particleInput: particleInputs) {
		const ref_ptr<ShaderInput> in = particleInput.in_;
		if (!in->isVertexAttribute()) continue;
		GLint loc = updateState_->shader()->attributeLocation(particleInput.in_->name());
		if (loc == -1) continue;
		particleAttributes_.emplace_back(in, loc);
	}
	// start with zero emitted particles
	//inputContainer_->set_numVertices(0);

	return particleRef_;
}

void Particles::setAdvanceFunction(const std::string &attributeName, const std::string &shaderFunction) {
	advanceFunctions_[attributeName] = shaderFunction;
	advanceModes_[attributeName] = ADVANCE_MODE_CUSTOM;
}

void Particles::setAdvanceRamp(const std::string &attributeName, const ref_ptr<Texture> &texture, RampMode mode) {
	ramps_[attributeName] = {texture, mode};
}

void Particles::configureAdvancing(
		const ref_ptr<ShaderInput> &in,
		GLuint counter,
		AdvanceMode mode) {
	std::string advanceImportKey;
	std::string advanceFunction;
	GLfloat advanceFactor = 1.0f;
	switch (mode) {
		case ADVANCE_MODE_CUSTOM: {
			auto needle = advanceFunctions_.find(in->name());
			if (needle == advanceFunctions_.end()) {
				REGEN_WARN("No custom advance function found for '" << in->name() << "'.");
				return;
			}
			advanceImportKey = needle->second;
			// get string after the last dot, convention is that this is the function name
			advanceFunction = advanceImportKey.substr(advanceImportKey.find_last_of('.') + 1);
			// get the factor for this attribute
			auto factorNeedle = advanceFactors_.find(in->name());
			if (factorNeedle != advanceFactors_.end()) {
				advanceFactor = factorNeedle->second;
			}
			break;
		}
		case ADVANCE_MODE_SRC:
			advanceImportKey = "regen.states.blending.src";
			advanceFunction = "blend_src";
			break;
		case ADVANCE_MODE_MULTIPLY:
			advanceImportKey = "regen.states.blending.multiply";
			advanceFunction = "blend_multiply";
			break;
		case ADVANCE_MODE_ADD:
			advanceImportKey = "regen.states.blending.add";
			advanceFunction = "blend_add";
			break;
		case ADVANCE_MODE_SMOOTH_ADD:
			advanceImportKey = "regen.states.blending.smooth_add";
			advanceFunction = "blend_smooth_add";
			break;
		case ADVANCE_MODE_SUBTRACT:
			advanceImportKey = "regen.states.blending.sub";
			advanceFunction = "blend_sub";
			break;
		case ADVANCE_MODE_REVERSE_SUBTRACT:
			advanceImportKey = "regen.states.blending.reverse_sub";
			advanceFunction = "blend_reverse_sub";
			break;
		case ADVANCE_MODE_DIFFERENCE:
			advanceImportKey = "regen.states.blending.difference";
			advanceFunction = "blend_difference";
			break;
		case ADVANCE_MODE_LIGHTEN:
			advanceImportKey = "regen.states.blending.lighten";
			advanceFunction = "blend_lighten";
			break;
		case ADVANCE_MODE_DARKEN:
			advanceImportKey = "regen.states.blending.darken";
			advanceFunction = "blend_darken";
			break;
		case ADVANCE_MODE_MIX:
			advanceImportKey = "regen.states.blending.mix";
			advanceFunction = "blend_mix";
			break;
	}
	// create a new shader input for the advance function
	shaderDefine(
			REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_ADVANCE_FUNCTION"),
			advanceFunction);
	shaderDefine(
			REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_ADVANCE_KEY"),
			advanceImportKey);
	shaderDefine(
			REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_ADVANCE_FACTOR"),
			REGEN_STRING(advanceFactor));

	// configure the ramp texture
	auto rampNeedle = ramps_.find(in->name());
	if (rampNeedle != ramps_.end()) {
		auto &rampTexture = rampNeedle->second.texture;
		auto &rampMode = rampNeedle->second.mode;
		std::string rampImportKey, rampFunction;
		switch (rampMode) {
			case RAMP_MODE_CUSTOM: {
				auto needle = rampFunctions_.find(in->name());
				if (needle == rampFunctions_.end()) {
					REGEN_WARN("No custom ramp function found for '" << in->name() << "'.");
					return;
				}
				rampImportKey = needle->second;
				rampFunction = advanceImportKey.substr(rampImportKey.find_last_of('.') + 1);
				break;
			}
			case RAMP_MODE_TIME:
				rampImportKey = "regen.particles.ramp.time";
				rampFunction = "ramp_time";
				break;
			case RAMP_MODE_LIFETIME:
				rampImportKey = "regen.particles.ramp.lifetime";
				rampFunction = "ramp_lifetime";
				break;
			case RAMP_MODE_EMITTER_DISTANCE:
				rampImportKey = "regen.particles.ramp.emitter_distance";
				rampFunction = "ramp_emitter_distance";
				break;
			case RAMP_MODE_CAMERA_DISTANCE:
				rampImportKey = "regen.particles.ramp.camera_distance";
				rampFunction = "ramp_camera_distance";
				break;
			case RAMP_MODE_VELOCITY:
				rampImportKey = "regen.particles.ramp.velocity";
				rampFunction = "ramp_velocity";
				break;
		}
		shaderDefine(
				REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_RAMP_KEY"),
				rampImportKey);
		shaderDefine(
				REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_RAMP_FUNCTION"),
				rampFunction);
		// finally create a texture state for the ramp texture
		auto rampTextureState = ref_ptr<TextureState>::alloc();
		rampTextureState->set_texture(rampTexture);
		rampTextureState->set_name(REGEN_STRING(in->name() << "Ramp"));
		joinStates(rampTextureState);
		shaderDefine(
				REGEN_STRING("PARTICLE_ATTRIBUTE" << counter << "_RAMP_TEXEL_SIZE"),
				REGEN_STRING(1.0 / rampTexture->width()));
	}
}

void Particles::createUpdateShader(const ShaderInputList &inputs) {
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addState(animationState_.get());
	shaderConfigurer.addState(this);

	StateConfig &shaderCfg = shaderConfigurer.cfg();
	shaderCfg.feedbackAttributes_.clear();
	for (const auto &input: inputs) {
		if (!input.in_->isVertexAttribute()) continue;
		shaderCfg.feedbackAttributes_.push_back(input.in_->name());
	}
	shaderCfg.feedbackMode_ = GL_INTERLEAVED_ATTRIBS;
	shaderCfg.feedbackStage_ = GL_VERTEX_SHADER;
	updateState_->createShader(shaderCfg, updateShaderKey_);
	shaderCfg.feedbackAttributes_.clear();
}

void Particles::glAnimate(RenderState *rs, GLdouble dt) {
	if (!feedbackRef_.get()) {
		return;
	}

	rs->toggles().push(RenderState::RASTERIZER_DISCARD, GL_TRUE);
	updateState_->enable(rs);

	rs->vao().push(particleVAO_.id());
	glBindBuffer(GL_ARRAY_BUFFER, particleRef_->bufferID());
	for (auto &particleAttribute: particleAttributes_) {
		particleAttribute.input->enableAttribute(particleAttribute.location);
	}

	bufferRange_.buffer_ = feedbackRef_->bufferID();
	bufferRange_.offset_ = feedbackRef_->address();
	rs->feedbackBufferRange().push(0, bufferRange_);
	rs->beginTransformFeedback(GL_POINTS);
	// bind the atomic counter for computing the bounding box to fixed location 0
	//boundingBoxCounter_->bindBufferBase(0);

	/*
	// only emit a limited number of particles per frame
	if (inputContainer_->numVertices() < numParticles_) {
		GLuint nextNumParticles = inputContainer_->numVertices() + maxEmits_;
		if (nextNumParticles > numParticles_) {
			inputContainer_->set_numVertices(numParticles_);
		} else {
			inputContainer_->set_numVertices(nextNumParticles);
		}
	}
	*/
	glDrawArrays(primitive_, 0, inputContainer_->numVertices());

	rs->endTransformFeedback();
	rs->feedbackBufferRange().pop(0);

	rs->vao().pop();
	updateState_->disable(rs);
	rs->toggles().pop(RenderState::RASTERIZER_DISCARD);

	// Update particle attribute layout.
	GLuint currOffset = bufferRange_.offset_;
	for (auto &particleAttribute: particleAttributes_) {
		particleAttribute.input->set_buffer(bufferRange_.buffer_, feedbackRef_);
		particleAttribute.input->set_offset(currOffset);
		currOffset += particleAttribute.input->elementSize();
	}
	// And update the VAO so that next drawing uses last feedback result.
	std::set<Mesh *> particleMeshes;
	getMeshViews(particleMeshes);
	for (auto particleMesh: particleMeshes) { particleMesh->updateVAO(rs); }

	// Ping-Pong VBO references so that next feedback goes to other buffer.
	ref_ptr<BufferReference> buf = particleRef_;
	particleRef_ = feedbackRef_;
	feedbackRef_ = buf;

	// Read atomic counter to get the bounding box of the particles
	//auto bounds = boundingBoxCounter_->updateBounds();
	//set_bounds(bounds.min, bounds.max);

	GL_ERROR_LOG();
}

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Particles::AdvanceMode &mode) {
		switch (mode) {
			case Particles::ADVANCE_MODE_CUSTOM:
				out << "custom";
				break;
			case Particles::ADVANCE_MODE_SRC:
				out << "src";
				break;
			case Particles::ADVANCE_MODE_ADD:
				out << "add";
				break;
			case Particles::ADVANCE_MODE_MULTIPLY:
				out << "multiply";
				break;
			case Particles::ADVANCE_MODE_SMOOTH_ADD:
				out << "smooth_add";
				break;
			case Particles::ADVANCE_MODE_SUBTRACT:
				out << "sub";
				break;
			case Particles::ADVANCE_MODE_REVERSE_SUBTRACT:
				out << "reverse_sub";
				break;
			case Particles::ADVANCE_MODE_DIFFERENCE:
				out << "diff";
				break;
			case Particles::ADVANCE_MODE_LIGHTEN:
				out << "lighten";
				break;
			case Particles::ADVANCE_MODE_DARKEN:
				out << "darken";
				break;
			case Particles::ADVANCE_MODE_MIX:
				out << "mix";
				break;
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Particles::AdvanceMode &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "custom") mode = Particles::ADVANCE_MODE_CUSTOM;
		else if (val == "src") mode = Particles::ADVANCE_MODE_SRC;
		else if (val == "add") mode = Particles::ADVANCE_MODE_ADD;
		else if (val == "multiply") mode = Particles::ADVANCE_MODE_MULTIPLY;
		else if (val == "smooth_add") mode = Particles::ADVANCE_MODE_SMOOTH_ADD;
		else if (val == "sub") mode = Particles::ADVANCE_MODE_SUBTRACT;
		else if (val == "reverse_sub") mode = Particles::ADVANCE_MODE_REVERSE_SUBTRACT;
		else if (val == "diff") mode = Particles::ADVANCE_MODE_DIFFERENCE;
		else if (val == "lighten") mode = Particles::ADVANCE_MODE_LIGHTEN;
		else if (val == "darken") mode = Particles::ADVANCE_MODE_DARKEN;
		else if (val == "mix") mode = Particles::ADVANCE_MODE_MIX;
		else {
			REGEN_WARN("Unknown blend mode '" << val << "'. Falling bac to custom.");
			mode = Particles::ADVANCE_MODE_CUSTOM;
		}
		return in;
	}

	std::ostream &operator<<(std::ostream &out, const Particles::RampMode &mode) {
		switch (mode) {
			case Particles::RampMode::RAMP_MODE_CUSTOM:
				out << "custom";
				break;
			case Particles::RampMode::RAMP_MODE_TIME:
				out << "time";
				break;
			case Particles::RampMode::RAMP_MODE_LIFETIME:
				out << "lifetime";
				break;
			case Particles::RampMode::RAMP_MODE_EMITTER_DISTANCE:
				out << "emitter_distance";
				break;
			case Particles::RampMode::RAMP_MODE_CAMERA_DISTANCE:
				out << "camera_distance";
				break;
			case Particles::RampMode::RAMP_MODE_VELOCITY:
				out << "velocity";
				break;
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Particles::RampMode &mode) {
		std::string val;
		in >> val;
		boost::to_lower(val);
		if (val == "custom") mode = Particles::RampMode::RAMP_MODE_CUSTOM;
		else if (val == "time") mode = Particles::RampMode::RAMP_MODE_TIME;
		else if (val == "lifetime") mode = Particles::RampMode::RAMP_MODE_LIFETIME;
		else if (val == "emitter_distance") mode = Particles::RampMode::RAMP_MODE_EMITTER_DISTANCE;
		else if (val == "camera_distance") mode = Particles::RampMode::RAMP_MODE_CAMERA_DISTANCE;
		else if (val == "velocity") mode = Particles::RampMode::RAMP_MODE_VELOCITY;
		else {
			REGEN_WARN("Unknown blend mode '" << val << "'. Falling bac to custom.");
			mode = Particles::RAMP_MODE_CUSTOM;
		}
		return in;
	}
} // namespace
