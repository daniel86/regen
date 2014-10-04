/*
 * cloud-layer-low.cpp
 *
 *  Created on: Oct 4, 2014
 *      Author: daniel
 */

#include "cloud-layer-low.h"

#include <regen/external/osghimmel/noise.h>
#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>
#include <regen/meshes/rectangle.h>

using namespace regen;

LowCloudLayer::LowCloudLayer(const ref_ptr<Sky> &sky, GLuint textureSize)
: CloudLayer(sky,textureSize)
{
  set_name("Cloud-Layer[low]");

  // TODO: unused ?
  q_ = ref_ptr<ShaderInput1f>::alloc("q");
  q_->setUniformData(0.0f);
  state()->joinShaderInput(q_);

  set_altitude(defaultAltitude());
  set_scale(defaultScale());
  set_change(defaultChange());

  state()->joinShaderInput(sky->sunPositionR(), "sunPositionR");

  bottomColor_ = ref_ptr<ShaderInput3f>::alloc("bcolor");
  bottomColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
  state()->joinShaderInput(bottomColor_);

  topColor_ = ref_ptr<ShaderInput3f>::alloc("tcolor");
  topColor_->setUniformData(Vec3f(1.f, 1.f, 1.f));
  state()->joinShaderInput(topColor_);

  thickness_ = ref_ptr<ShaderInput1f>::alloc("thickness");
  thickness_->setUniformData(3.0f);
  state()->joinShaderInput(thickness_);

  offset_ = ref_ptr<ShaderInput1f>::alloc("offset");
  offset_->setUniformData(-0.5f);
  state()->joinShaderInput(offset_);

  shaderState_ = ref_ptr<HasShader>::alloc("regen.sky.clouds.low-layer");
  state()->joinStates(shaderState_->shaderState());

  meshState_ = Rectangle::getUnitQuad();
  state()->joinStates(meshState_);
}


const float LowCloudLayer::defaultAltitude()
{ return 2.0f; }

const Vec2f LowCloudLayer::defaultScale()
{ return Vec2f(128.0, 128.0); }

GLdouble LowCloudLayer::defaultChange()
{ return 0.1f; }

void LowCloudLayer::set_bottomColor(const Vec3f &color)
{ bottomColor_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& LowCloudLayer::bottomColor() const
{ return bottomColor_; }

void LowCloudLayer::set_topColor(const Vec3f &color)
{ topColor_->setVertex(0, color); }

const ref_ptr<ShaderInput3f>& LowCloudLayer::topColor() const
{ return topColor_; }

void LowCloudLayer::set_thickness(GLdouble thickness)
{ thickness_->setVertex(0, thickness); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::thickness() const
{ return thickness_; }

void LowCloudLayer::set_offset(GLdouble offset)
{ offset_->setVertex(0, offset); }

const ref_ptr<ShaderInput1f>& LowCloudLayer::offset() const
{ return offset_; }

#define _rad(deg) ((deg) * M_PI / 180.0L)

void LowCloudLayer::updateSkyLayer(RenderState *rs, GLdouble dt)
{
  // TODO: starmap and planets also require this ... - find better place
  const float fov = sky_->camera()->fov()->getVertex(0); // himmel.getCameraFovHint();
  const float height = sky_->viewport()->getVertex(0).x;
  q_->setUniformData(sqrt(2.0) * 2.0 * tan(_rad(fov * 0.5)) / height);
  CloudLayer::updateSkyLayer(rs,dt);
}

