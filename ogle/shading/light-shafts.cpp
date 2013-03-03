/*
 * light-shafts.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/rectangle.h>
#include "light-shafts.h"

SkyLightShaft::SkyLightShaft(
    const ref_ptr<DirectionalLight> &sun,
    const ref_ptr<Texture> &colorTexture,
    const ref_ptr<Texture> &depthTexture)
: State(), sun_(sun), lightDirLoc_(-1)
{
  scatteringDensity_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("sunScatteringDensity"));
  scatteringDensity_->setUniformData(1.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(scatteringDensity_));

  scatteringSamples_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("sunScatteringSamples"));
  scatteringSamples_->setUniformData(40.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(scatteringSamples_));

  scatteringExposure_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("sunScatteringExposure"));
  scatteringExposure_->setUniformData(0.6f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(scatteringExposure_));

  scatteringDecay_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("sunScatteringDecay"));
  scatteringDecay_->setUniformData(0.9f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(scatteringDecay_));

  scatteringWeight_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("sunScatteringWeight"));
  scatteringWeight_->setUniformData(0.1f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(scatteringWeight_));

  joinStates(ref_ptr<State>::manage(
      new TextureState(colorTexture, "colorTexture")));
  joinStates(ref_ptr<State>::manage(
      new TextureState(depthTexture, "depthTexture")));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_) );

  mesh_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());
}

void SkyLightShaft::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(mesh_.get());
  shader_->createShader(_cfg.cfg(), "light_shafts.sky");
  lightDirLoc_ = shader_->shader()->uniformLocation("lightDirection");
}

void SkyLightShaft::enable(RenderState *rs)
{
  State::enable(rs);
  sun_->direction()->enableUniform(lightDirLoc_);
  mesh_->draw(1);
}
const ref_ptr<ShaderInput1f>& SkyLightShaft::scatteringDensity() const
{
  return scatteringDensity_;
}
const ref_ptr<ShaderInput1f>& SkyLightShaft::scatteringSamples() const
{
  return scatteringSamples_;
}
const ref_ptr<ShaderInput1f>& SkyLightShaft::scatteringExposure() const
{
  return scatteringExposure_;
}
const ref_ptr<ShaderInput1f>& SkyLightShaft::scatteringDecay() const
{
  return scatteringDecay_;
}
const ref_ptr<ShaderInput1f>& SkyLightShaft::scatteringWeight() const
{
  return scatteringWeight_;
}
