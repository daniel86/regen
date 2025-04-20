#include <regen/textures/texture-state.h>
#include "lightning-bolt.h"

using namespace regen;

#define ATTRIBUTE_NAME_BRIGHTNESS "brightness"

LightningBolt::Config::Config() = default;

LightningBolt::LightningBolt(const Config &cfg)
		: Mesh(GL_LINES, BUFFER_USAGE_DYNAMIC_DRAW),
		  Animation(true, false),
		  maxSubDivisions_(cfg.maxSubDivisions_),
		  maxBranches_(cfg.maxBranches_) {
	frequencyConfig_ = ref_ptr<ShaderInput2f>::alloc("lightningFrequency");
	frequencyConfig_->setUniformData(Vec2f(0.0f, 0.0f));
	joinShaderInput(frequencyConfig_);

	lifetimeConfig_ = ref_ptr<ShaderInput2f>::alloc("lightningLifetime");
	lifetimeConfig_->setUniformData(Vec2f(2.0f, 0.5f));
	joinShaderInput(lifetimeConfig_);

	jitterOffset_ = ref_ptr<ShaderInput1f>::alloc("lightningMaxOffset");
	jitterOffset_->setUniformData(2.0f);
	joinShaderInput(jitterOffset_);

	branchProbability_ = ref_ptr<ShaderInput1f>::alloc("branchProbability");
	branchProbability_->setUniformData(0.5f);
	joinShaderInput(branchProbability_);

	branchOffset_ = ref_ptr<ShaderInput1f>::alloc("branchOffset");
	branchOffset_->setUniformData(2.0f);
	joinShaderInput(branchOffset_);

	branchLength_ = ref_ptr<ShaderInput1f>::alloc("branchLength");
	branchLength_->setUniformData(0.7f);
	joinShaderInput(branchLength_);

	branchDarkening_ = ref_ptr<ShaderInput1f>::alloc("branchDarkening");
	branchDarkening_->setUniformData(0.5f);
	joinShaderInput(branchDarkening_);

	matAlpha_ = ref_ptr<ShaderInput1f>::alloc("matAlpha");
	matAlpha_->setUniformData(0.0f);
	joinShaderInput(matAlpha_);

	source_ = ref_ptr<ShaderInput3f>::alloc("lightningSource");
	source_->setUniformData(Vec3f(0.0f, 60.0f, 0.0f));
	joinShaderInput(source_);

	target_ = ref_ptr<ShaderInput3f>::alloc("lightningTarget");
	target_->setUniformData(Vec3f(0.0f, 0.0f, 0.0f));
	joinShaderInput(target_);

	// allocate vertex data for max number of segments and branches,
	// particular bolts will have less segments and branches.
	unsigned int numSegments = static_cast<int>(pow(2, maxSubDivisions_)) * maxBranches_;
	unsigned int numVertices = numSegments * 2;
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	pos_->setVertexData(numVertices);
	brightness_ = ref_ptr<ShaderInput1f>::alloc(ATTRIBUTE_NAME_BRIGHTNESS);
	brightness_->setVertexData(numVertices);

	begin(InputContainer::INTERLEAVED);
	setInput(pos_);
	setInput(brightness_);
	end();
	bufferOffset_ = std::min(pos_->offset(), brightness_->offset());
	elementSize_ = pos_->elementSize() + brightness_->elementSize();
}

LightningBolt::LightningBolt(const ref_ptr<LightningBolt> &other)
		: Mesh(other),
		  Animation(true, false),
		  maxSubDivisions_(other->maxSubDivisions_),
		  maxBranches_(other->maxBranches_),
		  bufferOffset_(other->bufferOffset_),
		  elementSize_(other->elementSize_) {
	frequencyConfig_ = other->frequencyConfig_;
	lifetimeConfig_ = other->lifetimeConfig_;
	jitterOffset_ = other->jitterOffset_;
	branchProbability_ = other->branchProbability_;
	branchOffset_ = other->branchOffset_;
	branchLength_ = other->branchLength_;
	branchDarkening_ = other->branchDarkening_;

	matAlpha_ = ref_ptr<ShaderInput1f>::alloc("matAlpha");
	matAlpha_->setUniformData(0.0f);
	joinShaderInput(matAlpha_);

	source_ = ref_ptr<ShaderInput3f>::alloc("lightningSource");
	source_->setUniformData(other->source_->getVertex(0).r);
	joinShaderInput(source_);

	target_ = ref_ptr<ShaderInput3f>::alloc("lightningTarget");
	target_->setUniformData(other->target_->getVertex(0).r);
	joinShaderInput(target_);

	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_POS));
	brightness_ = ref_ptr<ShaderInput1f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_BRIGHTNESS));
}

