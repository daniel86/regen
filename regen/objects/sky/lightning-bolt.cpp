#include <regen/textures/texture-state.h>
#include "lightning-bolt.h"

using namespace regen;

#define ATTRIBUTE_NAME_BRIGHTNESS "brightness"

LightningStrike::LightningStrike(uint32_t strikeIdx,
		const StrikePoint &source, const StrikePoint &target,
		const ref_ptr<ShaderInput1f> &alpha)
	: strikeIdx_(strikeIdx),
	  source_(source),
	  target_(target),
	  alpha_(alpha) {
	drawIndex_.store(0u, std::memory_order_release);
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

void LightningStrike::updateSegmentData() {
	Vec3f source;
	if (source_.pos.get() != nullptr) {
		source = source_.pos->getVertex(source_.instance).r;
	} else if (source_.tf.get() != nullptr) {
		source = source_.tf->position(source_.instance).r;
	} else {
		REGEN_WARN("Ignoring LightningStrike, no valid source defined.");
		return;
	}

	Vec3f target;
	if (target_.pos.get() != nullptr) {
		target = target_.pos->getVertex(target_.instance).r;
	} else if (target_.tf.get() != nullptr) {
		target = target_.tf->position(target_.instance).r;
	} else {
		REGEN_WARN("Ignoring LightningStrike, no valid target defined.");
		return;
	}

	updateSegmentData(source, target);
}

void LightningStrike::SegmentData::push_back(
		const Vec3f &start,
		const Vec3f &end,
		uint32_t strikeIdx,
		float brightness) {
	pos_.push_back(start);
	pos_.push_back(end);
	brightness_.push_back(brightness);
	brightness_.push_back(brightness);
	strikeIdx_.push_back(strikeIdx);
	strikeIdx_.push_back(strikeIdx);
}

void LightningStrike::updateSegmentData(const Vec3f &source, const Vec3f &target) {
	// update CPU segment data.
	// we use two std::vectors in the process which are swapped for each subdivision.
	// internally the vector should keep the memory allocated, so there should be no
	// reallocation of memory after the first iteration.
	// the memory in the vector is further aligned with the GL buffer such that we can
	// directly copy the data to the buffer.
	const uint32_t currentDrawIdx = drawIndex_.load(std::memory_order_acquire);
	// Obtain the two update indices, which are not the current draw index
	// as we should not modify the segment being drawn.
	// NOTE: this logic will break if the number of segment buffers is changed from 3 to something else.
	const uint32_t updateIdx[2] = {
		(currentDrawIdx == 0u) ? 1u : 0u,
		(currentDrawIdx == 2u) ? 1u : 2u
	};
	SegmentData *updateSegments[2] = {
		&segments_[updateIdx[0]],
		&segments_[updateIdx[1]]
	};

	updateSegments[1]->clear();
	updateSegments[0]->clear();
	updateSegments[0]->push_back(source, target, strikeIdx_);
	uint32_t lastIndex = 0, nextIndex = 1;
	float offsetAmount = jitterOffset_;
	float heightRange = source.y - target.y;

	int numMainBranchVertices = static_cast<int>(pow(2, maxSubDivisions_));
	int numSubBranchVertices = numMainBranchVertices;
	int numRemainingVertices = numMainBranchVertices * static_cast<int>(maxBranches_ - 1);

	for (unsigned int i = 0u; i < maxSubDivisions_; ++i) {
		numSubBranchVertices /= 2;

		auto &segmentData = updateSegments[lastIndex];
		for (uint32_t segmentIdx = 0u; segmentIdx < segmentData->pos_.size() / 2; ++segmentIdx) {
			const uint32_t startIdx = segmentIdx * 2;
			const uint32_t endIdx = startIdx + 1;

			const Vec3f &startPos = segmentData->pos_[startIdx];
			const Vec3f &endPos = segmentData->pos_[endIdx];

			const float startBrightness = segmentData->brightness_[startIdx];
			const float endBrightness = segmentData->brightness_[endIdx];

			Vec3f midPoint = (startPos + endPos) * 0.5f;
			Vec3f direction = endPos - startPos;
			direction.normalize();
			midPoint += getPerpendicular(direction) * (offsetAmount * (math::random<float>() * 2.0f - 1.0f));
			updateSegments[nextIndex]->push_back(
				startPos, midPoint,
				strikeIdx_,
				startBrightness);
			updateSegments[nextIndex]->push_back(
				midPoint, endPos,
				strikeIdx_,
				endBrightness);

			direction = midPoint - startPos;
			// with some probability create a new branch.
			if (numRemainingVertices > numSubBranchVertices &&
				math::random<float>() < branchProbability_ *
										// decrease the probability for sub-branches
										std::min(1.0f, startBrightness + 0.25f) *
										// decrease the probability further away from the target
										std::min(1.0f, (1.0f - (midPoint.y - target.y) / heightRange) + 0.25f)) {
				updateSegments[nextIndex]->push_back(
						midPoint,
						branch(midPoint, direction, endPos,
							   offsetAmount * branchOffset_,
							   branchLength_),
						strikeIdx_,
						// decrease brightness for sub-branches
						startBrightness * branchDarkening_);
				numRemainingVertices -= numSubBranchVertices;
			}
		}

		updateSegments[lastIndex]->clear();
		nextIndex = lastIndex;
		lastIndex = 1 - lastIndex;
		offsetAmount *= 0.5f;
	}

	const uint32_t nextDrawIdx = updateIdx[lastIndex];
	drawIndex_.store(nextDrawIdx, std::memory_order_release);
}

bool LightningStrike::updateStrike(double dt_s) {
	u_time_ += dt_s;

	if (active_) {
		static const float maxAlpha_ = 1.0f;

		if (u_time_ >= u_maxLifetime_) {
			u_time_ = 0.0;
			active_ = false;
			u_nextStrike_ = frequencyConfig_.x + frequencyConfig_.y * (math::random<float>() * 2.0f - 1.0f);
			alpha_->setVertex(strikeIdx_, 0.0f);
		} else {
			float fade = (1.0f - static_cast<float>(u_time_ / u_maxLifetime_)) * maxAlpha_;
			fade = std::min(1.0f, std::max(0.0f, fade));
			alpha_->setVertex(strikeIdx_, fade);
		}
		return true;
	}
	else if (u_time_ >= u_nextStrike_) {
		active_ = true;
		u_time_ = 0.0;
		u_maxLifetime_ = lifetimeConfig_.x + lifetimeConfig_.y * (math::random<float>() * 2.0f - 1.0f);
		if (source_.randomizeInstance) {
			source_.instance = static_cast<uint32_t>(math::randomInt() % source_.numInstances);
		}
		if (target_.randomizeInstance) {
			target_.instance = static_cast<uint32_t>(math::randomInt() % target_.numInstances);
		}
		alpha_->setVertex(strikeIdx_, 0.0f);
		updateSegmentData();
		return true;
	} else {
		return false;
	}
}

LightningBolt::LightningBolt()
		: Mesh(GL_LINES, BufferUpdateFlags::PARTIAL_PER_FRAME),
		  Animation(true, true) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	brightness_ = ref_ptr<ShaderInput1f>::alloc(ATTRIBUTE_NAME_BRIGHTNESS);
	strikeIdx_ = ref_ptr<ShaderInput1ui>::alloc("strikeIdx");

	strikeSSBO_ = ref_ptr<SSBO>::alloc("LightningStrikes", BufferUpdateFlags::FULL_PER_FRAME);
	strikeAlpha_ = ref_ptr<ShaderInput1f>::alloc("strikeAlpha");
	strikeWidth_ = ref_ptr<ShaderInput1f>::alloc("strikeWidth");
	strikeSSBO_->addStagedInput(strikeAlpha_);
	strikeSSBO_->addStagedInput(strikeWidth_);
	setInput(strikeSSBO_);
}

