#include "darkness.h"

using namespace regen;

Darkness::Darkness(const ref_ptr<Sky> &sky, GLint levelOfDetail)
		: SkyLayer(sky) {
	meshState_ = ref_ptr<SkyBox>::alloc(levelOfDetail, "regen.weather.darkness");
}
