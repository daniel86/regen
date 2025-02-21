/*
 * star-map.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "star-map.h"

#include <regen/meshes/primitives/rectangle.h>
#include <regen/external/osghimmel/earth.h>
#include <regen/textures/texture-loader.h>
#include <regen/states/depth-state.h>

using namespace regen;

StarMap::StarMap(const ref_ptr<Sky> &sky, GLint levelOfDetail)
		: SkyLayer(sky) {
	state()->joinStates(ref_ptr<BlendState>::alloc(GL_ONE, GL_ZERO));

	auto starsUniforms = ref_ptr<UniformBlock>::alloc("StarMap");
	state()->joinShaderInput(starsUniforms);

	scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
	scattering_->setUniformData(defaultScattering());
	starsUniforms->addUniform(scattering_);

	deltaM_ = ref_ptr<ShaderInput1f>::alloc("deltaM");
	deltaM_->setUniformData(0.5f);
	starsUniforms->addUniform(deltaM_);

	set_apparentMagnitude(6.5);

	meshState_ = ref_ptr<SkyBox>::alloc(levelOfDetail, "regen.sky.star-map");
}

GLdouble StarMap::defaultScattering() { return 0.2; }

void StarMap::set_texture(const std::string &textureFile) { meshState_->setCubeMap(textures::loadCube(textureFile)); }

void StarMap::set_scattering(GLdouble scattering) { scattering_->setVertex(0, scattering); }

void StarMap::set_apparentMagnitude(GLdouble apparentMagnitude) {
	// Precompute brightness based on logarithmic scale.
	// (Similar to starsgeode vertex shader.)
	deltaM_->setVertex(0, pow(2.512, apparentMagnitude -
									 static_cast<double>(osgHimmel::Earth::apparentMagnitudeLimit())));
}

void StarMap::set_deltaMagnitude(GLdouble deltaMagnitude) {
	deltaM_->setVertex(0, deltaMagnitude);
}

void StarMap::updateSkyLayer(RenderState *rs, GLdouble dt) {}