static LightningStrike::StrikePoint getStrikeSource(LoadingContext &ctx, const ref_ptr<scene::SceneInputNode> &input) {
	LightningStrike::StrikePoint source;
	if (input->hasAttribute("source-constant")) {
		Vec3f pos = input->getValue<Vec3f>("source-constant", Vec3f::zero());
		source.pos = ref_ptr<ShaderInput3f>::alloc("sourcePos");
		source.pos->setUniformData(pos);
	} else if (input->hasAttribute("source-tf")) {
		ref_ptr<ModelTransformation> tf = ctx.scene()->getResource<ModelTransformation>(input->getValue("source-tf"));
		if (tf.get() != nullptr) {
			source.tf = tf;
			source.numInstances = tf->numInstances();
		} else {
			REGEN_WARN("Ignoring " << input->getDescription() << ", failed to load source transformation '" << input->getValue("source-tf") << "'.");
		}
	}
	if (input->hasAttribute("source-instance")) {
		source.instance = input->getValue<unsigned int>("source-instance", 0u);
	}
	return source;
}

static LightningStrike::StrikePoint getStrikeTarget(LoadingContext &ctx, const ref_ptr<scene::SceneInputNode> &input) {
	LightningStrike::StrikePoint target;
	if (input->hasAttribute("target-constant")) {
		Vec3f pos = input->getValue<Vec3f>("target-constant", Vec3f::zero());
		target.pos = ref_ptr<ShaderInput3f>::alloc("targetPos");
		target.pos->setUniformData(pos);
	} else if (input->hasAttribute("target-tf")) {
		ref_ptr<ModelTransformation> tf = ctx.scene()->getResource<ModelTransformation>(input->getValue("target-tf"));
		if (tf.get() != nullptr) {
			target.tf = tf;
			target.numInstances = tf->numInstances();
		} else {
			REGEN_WARN("Ignoring " << input->getDescription() << ", failed to load target transformation '" << input->getValue("target-tf") << "'.");
		}
	}
	if (input->hasAttribute("target-instance")) {
		target.instance = input->getValue<unsigned int>("target-instance", 0u);
	}
	return target;
}

