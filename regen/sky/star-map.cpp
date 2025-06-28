#include "star-map.h"

#include <regen/sky/earth.h>
#include <regen/textures/texture-loader.h>

using namespace regen;

StarMap::StarMap(const ref_ptr<Sky> &sky, GLint levelOfDetail)
		: SkyLayer(sky) {
	state()->joinStates(ref_ptr<BlendState>::alloc(BLEND_MODE_SRC));

	scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
	scattering_->setUniformData(defaultScattering());
	state()->joinShaderInput(scattering_);

	deltaM_ = ref_ptr<ShaderInput1f>::alloc("deltaM");
	deltaM_->setUniformData(0.5f);
	state()->joinShaderInput(deltaM_);

	set_apparentMagnitude(6.5);

	meshState_ = ref_ptr<SkyBox>::alloc(levelOfDetail, "regen.weather.star-map");
}

float StarMap::defaultScattering() {
	return 0.2;
}

void StarMap::set_texture(const std::string &textureFile) {
	meshState_->setCubeMap(textures::loadCube(textureFile));
}

void StarMap::set_apparentMagnitude(float apparentMagnitude) {
	// Precompute brightness based on logarithmic scale.
	// (Similar to starsgeode vertex shader.)
	deltaM_->setVertex(0, powf(2.512f, apparentMagnitude - Earth::apparentMagnitudeLimit()));
}

void StarMap::set_deltaMagnitude(float deltaMagnitude) {
	deltaM_->setVertex(0, deltaMagnitude);
}
