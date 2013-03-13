/*
 * volumetric-fog.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/box.h>
#include <ogle/states/atomic-states.h>

#include "volumetric-fog.h"
using namespace ogle;

VolumetricFog::VolumetricFog() : State()
{
  spotFog_ = ref_ptr<LightPass>::manage(new LightPass(LightPass::SPOT, "fog.volumetric.spot"));
  pointFog_ = ref_ptr<LightPass>::manage(new LightPass(LightPass::POINT, "fog.volumetric.point"));

  fogDistance_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogDistance"));
  fogDistance_->setUniformData(Vec2f(0.0,100.0));
  joinShaderInput(ref_ptr<ShaderInput>::cast(fogDistance_));

  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));

  fogSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(ref_ptr<State>::cast(fogSequence_));
}

const ref_ptr<ShaderInput2f>& VolumetricFog::fogDistance() const
{
  return fogDistance_;
}

void VolumetricFog::createShader(ShaderState::Config &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  spotFog_->createShader(_cfg.cfg());
  pointFog_->createShader(_cfg.cfg());
}

void VolumetricFog::set_gDepthTexture(const ref_ptr<Texture> &t)
{
  if(gDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(gDepthTexture_));
  }
  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(t, "gDepthTexture"));
  joinStatesFront(ref_ptr<State>::cast(gDepthTexture_));
}
void VolumetricFog::set_tBuffer(
    const ref_ptr<Texture> &color,
    const ref_ptr<Texture> &depth)
{
  if(tDepthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(tDepthTexture_));
    disjoinStates(ref_ptr<State>::cast(tColorTexture_));
  }
  if(!color.get()) { return; }
  shaderDefine("USE_TBUFFER", "TRUE");
  tDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depth, "tDepthTexture"));
  joinStatesFront(ref_ptr<State>::cast(tDepthTexture_));

  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(color, "tColorTexture"));
  joinStatesFront(ref_ptr<State>::cast(tColorTexture_));
}

void VolumetricFog::addLight(
    const ref_ptr<SpotLight> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x,
    const ref_ptr<ShaderInput2f> &y)
{
  if(spotFog_->empty()) {
    fogSequence_->joinStates(ref_ptr<State>::cast(spotFog_));
  }
  ref_ptr<ShadowMap> sm; // no shadow map used
  list< ref_ptr<ShaderInput> > inputs;
  inputs.push_back(ref_ptr<ShaderInput>::cast(exposure));
  inputs.push_back(ref_ptr<ShaderInput>::cast(x));
  inputs.push_back(ref_ptr<ShaderInput>::cast(y));
  spotFog_->addLight(ref_ptr<Light>::cast(l),sm,inputs);
}
void VolumetricFog::addLight(
    const ref_ptr<PointLight> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x)
{
  if(pointFog_->empty()) {
    fogSequence_->joinStates(ref_ptr<State>::cast(pointFog_));
  }
  ref_ptr<ShadowMap> sm; // no shadow map used
  list< ref_ptr<ShaderInput> > inputs;
  inputs.push_back(ref_ptr<ShaderInput>::cast(exposure));
  inputs.push_back(ref_ptr<ShaderInput>::cast(x));
  pointFog_->addLight(ref_ptr<Light>::cast(l),sm,inputs);
}
void VolumetricFog::removeLight(SpotLight *l)
{
  spotFog_->removeLight(l);
  if(spotFog_->empty()) {
    fogSequence_->disjoinStates(ref_ptr<State>::cast(spotFog_));
  }
}
void VolumetricFog::removeLight(PointLight *l)
{
  pointFog_->removeLight(l);
  if(pointFog_->empty()) {
    fogSequence_->disjoinStates(ref_ptr<State>::cast(pointFog_));
  }
}