static LightningStrike::StrikePoint getStrikePoint_tf(
			LoadingContext &ctx,
			const ref_ptr<scene::SceneInputNode> &input,
			const std::string &attrName) {
	LightningStrike::StrikePoint strikePoint;
	ref_ptr<ModelTransformation> tf = ctx.scene()->getResource<ModelTransformation>(input->getValue(attrName));
	if (tf.get() != nullptr) {
		strikePoint.tf = tf;
		strikePoint.numInstances = tf->numInstances();
	} else {
		REGEN_WARN("Ignoring " << input->getDescription() << ", failed to load target transformation '" << input->getValue(attrName) << "'.");
	}
	return strikePoint;
}

static void selectTargetInstance(
		const LightningStrike::StrikePoint &source,
		LightningStrike::StrikePoint &target,
		const ref_ptr<scene::SceneInputNode> &input) {
	if (target.numInstances <= 1u) return;
	if (input->hasAttribute("target-instance")) return;
	if (!target.randomizeInstance && source.instance < target.numInstances) {
		target.instance = source.instance;
	} else {
		target.instance = static_cast<uint32_t>(math::randomInt() % target.numInstances);
	}
}

using StrikeDescr = std::pair< LightningStrike::StrikePoint, LightningStrike::StrikePoint>;

