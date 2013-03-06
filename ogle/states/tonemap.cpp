/*
 * tonemap.cpp
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#include <ogle/meshes/rectangle.h>

#include "tonemap.h"
using namespace ogle;

Tonemap::Tonemap(
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput)
: State()
{
  blurAmount_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurAmount"));
  blurAmount_->setUniformData(0.5f);
  blurAmount_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurAmount_));

  effectAmount_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("effectAmount"));
  effectAmount_->setUniformData(0.2f);
  effectAmount_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(effectAmount_));

  exposure_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("exposure"));
  exposure_->setUniformData(16.0f);
  exposure_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(exposure_));

  gamma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("gamma"));
  gamma_->setUniformData(0.5f);
  gamma_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(gamma_));

  radialBlurSamples_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("radialBlurSamples"));
  radialBlurSamples_->setUniformData(30.0f);
  radialBlurSamples_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(radialBlurSamples_));

  radialBlurStartScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("radialBlurStartScale"));
  radialBlurStartScale_->setUniformData(1.0f);
  radialBlurStartScale_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(radialBlurStartScale_));

  radialBlurScaleMul_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("radialBlurScaleMul"));
  radialBlurScaleMul_->setUniformData(0.9f);
  radialBlurScaleMul_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(radialBlurScaleMul_));

  vignetteInner_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("vignetteInner"));
  vignetteInner_->setUniformData(0.7f);
  vignetteInner_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(vignetteInner_));

  vignetteOuter_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("vignetteOuter"));
  vignetteOuter_->setUniformData(1.5f);
  vignetteOuter_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(vignetteOuter_));

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(input, "inputTexture"));
  joinStates(ref_ptr<State>::cast(texState));

  texState = ref_ptr<TextureState>::manage(new TextureState(blurInput, "blurTexture"));
  joinStates(ref_ptr<State>::cast(texState));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

void Tonemap::createShader(ShaderConfig &cfg)
{
  shader_->createShader(cfg, "tonemap");
}

const ref_ptr<ShaderInput1f>& Tonemap::blurAmount() const
{
  return blurAmount_;
}
const ref_ptr<ShaderInput1f>& Tonemap::effectAmount() const
{
  return effectAmount_;
}
const ref_ptr<ShaderInput1f>& Tonemap::exposure() const
{
  return exposure_;
}
const ref_ptr<ShaderInput1f>& Tonemap::gamma() const
{
  return gamma_;
}
const ref_ptr<ShaderInput1f>& Tonemap::radialBlurSamples() const
{
  return radialBlurSamples_;
}
const ref_ptr<ShaderInput1f>& Tonemap::radialBlurStartScale() const
{
  return radialBlurStartScale_;
}
const ref_ptr<ShaderInput1f>& Tonemap::radialBlurScaleMul() const
{
  return radialBlurScaleMul_;
}
const ref_ptr<ShaderInput1f>& Tonemap::vignetteInner() const
{
  return vignetteInner_;
}
const ref_ptr<ShaderInput1f>& Tonemap::vignetteOuter() const
{
  return vignetteOuter_;
}
