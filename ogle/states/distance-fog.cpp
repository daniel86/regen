/*
 * distance-fog.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include "distance-fog.h"

#include <ogle/meshes/rectangle.h>
#include <ogle/render-tree/shader-configurer.h>

DistanceFog::DistanceFog()
: State()
{
  fogColor_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("fogColor"));
  fogColor_->setUniformData(Vec3f(1.0));
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogColor_));

  fogStart_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogStart"));
  fogStart_->setUniformData(0.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogStart_));

  fogEnd_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogEnd"));
  fogEnd_->setUniformData(100.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogEnd_));

  fogDensity_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogDensity"));
  fogDensity_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogDensity_));

  fogShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(fogShader_));

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

void DistanceFog::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  fogShader_->createShader(cfg, "fog.distance");
}

void DistanceFog::set_gDepthTexture(const ref_ptr<Texture> &t)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
  }
  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  gDepthTexture_->set_name("gDepthTexture");
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));
}
void DistanceFog::set_tBuffer(
    const ref_ptr<Texture> &color,
    const ref_ptr<Texture> &depth)
{
  shaderDefine("USE_TBUFFER", "TRUE");
  if(tDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  tDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depth));
  tDepthTexture_->set_name("tDepthTexture");
  joinStatesFront(ref_ptr<State>::cast(tDepthTexture_));

  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(color));
  tColorTexture_->set_name("tColorTexture");
  joinStatesFront(ref_ptr<State>::cast(tColorTexture_));
}
void DistanceFog::set_skyColor(const ref_ptr<TextureCube> &t)
{
  shaderDefine("USE_SKY_COLOR", "TRUE");
  if(skyColorTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(skyColorTexture_));
  }
  skyColorTexture_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(t)));
  skyColorTexture_->set_name("skyColorTexture");
  joinStatesFront(ref_ptr<State>::cast(skyColorTexture_));
}

const ref_ptr<ShaderInput3f>& DistanceFog::fogColor() const
{
  return fogColor_;
}
const ref_ptr<ShaderInput1f>& DistanceFog::fogStart() const
{
  return fogStart_;
}
const ref_ptr<ShaderInput1f>& DistanceFog::fogEnd() const
{
  return fogEnd_;
}
const ref_ptr<ShaderInput1f>& DistanceFog::fogDensity() const
{
  return fogDensity_;
}
