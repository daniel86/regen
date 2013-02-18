/*
 * ambient-occlusion.cpp
 *
 *  Created on: 08.02.2013
 *      Author: daniel
 */

#include "ambient-occlusion.h"

#include <ogle/textures/texture-loader.h>
#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/rectangle.h>

SSAO::SSAO()
: State()
{
  aoSampleRad_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoSampleRad"));
  aoSampleRad_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoSampleRad_));

  aoBias_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoBias"));
  aoBias_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoBias_));

  aoConstAttenuation_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoConstAttenuation"));
  aoConstAttenuation_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoConstAttenuation_));

  aoLinearAttenuation_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("aoLinearAttenuation"));
  aoLinearAttenuation_->setUniformData(1.0);
  joinShaderInput(ref_ptr<ShaderInput>::cast(aoLinearAttenuation_));

  ref_ptr<Texture> randomNormals =
      TextureLoader::load("res/textures/random_normals.png");
  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(
      new TextureState(randomNormals));
  texState->set_name("randomNorTexture");
  joinStates(ref_ptr<State>::cast(texState));

  aoShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(aoShader_));

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

void SSAO::createShader(ShaderConfig &cfg)
{
  aoShader_->createShader(cfg, "ssao");
}

void SSAO::set_norWorldTexture(const ref_ptr<Texture> &t)
{
  if(norWorldTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(norWorldTexture_));
  }
  norWorldTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  norWorldTexture_->set_name("gNorWorldTexture");
  joinStatesFront(ref_ptr<State>::cast(norWorldTexture_));
}
void SSAO::set_depthTexture(const ref_ptr<Texture> &t)
{
  if(depthTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(depthTexture_));
  }
  depthTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  depthTexture_->set_name("gDepthTexture");
  joinStatesFront(ref_ptr<State>::cast(depthTexture_));
}

const ref_ptr<ShaderInput1f>& SSAO::aoSampleRad() const
{
  return aoSampleRad_;
}
const ref_ptr<ShaderInput1f>& SSAO::aoBias() const
{
  return aoBias_;
}
const ref_ptr<ShaderInput1f>& SSAO::aoConstAttenuation() const
{
  return aoConstAttenuation_;
}
const ref_ptr<ShaderInput1f>& SSAO::aoLinearAttenuation() const
{
  return aoLinearAttenuation_;
}
