/*
 * distance-fog.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include <ogle/meshes/rectangle.h>
#include <ogle/states/shader-configurer.h>
#include <ogle/states/blend-state.h>

#include "distance-fog.h"
using namespace ogle;

DistanceFog::DistanceFog() : FullscreenPass("fog.distance")
{
  fogColor_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("fogColor"));
  fogColor_->setUniformData(Vec3f(1.0));
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogColor_));

  fogDistance_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogDistance"));
  fogDistance_->setUniformData(Vec2f(0.0,100.0));
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogDistance_));

  fogDensity_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogDensity"));
  fogDensity_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogDensity_));

  // add blend fog on top of scene
  joinStatesFront(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
}

void DistanceFog::set_gBuffer(
    const ref_ptr<Texture> &depth)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
  }
  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depth,"gDepthTexture"));
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));
}
void DistanceFog::set_tBuffer(
    const ref_ptr<Texture> &color,
    const ref_ptr<Texture> &depth)
{
  shaderDefine("USE_TBUFFER", depth.get()?"TRUE":"FALSE");
  if(tDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  if(color.get()) {
    tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(color));
    tColorTexture_->set_name("tColorTexture");
    joinStatesFront(ref_ptr<State>::cast(tColorTexture_));
  }
  if(depth.get()) {
    tDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depth,"tDepthTexture"));
    joinStatesFront(ref_ptr<State>::cast(tDepthTexture_));
  }
}
void DistanceFog::set_skyColor(const ref_ptr<TextureCube> &t)
{
  shaderDefine("USE_SKY_COLOR", t.get()?"TRUE":"FALSE");
  if(skyColorTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(skyColorTexture_));
  }
  if(t.get()) {
    skyColorTexture_ = ref_ptr<TextureState>::manage(
        new TextureState(ref_ptr<Texture>::cast(t),"skyColorTexture"));
    joinStatesFront(ref_ptr<State>::cast(skyColorTexture_));
  }
}

const ref_ptr<ShaderInput3f>& DistanceFog::fogColor() const
{ return fogColor_; }
const ref_ptr<ShaderInput2f>& DistanceFog::fogDistance() const
{ return fogDistance_; }
const ref_ptr<ShaderInput1f>& DistanceFog::fogDensity() const
{ return fogDensity_; }
