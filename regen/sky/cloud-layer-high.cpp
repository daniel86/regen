/*
 * cloud-layer-high.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "cloud-layer-high.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>

using namespace regen;

HighCloudLayer::HighCloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize)
: CloudLayer(sky,textureSize)
{
  set_name("Cloud-Layer[high]");

  color_ = ref_ptr<ShaderInput3f>::alloc("color");
  color_->setUniformData(Vec3f(1.f, 1.f, 1.f));
  state()->joinShaderInput(color_);

  set_altitude(defaultAltitude());
  set_scale(defaultScale());
  set_change(defaultChange());

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.clouds.high-layer");
  state()->joinStates(shaderState_->shaderState());

  meshState_ = Rectangle::getUnitQuad();
  state()->joinStates(meshState_);
}

const float HighCloudLayer::defaultAltitude()
{ return 8.0f; }

const Vec2f HighCloudLayer::defaultScale()
{ return Vec2f(32.0, 32.0); }

GLdouble HighCloudLayer::defaultChange()
{ return 0.1f; }

void HighCloudLayer::set_color(const Vec3f &color)
{ color_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& HighCloudLayer::color() const
{ return color_; }
