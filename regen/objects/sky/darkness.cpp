#include "darkness.h"

using namespace regen;

Darkness::Darkness(const ref_ptr<Sky> &sky, int levelOfDetail)
		: SkyLayer(sky) {
	meshState_ = ref_ptr<SkyBox>::alloc(levelOfDetail, "regen.weather.darkness");
}
