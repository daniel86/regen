#include "blanket.h"

#include "regen/shapes/bounding-shape.h"

using namespace regen;

static Rectangle::Config getRectangleConfig(const Blanket::BlanketConfig &cfg) {
	Rectangle::Config rectCfg;
	rectCfg.levelOfDetails = cfg.levelOfDetails;
	rectCfg.posScale.x = cfg.blanketSize.x;
	rectCfg.posScale.z = cfg.blanketSize.y;
	rectCfg.posScale.y = 1.0;
	rectCfg.rotation = Vec3f(0.0f, 0.0f, M_PIf);
	rectCfg.texcoScale = cfg.texcoScale;
	rectCfg.isNormalRequired = true;
	rectCfg.isTangentRequired = true;
	rectCfg.isTexcoRequired = true;
	rectCfg.centerAtOrigin = true;
	rectCfg.updateHint = cfg.updateHint;
	rectCfg.mapMode = cfg.mapMode;
	rectCfg.accessMode = cfg.accessMode;
	return rectCfg;
}

namespace regen {
	class BlanketLifetimeAnimation : public Animation {
	public:
		BlanketLifetimeAnimation(Blanket *blanket)
			: Animation(false, true),
			  blanket_(blanket) {
		}

		void cpuUpdate(double dt) override {
			blanket_->updateLifetime(dt * 0.001f);
		}

	protected:
		Blanket *blanket_;
	};
}

Blanket::Blanket(const BlanketConfig &cfg, uint32_t numInstances, const SystemTime &systemTime)
		: Rectangle(getRectangleConfig(cfg)),
          numBlankets_(numInstances),
		  blanketLifetimeMax_(cfg.blanketLifetime) {
	numDeadBlankets_ = cfg.isInitiallyDead ? numInstances : 0;
	deadBlankets_.resize(numInstances);
	systemTime_ = &systemTime;
	for (uint32_t i = 0; i < numInstances; ++i) {
		deadBlankets_[i] = i;
		if (cfg.isInitiallyDead && hasIndexedShapes()) {
			indexedShape(i)->setTraversalMask(0);
		}
	}
	blanketLifetime_.resize(numInstances, cfg.isInitiallyDead ? 0.0f : 1.0f);

	// create GL buffer for blanket data that is written per-frame but only for the few
	// blankets that were inserted.
	timeOfBirth_ = ref_ptr<ShaderInput1f>::alloc("timeOfBirth");
	timeOfBirth_->setInstanceData(numBlankets_, 1, (byte*)blanketLifetime_.data());

	blanketBuffer_ = ref_ptr<SSBO>::alloc("Blanket", BufferUpdateFlags::PARTIAL_PER_FRAME);
	blanketBuffer_->addStagedInput(timeOfBirth_);
	setInput(blanketBuffer_);

	// expose max lifetime as shader input
	auto maxLife = ref_ptr<ShaderInput1f>::alloc("maxLifetime");
	maxLife->setUniformData(blanketLifetimeMax_);
	setInput(maxLife);
}

void Blanket::updateAttributes() {
	Rectangle::updateAttributes();

	// initialize traversal masks
	blanketTraversalMask_ = BoundingShape::TRAVERSAL_BIT_DRAW;
	if (hasIndexedShapes() && numDeadBlankets_ > 0) {
		for (uint32_t i = 0; i < numBlankets_; ++i) {
			indexedShape(i)->setTraversalMask(0);
		}
	}

	if (blanketLifetimeMax_ > 0.0f) {
		lifetimeAnimation_ = ref_ptr<BlanketLifetimeAnimation>::alloc(this);
		lifetimeAnimation_->startAnimation();
	}
}

void Blanket::updateLifetime(float deltaSeconds) {
	if (blanketLifetimeMax_ <= 0.0f) {
		return; // infinite lifetime
	}
	float delta = deltaSeconds / blanketLifetimeMax_;
	for (uint32_t i = 0; i < numBlankets_; ++i) {
		if (blanketLifetime_[i] <= 0.0f) continue;
		blanketLifetime_[i] -= delta;
		if (blanketLifetime_[i] <= 0.0f) {
			blanketLifetime_[i] = 0.0f;
			deadBlankets_[numDeadBlankets_++] = i;
			if (!indexedShapes_->empty()) {
				// Disable rendering etc. of dead blankets.
				//   - the spatial index will not report any intersections with them.
				indexedShape(i)->setTraversalMask(0); // disable rendering etc.
			}
		}
	}
}

uint32_t Blanket::reviveBlanket() {
	if (numDeadBlankets_ == 0) {
		REGEN_WARN("No dead blankets to revive!");
		return 0; // no dead blankets
	} else {
		--numDeadBlankets_;
		auto idx = deadBlankets_[numDeadBlankets_];
		blanketLifetime_[idx] = 1.0f;

		const boost::posix_time::ptime &currentTime = systemTime_->p_time;
		static constexpr float timeScale = 1.0f / 1e+6f;
		timeOfBirth_->setVertex(idx, currentTime.time_of_day().total_microseconds() * timeScale);

		// reset traversal mask
		if (!indexedShapes_->empty()) {
			indexedShape(idx)->setTraversalMask(blanketTraversalMask_);
		}
		return idx;
	}
}