void LightningBolt::setLifetime(float base, float variance) {
	lifetimeConfig_->setVertex(0, Vec2f(base, variance));
}

bool LightningBolt::isBoltActive() const {
	return u_lifetime_ > 0.0f;
}

void LightningBolt::setFrequency(float base, float variance) {
	frequencyConfig_->setVertex(0, Vec2f(base, variance));
	hasFrequency_ = (base > 0.0f);
}

void LightningBolt::setJitterOffset(float maxOffset) {
	jitterOffset_->setVertex(0, maxOffset);
}

void LightningBolt::setBranchProbability(float probability) {
	branchProbability_->setVertex(0, probability);
}

void LightningBolt::setBranchOffset(float offset) {
	branchOffset_->setVertex(0, offset);
}

void LightningBolt::setBranchLength(float length) {
	branchLength_->setVertex(0, length);
}

void LightningBolt::setBranchDarkening(float darkening) {
	branchDarkening_->setVertex(0, darkening);
}

void LightningBolt::setNextStrike() {
	auto frequency = frequencyConfig_->getVertex(0);
	u_nextStrike_ = -(frequency.r.x + frequency.r.y * (math::random<float>() * 2.0f - 1.0f));
}

void LightningBolt::updateBolt(double dt_s) {
	u_lifetime_ -= dt_s;
	if (hasFrequency_ && u_lifetime_ < u_nextStrike_) {
		strike();
	}
	matAlpha_->setVertex(0, static_cast<float>(std::min(1.0, std::max(0.0,
			u_lifetime_ / (u_lifetimeBegin_ * 0.5)))));
}

void LightningBolt::glAnimate(RenderState *rs, GLdouble dt) {
	// TODO: this computation is blocking the GPU thread! it could be done in a separate thread,
	//       but then a ping-pong buffer should be used to avoid synchronization issues.
	updateBolt(dt * 0.001);
	if (isBoltActive() != isActive_) {
		isActive_ = !isActive_;
		set_isHidden(!isActive_);
	}
}

void LightningBolt::setSourcePosition(const ref_ptr<regen::ShaderInput3f> &from) {
	disjoinShaderInput(source_);
	source_ = from;
	joinShaderInput(source_);
}

void LightningBolt::setSourcePosition(const regen::Vec3f &from) {
	source_->setVertex(0, from);
}

void LightningBolt::setTargetPosition(const ref_ptr<regen::ShaderInput3f> &to) {
	disjoinShaderInput(target_);
	target_ = to;
	joinShaderInput(target_);
}

void LightningBolt::setTargetPosition(const regen::Vec3f &to) {
	target_->setVertex(0, to);
}

void LightningBolt::updateVertexData() {
	unsigned int numVertices = segments_[segmentIndex_].size() * 2;
	RenderState::get()->copyWriteBuffer().push(pos_->buffer());
	glBufferSubData(GL_COPY_WRITE_BUFFER,
					bufferOffset_,
					numVertices * elementSize_,
					segments_[segmentIndex_].data());
	RenderState::get()->copyWriteBuffer().pop();
	// update the number of vertices
	inputContainer()->set_numVertices(numVertices);
}

static Vec3f getPerpendicular1(const Vec3f &v) {
	if (std::abs(v.x) < std::abs(v.y) && std::abs(v.x) < std::abs(v.z)) {
		return v.cross(Vec3f::right());
	} else if (std::abs(v.y) < std::abs(v.z)) {
		return v.cross(Vec3f::up());
	} else {
		return v.cross(Vec3f::front());
	}
}

static Vec3f getPerpendicular(const Vec3f &v) {
	// Compute a vector perpendicular to v
	auto w = getPerpendicular1(v);
	w.normalize();
	// Generate a random angle
	auto phi = static_cast<float>(math::random<float>() * 2.0f * M_PI);
	// Rotate the perpendicular vector around v by phi
	return w * std::cos(phi) + v.cross(w) * std::sin(phi);
}

