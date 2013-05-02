/*
 * volumetric-fog.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include <regen/states/state-configurer.h>
#include <regen/meshes/box.h>
#include <regen/states/atomic-states.h>

#include "volumetric-fog.h"
using namespace regen;

VolumetricFog::VolumetricFog() : State()
{
  spotFog_ = ref_ptr<LightPass>::manage(new LightPass(Light::SPOT, "fog.volumetric.spot"));
  pointFog_ = ref_ptr<LightPass>::manage(new LightPass(Light::POINT, "fog.volumetric.point"));

  shadowSampleStep_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowSampleStep"));
  shadowSampleStep_->setUniformData(0.025);
  joinShaderInput(shadowSampleStep_);

  shadowSampleThreshold_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowSampleThreshold"));
  shadowSampleThreshold_->setUniformData(0.075);
  joinShaderInput(shadowSampleThreshold_);

  fogDistance_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("fogDistance"));
  fogDistance_->setUniformData(Vec2f(0.0,100.0));
  joinShaderInput(fogDistance_);

  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));

  fogSequence_ = ref_ptr<StateSequence>::manage(new StateSequence);
  joinStates(fogSequence_);
}

const ref_ptr<ShaderInput2f>& VolumetricFog::fogDistance() const
{ return fogDistance_; }
const ref_ptr<ShaderInput1f>& VolumetricFog::shadowSampleStep() const
{ return shadowSampleStep_; }
const ref_ptr<ShaderInput1f>& VolumetricFog::shadowSampleThreshold() const
{ return shadowSampleThreshold_; }

void VolumetricFog::setShadowFiltering(ShadowMap::FilterMode filtering)
{
  spotFog_->setShadowFiltering(filtering);
  pointFog_->setShadowFiltering(filtering);
}

void VolumetricFog::createShader(State::Config &cfg)
{
  StateConfigurer _cfg(cfg);
  _cfg.addState(this);
  spotFog_->createShader(_cfg.cfg());
  pointFog_->createShader(_cfg.cfg());
}

void VolumetricFog::set_gDepthTexture(const ref_ptr<Texture> &t)
{
  if(gDepthTexture_.get()) {
    disjoinStates(gDepthTexture_);
  }
  gDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(t, "gDepthTexture"));
  joinStatesFront(gDepthTexture_);
}
void VolumetricFog::set_tBuffer(
    const ref_ptr<Texture> &color,
    const ref_ptr<Texture> &depth)
{
  if(tDepthTexture_.get()) {
    disjoinStates(tDepthTexture_);
    disjoinStates(tColorTexture_);
  }
  if(!color.get()) { return; }
  shaderDefine("USE_TBUFFER", "TRUE");
  tDepthTexture_ = ref_ptr<TextureState>::manage(new TextureState(depth, "tDepthTexture"));
  joinStatesFront(tDepthTexture_);

  tColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(color, "tColorTexture"));
  joinStatesFront(tColorTexture_);
}

void VolumetricFog::addSpotLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShadowMap> &sm,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x,
    const ref_ptr<ShaderInput2f> &y)
{
  if(spotFog_->empty()) {
    fogSequence_->joinStates(spotFog_);
  }
  list< ref_ptr<ShaderInput> > inputs;
  inputs.push_back(exposure);
  inputs.push_back(x);
  inputs.push_back(y);
  spotFog_->addLight(l,sm,inputs);
}
void VolumetricFog::addSpotLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x,
    const ref_ptr<ShaderInput2f> &y)
{
  addSpotLight(l, ref_ptr<ShadowMap>(), exposure, x, y);
}

void VolumetricFog::addPointLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShadowMap> &sm,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x)
{
  if(pointFog_->empty()) {
    fogSequence_->joinStates(pointFog_);
  }
  list< ref_ptr<ShaderInput> > inputs;
  inputs.push_back(exposure);
  inputs.push_back(x);
  pointFog_->addLight(l,sm,inputs);
}
void VolumetricFog::addPointLight(
    const ref_ptr<Light> &l,
    const ref_ptr<ShaderInput1f> &exposure,
    const ref_ptr<ShaderInput2f> &x)
{
  addPointLight(l, ref_ptr<ShadowMap>(), exposure, x);
}

void VolumetricFog::removeLight(Light *l)
{
  if(spotFog_->hasLight(l)) {
    spotFog_->removeLight(l);
    if(spotFog_->empty()) {
      fogSequence_->disjoinStates(spotFog_);
    }
  }
  if(pointFog_->hasLight(l)) {
    pointFog_->removeLight(l);
    if(pointFog_->empty()) {
      fogSequence_->disjoinStates(pointFog_);
    }
  }
}

