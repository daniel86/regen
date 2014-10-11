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

StarMapLayer::StarMapLayer(const ref_ptr<Sky> &sky)
: SkyLayer(sky)
{
  state()->joinStates(ref_ptr<BlendState>::alloc(GL_ONE, GL_ZERO));

  starMap_ = ref_ptr<TextureState>::alloc();
  starMap_->set_name("starmapCube");
  state()->joinStates(starMap_);

  scattering_ = ref_ptr<ShaderInput1f>::alloc("scattering");
  scattering_->setUniformData(defaultScattering());
  state()->joinShaderInput(scattering_);

  apparentMagnitude_ = ref_ptr<ShaderInput1f>::alloc("apparentMagnitude");
  apparentMagnitude_->setUniformData(defaultApparentMagnitude());
  state()->joinShaderInput(apparentMagnitude_);

  colorRatio_ = ref_ptr<ShaderInput1f>::alloc("colorRatio");
  colorRatio_->setUniformData(defaultColorRatio());
  state()->joinShaderInput(colorRatio_);

  color_ = ref_ptr<ShaderInput3f>::alloc("color");
  color_->setUniformData(defaultColor());
  state()->joinShaderInput(color_);

  deltaM_ = ref_ptr<ShaderInput1f>::alloc("deltaM");
  deltaM_->setUniformData(1.f);
  state()->joinShaderInput(deltaM_);

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.star-map");
  //state()->joinStates(shaderState_->shaderState());

  meshState_ = Rectangle::getUnitQuad();
  //state()->joinStates(meshState_);
}


const GLdouble StarMapLayer::defaultScattering()
{ return 2.0; }

const GLdouble StarMapLayer::defaultApparentMagnitude()
{ return 6.0; }

const GLdouble StarMapLayer::defaultColorRatio()
{ return 0.33; }

const Vec3f StarMapLayer::defaultColor()
{ return Vec3f(0.66, 0.78, 1.00); }


void StarMapLayer::set_texture(const string &textureFile)
{ starMap_->set_texture(textures::loadCube(textureFile)); }

void StarMapLayer::set_scattering(GLdouble scattering)
{ scattering_->setVertex(0, scattering); }

const ref_ptr<ShaderInput1f>& StarMapLayer::scattering() const
{ return scattering_; }

void StarMapLayer::set_apparentMagnitude(GLdouble apparentMagnitude)
{
  apparentMagnitude_->setVertex(0, apparentMagnitude);

  // Precompute brightness based on logarithmic scale.
  // (Similar to starsgeode vertex shader.)
  deltaM_->setVertex(0, pow(2.512, apparentMagnitude_->getVertex(0) -
      static_cast<double>(osgHimmel::Earth::apparentMagnitudeLimit())));
}

const ref_ptr<ShaderInput1f>& StarMapLayer::apparentMagnitude() const
{ return apparentMagnitude_; }

void StarMapLayer::set_colorRatio(GLdouble colorRatio)
{ colorRatio_->setVertex(0, colorRatio); }

const ref_ptr<ShaderInput1f>& StarMapLayer::colorRatio() const
{ return colorRatio_; }

void StarMapLayer::set_color(const Vec3f &color)
{ color_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& StarMapLayer::color() const
{ return color_; }


ref_ptr<Mesh> StarMapLayer::getMeshState()
{ return meshState_; }

ref_ptr<HasShader> StarMapLayer::getShaderState()
{ return shaderState_; }


void StarMapLayer::updateSkyLayer(RenderState *rs, GLdouble dt)
{
}