static Vec3f branch(
		const Vec3f &start,
		const Vec3f &dir,
		const Vec3f &segmentEnd,
		float offsetAmount,
		float branchLength) {
	Vec3f v = segmentEnd;
	v += getPerpendicular(dir) * (offsetAmount * (math::random<float>() * 2.0f - 1.0f));
	v -= start;
	v.normalize();
	// pull the end point of the branch closer to origin by factor lengthScale
	return start + v * dir.length() * branchLength;
}

void LightningBolt::strike(const Vec3f &source, const Vec3f &target) {
	setSourcePosition(source);
	setTargetPosition(target);
	strike();
}

void LightningBolt::updateSegmentData() {
	// update CPU segment data.
	// we use two std::vectors in the process which are swapped for each subdivision.
	// internally the vector should keep the memory allocated, so there should be no
	// reallocation of memory after the first iteration.
	// the memory in the vector is further aligned with the GL buffer such that we can
	// directly copy the data to the buffer.
	// TODO: could rather generate a 2D bolt oriented wrt the camera.
	auto source = source_->getVertex(0);
	auto target = target_->getVertex(0);
	segments_[1].clear();
	segments_[0].clear();
	segments_[0].emplace_back(source.r, target.r);
	segmentIndex_ = 0;
	unsigned int nextIndex = 1;
	float offsetAmount = jitterOffset_->getVertex(0).r;
	float heightRange = source.r.y - target.r.y;
	auto branchProbability = branchProbability_->getVertex(0);
	auto branchOffset = branchOffset_->getVertex(0);
	auto branchDarkening = branchDarkening_->getVertex(0);

	int numMainBranchVertices = static_cast<int>(pow(2, maxSubDivisions_));
	int numSubBranchVertices = numMainBranchVertices;
	int numRemainingVertices = numMainBranchVertices * static_cast<int>(maxBranches_ - 1);

	// update the lifetime
	auto lifetime_cfg = lifetimeConfig_->getVertex(0);
	u_lifetime_ = lifetime_cfg.r.x + lifetime_cfg.r.y * (math::random<float>() * 2.0f - 1.0f);
	u_lifetimeBegin_ = u_lifetime_;

	Vec3f midPoint, direction;
	for (unsigned int i = 0u; i < maxSubDivisions_; ++i) {
		numSubBranchVertices /= 2;

		for (auto &segment: segments_[segmentIndex_]) {
			midPoint = (segment.start.pos + segment.end.pos) * 0.5f;
			direction = segment.end.pos - segment.start.pos;
			direction.normalize();
			midPoint += getPerpendicular(direction) * (offsetAmount * (math::random<float>() * 2.0f - 1.0f));
			segments_[nextIndex].emplace_back(segment.start.pos, midPoint, segment.start.brightness);
			segments_[nextIndex].emplace_back(midPoint, segment.end.pos, segment.end.brightness);

			direction = midPoint - segment.start.pos;
			// with some probability create a new branch.
			if (numRemainingVertices > numSubBranchVertices &&
			    math::random<float>() < branchProbability.r *
			    	// decrease the probability for sub-branches
					std::min(1.0f, segment.start.brightness + 0.25f) *
					// decrease the probability further away from the target
					std::min(1.0f, (1.0f - (midPoint.y - target.r.y) / heightRange) + 0.25f)) {
				segments_[nextIndex].emplace_back(
						midPoint,
						branch(midPoint, direction, segment.end.pos,
							   offsetAmount * branchOffset.r,
							   branchLength_->getVertex(0).r),
						// decrease brightness for sub-branches
						segment.start.brightness * branchDarkening.r);
				numRemainingVertices -= numSubBranchVertices;
			}
		}
		segments_[segmentIndex_].clear();
		nextIndex = segmentIndex_;
		segmentIndex_ = 1 - segmentIndex_;
		offsetAmount *= 0.5f;
	}
}

void LightningBolt::strike() {
	updateSegmentData();
	updateVertexData();
	setNextStrike();
	segments_[segmentIndex_].clear();
}