static std::vector<StrikeDescr> getStrikePoints(LoadingContext &ctx, const ref_ptr<scene::SceneInputNode> &input) {
	std::vector<StrikeDescr> points;
	if (input->hasAttribute("ring-tf")) {
		// Create a ring of strikes ranging over all instances
		LightningStrike::StrikePoint ringPoint = getStrikePoint_tf(ctx, input, "ring-tf");
		const uint32_t ringStrength = input->getValue<unsigned int>("ring-strength", 1u);
		for (uint32_t ringIdx= 0u; ringIdx < ringStrength; ++ringIdx) {
			for (uint32_t instance=0u; instance < ringPoint.numInstances; ++instance) {
				auto source = ringPoint;
				auto target = ringPoint;
				source.instance = instance;
				target.instance = (instance + 1) % ringPoint.numInstances;
				points.emplace_back(source, target);
			}
		}
	} else {
		auto source = getStrikeSource(ctx, input);
		auto target = getStrikeTarget(ctx, input);
		if (input->hasAttribute("source-instance")) {
			selectTargetInstance(source, target, input);
			points.emplace_back(source, target);
		} else {
			uint32_t maxInstances = source.numInstances;
			if (input->hasAttribute("source-max-instances")) {
				maxInstances = std::min(maxInstances,
					input->getValue<unsigned int>("source-max-instances", maxInstances));
			}
			for (uint32_t instance=0u; instance < maxInstances; ++instance) {
				source.instance = instance;
				selectTargetInstance(source, target, input);
				points.emplace_back(source, target);
			}
		}
	}
	return points;
}

void LightningBolt::load(LoadingContext &ctx, scene::SceneInputNode &input) {
	std::vector<ref_ptr<scene::SceneInputNode>> handledChildren;

	uint32_t strikeIdx = 0u;
	for (auto &child : input.getChildren("strike")) {
		auto strikePoints = getStrikePoints(ctx, child);
		for (auto &[strikeSource, strikeTarget] : strikePoints) {
			auto strike = ref_ptr<LightningStrike>::alloc(
					strikeIdx++,
					strikeSource,
					strikeTarget,
					strikeAlpha_);
			if (child->hasAttribute("num-subdivisions")) {
				strike->maxSubDivisions_ = child->getValue<unsigned int>("num-subdivisions", 5);
			}
			if (child->hasAttribute("max-branches")) {
				strike->maxBranches_ = child->getValue<unsigned int>("max-branches", 3);
			}
			if (child->hasAttribute("lifetime")) {
				auto lifetime = child->getValue<Vec2f>("lifetime", Vec2f(2.0f, 0.5f));
				strike->lifetimeConfig_ = lifetime;
			}
			if (child->hasAttribute("jitter-offset")) {
				strike->jitterOffset_ = child->getValue<float>("jitter-offset", 8.0f);
			}
			if (child->hasAttribute("frequency")) {
				auto frequency = child->getValue<Vec2f>("frequency", Vec2f::zero());
				strike->frequencyConfig_ = frequency;
				strike->hasFrequency_ = (frequency.x != 0.0f || frequency.y != 0.0f);
			}
			if (child->hasAttribute("branch-probability")) {
				strike->branchProbability_ = child->getValue<float>("branch-probability", 0.5f);
			}
			if (child->hasAttribute("branch-offset")) {
				strike->branchOffset_ = child->getValue<float>("branch-offset", 0.5f);
			}
			if (child->hasAttribute("branch-length")) {
				strike->branchLength_ = child->getValue<float>("branch-length", 0.5f);
			}
			if (child->hasAttribute("branch-darkening")) {
				strike->branchDarkening_ = child->getValue<float>("branch-darkening", 0.5f);
			}
			if (child->hasAttribute("width-factor")) {
				strike->widthFactor_ = child->getValue<float>("width-factor", 0.5f);
			}
			addLightningStrike(strike);
		}
		handledChildren.push_back(child);
	}
	for (auto &handled : handledChildren) {
		input.removeChild(handled);
	}
	createResources();
}

void LightningBolt::addLightningStrike(const ref_ptr<LightningStrike> &strike) {
	strikes_.push_back(strike);
}

