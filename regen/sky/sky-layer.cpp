#include "regen/sky/sky-layer.h"
#include "regen/sky/sky.h"

using namespace regen;

SkyLayer::SkyLayer(const ref_ptr<Sky> &sky) {
	sky_ = sky;
	updateInterval_ = 4000.0;
	updateState_ = ref_ptr<State>::alloc();
	dt_ = updateInterval_;
}

SkyLayer::~SkyLayer() = default;

bool SkyLayer::advanceTime(double dt) {
	dt_ += dt;
	return (dt_ >= updateInterval_);
}

void SkyLayer::updateSky(RenderState *rs, GLdouble dt) {
	updateSkyLayer(rs, dt_);
	dt_ = 0.0;
}
