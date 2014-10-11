/*
 * star-map.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "star-map.h"

#include <regen/meshes/rectangle.h>
#include <regen/external/osghimmel/earth.h>
#include <regen/textures/texture-loader.h>
#include <regen/states/depth-state.h>

using namespace regen;

StarMap::StarMap(const ref_ptr<Sky> &sky)
: SkyLayer(sky)
{
  state()->joinStates(ref_ptr<BlendState>::alloc(GL_ONE, GL_ZERO));

  starMap_ = ref_ptr<TextureState>::alloc();
  starMap_->set_name("starmapCube");
  state()->joinStates(starMap_);

  scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
  scattering_->setUniformData(defaultScattering());
  state()->joinShaderInput(scattering_);

  deltaM_ = ref_ptr<ShaderInput1f>::alloc("deltaM");
  deltaM_->setUniformData(1.f);
  state()->joinShaderInput(deltaM_);

  set_apparentMagnitude(6.5);

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.star-map");
  meshState_ = Rectangle::getUnitQuad();
}


const GLdouble StarMap::defaultScattering()
{ return 0.2; }


void StarMap::set_texture(const string &textureFile)
{ starMap_->set_texture(textures::loadCube(textureFile)); }

void StarMap::set_scattering(GLdouble scattering)
{ scattering_->setVertex(0, scattering); }

const ref_ptr<ShaderInput1f>& StarMap::scattering() const
{ return scattering_; }

void StarMap::set_apparentMagnitude(GLdouble apparentMagnitude)
{
  // Precompute brightness based on logarithmic scale.
  // (Similar to starsgeode vertex shader.)
  deltaM_->setVertex(0, pow(2.512, apparentMagnitude -
      static_cast<double>(osgHimmel::Earth::apparentMagnitudeLimit())));
}


ref_ptr<Mesh> StarMap::getMeshState()
{ return meshState_; }

ref_ptr<HasShader> StarMap::getShaderState()
{ return shaderState_; }


void StarMap::updateSkyLayer(RenderState *rs, GLdouble dt)
{
}


