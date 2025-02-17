#include "regen/sky/sky-layer.h"
#include "regen/sky/sky.h"

using namespace regen;

SkyLayer::SkyLayer(const ref_ptr<Sky> &sky) {
	sky_ = sky;
	updateInterval_ = 4000.0;
	dt_ = updateInterval_;
}

SkyLayer::~SkyLayer() = default;

void SkyLayer::updateSky(RenderState *rs, GLdouble dt) {
	dt_ += dt;
	if (dt_ < updateInterval_) { return; }

	updateSkyLayer(rs, dt_);
	GL_ERROR_LOG();

	dt_ = 0.0;
}
