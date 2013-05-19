/*
 * distance-fog.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include <regen/meshes/rectangle.h>
#include <regen/states/blend-state.h>

#include "distance-fog.h"
using namespace regen;

DistanceFog::DistanceFog() : FullscreenPass("regen.shading.fog.distance")
{
  fogColor_ = ref_ptr<ShaderInput3f>::alloc("fogColor");
  fogColor_->setUniformData(Vec3f(1.0));
  joinShaderInput(fogColor_);

  fogDistance_ = ref_ptr<ShaderInput2f>::alloc("fogDistance");
  fogDistance_->setUniformData(Vec2f(0.0,100.0));
  joinShaderInput(fogDistance_);

  fogDensity_ = ref_ptr<ShaderInput1f>::alloc("fogDensity");
  fogDensity_->setUniformData(1.0);
  joinShaderInput(fogDensity_);

  // add blend fog on top of scene
  joinStatesFront(ref_ptr<BlendState>::alloc(BLEND_MODE_ALPHA));
}

void DistanceFog::set_gBuffer(
    const ref_ptr<Texture> &depth)
{
  if(gDepthTexture_.get()) {
    disjoinStates(gDepthTexture_);
  }
  gDepthTexture_ = ref_ptr<TextureState>::alloc(depth,"gDepthTexture");
  joinStatesFront(gDepthTexture_);
}
void DistanceFog::set_tBuffer(
    const ref_ptr<Texture> &color,
    const ref_ptr<Texture> &depth)
{
  shaderDefine("USE_TBUFFER", depth.get()?"TRUE":"FALSE");
  if(tDepthTexture_.get()) {
    disjoinStates(tDepthTexture_);
    disjoinStates(tColorTexture_);
  }
  if(color.get()) {
    tColorTexture_ = ref_ptr<TextureState>::alloc(color);
    tColorTexture_->set_name("tColorTexture");
    joinStatesFront(tColorTexture_);
  }
  if(depth.get()) {
    tDepthTexture_ = ref_ptr<TextureState>::alloc(depth,"tDepthTexture");
    joinStatesFront(tDepthTexture_);
  }
}
void DistanceFog::set_skyColor(const ref_ptr<TextureCube> &t)
{
  shaderDefine("USE_SKY_COLOR", t.get()?"TRUE":"FALSE");
  if(skyColorTexture_.get()) {
    disjoinStates(skyColorTexture_);
  }
  if(t.get()) {
    skyColorTexture_ = ref_ptr<TextureState>::alloc(t,"skyColorTexture");
    joinStatesFront(skyColorTexture_);
  }
}

const ref_ptr<ShaderInput3f>& DistanceFog::fogColor() const
{ return fogColor_; }
const ref_ptr<ShaderInput2f>& DistanceFog::fogDistance() const
{ return fogDistance_; }
const ref_ptr<ShaderInput1f>& DistanceFog::fogDensity() const
{ return fogDensity_; }
