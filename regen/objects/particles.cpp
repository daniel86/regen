#include <regen/utility/string-util.h>
#include "regen/scene/state-configurer.h"
#include <regen/gl/gl-enum.h>
#include <random>
#include "particles.h"

using namespace regen;

///////////

Particles::Particles(uint32_t numParticles, const std::string &updateShaderKey)
		: Mesh(GL_POINTS, BufferUpdateFlags::NEVER, VERTEX_LAYOUT_INTERLEAVED),
		  Animation(true, false),
		  updateShaderKey_(updateShaderKey),
		  maxEmits_(100u) {
	setAnimationName("particles");
	setClientAccessMode(BUFFER_CPU_WRITE);
	feedbackBuffer_ = ref_ptr<VBO>::alloc(
			TRANSFORM_FEEDBACK_BUFFER,
			BufferUpdateFlags::FULL_PER_FRAME,
			VERTEX_LAYOUT_INTERLEAVED);
	feedbackBuffer_->setClientAccessMode(BUFFER_GPU_ONLY);
	set_numVertices(numParticles);
	updateState_ = ref_ptr<ShaderState>::alloc();
	numParticles_ = numParticles;
}

void Particles::begin() {
	uint32_t numParticles = numVertices();

	// Initialize the random number generator and distribution
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis(0, std::numeric_limits<int>::max());

	// get a random seed for each particle
	ref_ptr<ShaderInput1ui> randomSeed_ = ref_ptr<ShaderInput1ui>::alloc("randomSeed");
	randomSeed_->setVertexData(numParticles, nullptr);
	for (uint32_t i = 0u; i < numParticles; ++i) {
		randomSeed_->setVertex(i, dis(gen));
	}
	setInput(randomSeed_);

	// initially set lifetime to zero so that particles
	// get emitted in the first step
	ref_ptr<ShaderInput1f> lifetimeInput_ = ref_ptr<ShaderInput1f>::alloc("lifetime");
	lifetimeInput_->setVertexData(numParticles, nullptr);
	for (uint32_t i = 0u; i < numParticles; ++i) {
		lifetimeInput_->setVertex(i, -1.0);
	}
	setInput(lifetimeInput_);
}