void LightningBolt::createResources() {
	// allocate vertex data for max number of segments and branches,
	// particular bolts will have less segments and branches.
	unsigned int numSegments = 0;
	unsigned int numVertices = 0;
	for (auto &strike : strikes_) {
		numSegments = (static_cast<int>(pow(2, strike->maxSubDivisions_)) * strike->maxBranches_);
		numVertices += numSegments * 2;
	}
	REGEN_INFO("Allocating LightningBolt with " <<
		numVertices << " vertices for " << strikes_.size() << " strikes.");

	{
		pos_->setVertexData(numVertices);
		brightness_->setVertexData(numVertices);
		strikeIdx_->setVertexData(numVertices);

		setInput(pos_);
		setInput(brightness_);
		setInput(strikeIdx_);
		updateVertexData();

		// create single LOD level
		auto &lod = meshLODs_.emplace_back();
		lod.d->numVertices = numVertices;
		lod.d->vertexOffset = 0;
	}

	{
		std::vector<float> alphas(strikes_.size(), 0.0f);
		std::vector<float> widths(strikes_.size());
		for (size_t i = 0; i < strikes_.size(); ++i) {
			widths[i] = strikes_[i]->widthFactor_;
		}

		strikeAlpha_->set_numArrayElements(strikes_.size());
		strikeAlpha_->set_forceArray(true);
		strikeAlpha_->setUniformUntyped((byte*)alphas.data());

		strikeWidth_->set_numArrayElements(strikes_.size());
		strikeWidth_->set_forceArray(true);
		strikeWidth_->setUniformUntyped((byte*)widths.data());

		strikeSSBO_->update();
	}
}

void LightningBolt::cpuUpdate(double dt) {
	bool active = false;
	double dt_s = dt * 0.001;
	for (auto &strike : strikes_) {
		active = strike->updateStrike(dt_s) || active;
	}
	if (active != isActive_) {
		isActive_ = !isActive_;
		set_isHidden(!isActive_);
	}
}

void LightningBolt::gpuUpdate(RenderState *rs, double dt) {
	if (!isActive_) return;

	const uint32_t maxVertices = pos_->numVertices();
	uint32_t numVertices = 0u;

	auto m_pos = pos_->mapClientDataRaw(BUFFER_GPU_WRITE);
	auto m_brightness = brightness_->mapClientDataRaw(BUFFER_GPU_WRITE);
	auto m_strikeIdx = strikeIdx_->mapClientDataRaw(BUFFER_GPU_WRITE);

	auto* posData = (Vec3f*)m_pos.w;
	auto* brightnessData = (float*)m_brightness.w;
	auto* strikeIdxData = (uint32_t*)m_strikeIdx.w;

	for (auto &strike : strikes_) {
		// Load the index of last updated segment.
		// NOTE: This is safe because draw and update thread are frame-synced, i.e. update will swap
		// the draw idx only after writing, and won't write again until draw is definitely done.
		// Draw will either use update data of last frame, or if it is slower it may also pick
		// up the data just written, which is also fine.
		const uint32_t drawIdx = strike->drawIndex_.load(std::memory_order_acquire);
		LightningStrike::SegmentData &segment = strike->segments_[drawIdx];
		const uint32_t strikeVertices = segment.pos_.size();
		if (strikeVertices == 0 || numVertices + strikeVertices > maxVertices) { continue; }

		// Copy position data
		std::memcpy(
			posData + numVertices,
			segment.pos_.data(),
			strikeVertices * pos_->vertexSize());

		// Copy brightness data
		std::memcpy(
			brightnessData + numVertices,
			segment.brightness_.data(),
			strikeVertices * brightness_->vertexSize());

		// Copy strikeIdx data
		std::memcpy(
			strikeIdxData + numVertices,
			segment.strikeIdx_.data(),
			strikeVertices * strikeIdx_->vertexSize());

		numVertices += strikeVertices;
	}

	m_pos.unmap();
	m_brightness.unmap();
	m_strikeIdx.unmap();

	// update the number of vertices
	set_numVertices(numVertices);
	if (!meshLODs_.empty()) {
		meshLODs_.front().d->numVertices = numVertices;
	}
}