ref_ptr<BufferReference> Particles::end() {
	vboRef_[0] = Mesh::updateVertexData();
	vboRef_[1] = feedbackBuffer_->adoptBufferRange(vboRef_[0]->allocatedSize());
	if (vboRef_[1].get() == nullptr) {
		REGEN_WARN("Unable to allocate VBO for particles. Particles will not work.");
		return vboRef_[0];
	}
	bufferRange_.size_ = vboRef_[0]->allocatedSize();

	// Create shader defines.
	uint32_t counter = 0;
	for (auto it = inputs().begin(); it != inputs().end(); ++it) {
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
	createUpdateShader();

	for (auto &particleInput: inputs()) {
		const ref_ptr<ShaderInput> in = particleInput.in_;
		if (!in->isVertexAttribute()) continue;
		int loc = updateState_->shader()->attributeLocation(particleInput.in_->name());
		if (loc == -1) continue;
		particleAttributes_.emplace_back(in, loc);
	}
	// start with zero emitted particles
	//set_numVertices(0);

	// create an atomic counter for computing the bounding box
	/**
	bboxBuffer_ = ref_ptr<BBoxBuffer>::alloc(Bounds<Vec3f>());
	bboxPass_ = ref_ptr<ComputePass>::alloc("regen.compute.bbox");
	bboxPass_->computeState()->setNumWorkUnits(numParticles_, 1, 1);
	bboxPass_->computeState()->setGroupSize(256, 1, 1);
	bboxPass_->setInput(ref_ptr<SSBO>::alloc(feedbackBuffer_, vboRef_));
	bboxPass_->setInput(bboxBuffer_);
	StateConfigurer shaderConfigurer;
	shaderConfigurer.define("NUM_ELEMENTS", REGEN_STRING(numParticles_));
	shaderConfigurer.addState(bboxPass_.get());
	bboxPass_->createShader(shaderConfigurer.cfg());
	**/

	return vboRef_[0];
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
		uint32_t counter,
		AdvanceMode mode) {
	std::string advanceImportKey;
	std::string advanceFunction;
	float advanceFactor = 1.0f;
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

void Particles::createUpdateShader() {
	StateConfigurer shaderConfigurer;
	shaderConfigurer.addState(animationState_.get());
	shaderConfigurer.addState(this);

	StateConfig &shaderCfg = shaderConfigurer.cfg();
	shaderCfg.feedbackAttributes_.clear();
	for (const auto &input: inputs()) {
		if (!input.in_->isVertexAttribute()) continue;
		shaderCfg.feedbackAttributes_.push_back(input.in_->name());
	}
	shaderCfg.feedbackMode_ = GL_INTERLEAVED_ATTRIBS;
	shaderCfg.feedbackStage_ = GL_VERTEX_SHADER;
	updateState_->createShader(shaderCfg, updateShaderKey_);
	shaderCfg.feedbackAttributes_.clear();
}

void Particles::gpuUpdate(RenderState *rs, double dt) {
	const uint32_t nextIdx = (updateIdx_ == 0) ? 1 : 0;

	rs->toggles().push(RenderState::RASTERIZER_DISCARD, true);
	updateState_->enable(rs);

	rs->vao().apply(particleVAO_.id());
	rs->arrayBuffer().apply(vboRef_[updateIdx_]->bufferID());
	for (auto &particleAttribute: particleAttributes_) {
		particleAttribute.input->enableAttribute(particleAttribute.location);
	}

	bufferRange_.buffer_ = vboRef_[nextIdx]->bufferID();
	bufferRange_.offset_ = vboRef_[nextIdx]->address();
	rs->feedbackBufferRange().push(0, bufferRange_);
	rs->beginTransformFeedback(GL_POINTS);
	// bind the atomic counter for computing the bounding box to fixed location 0
	//boundingBoxCounter_->bindBufferBase(0);

	/*
	// only emit a limited number of particles per frame
	if (numVertices_ < numParticles_) {
		uint32_t nextNumParticles = numVertices_ + maxEmits_;
		if (nextNumParticles > numParticles_) {
			set_numVertices(numParticles_);
		} else {
			set_numVertices(nextNumParticles);
		}
	}
	*/
	glDrawArrays(primitive_, 0, numVertices());

	rs->endTransformFeedback();
	rs->feedbackBufferRange().pop(0);

	updateState_->disable(rs);
	rs->toggles().pop(RenderState::RASTERIZER_DISCARD);

	// Update particle attribute layout.
	uint32_t currOffset = bufferRange_.offset_;
	for (auto &particleAttribute: particleAttributes_) {
		particleAttribute.input->set_buffer(bufferRange_.buffer_, vboRef_[nextIdx]);
		particleAttribute.input->set_offset(currOffset);
		currOffset += particleAttribute.input->elementSize();
	}
	// And update the VAO so that next drawing uses last feedback result.
	updateVAO(bufferRange_.buffer_);
	for (auto particleMesh: meshViews_) {
		particleMesh->updateVAO(bufferRange_.buffer_);
	}

	// Ping-Pong VBO references so that next feedback goes to other buffer.
	updateIdx_ = nextIdx;

	// TODO: Update bounding box every ~166 ms.
	//      Problem: No support yet for binding VBO as SSBO.
	//      The current interface requires StagedBuffer.
	/**
	bbox_time_ += dt;
	if (bbox_time_ > 166.0) {
		bboxBuffer_->clear();
		bboxPass_->enable(rs);
		bboxPass_->disable(rs);
		// update the grid in case the bounding box around the boids changed.
		if(bboxBuffer_->updateBoundingBox()) {
			auto &newBounds = bboxBuffer_->bbox();
			if (newBounds.max != newBounds.min) {
				set_bounds(newBounds.min, newBounds.max);
			}
		}
		bbox_time_ = 0.0;
	}
	**/
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
