/*
 * tonemap.cpp
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#include <regen/meshes/rectangle.h>

#include "tonemap.h"
using namespace regen;

Tonemap::Tonemap(
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput)
: FullscreenPass("regen.post-passes.tonemap")
{
  ref_ptr<TextureState> texState;
  texState = ref_ptr<TextureState>::alloc(input, "inputTexture");
  joinStatesFront(texState);
  texState = ref_ptr<TextureState>::alloc(blurInput, "blurTexture");
  joinStatesFront(texState);

  blurAmount_ = ref_ptr<ShaderInput1f>::alloc("blurAmount");
  blurAmount_->setUniformData(0.5f);
  blurAmount_->set_isConstant(GL_TRUE);
  joinShaderInput(blurAmount_);

  effectAmount_ = ref_ptr<ShaderInput1f>::alloc("effectAmount");
  effectAmount_->setUniformData(0.2f);
  effectAmount_->set_isConstant(GL_TRUE);
  joinShaderInput(effectAmount_);

  exposure_ = ref_ptr<ShaderInput1f>::alloc("exposure");
  exposure_->setUniformData(16.0f);
  exposure_->set_isConstant(GL_TRUE);
  joinShaderInput(exposure_);

  gamma_ = ref_ptr<ShaderInput1f>::alloc("gamma");
  gamma_->setUniformData(0.5f);
  gamma_->set_isConstant(GL_TRUE);
  joinShaderInput(gamma_);

  radialBlurSamples_ = ref_ptr<ShaderInput1f>::alloc("radialBlurSamples");
  radialBlurSamples_->setUniformData(30.0f);
  radialBlurSamples_->set_isConstant(GL_TRUE);
  joinShaderInput(radialBlurSamples_);

  radialBlurStartScale_ = ref_ptr<ShaderInput1f>::alloc("radialBlurStartScale");
  radialBlurStartScale_->setUniformData(1.0f);
  radialBlurStartScale_->set_isConstant(GL_TRUE);
  joinShaderInput(radialBlurStartScale_);

  radialBlurScaleMul_ = ref_ptr<ShaderInput1f>::alloc("radialBlurScaleMul");
  radialBlurScaleMul_->setUniformData(0.9f);
  radialBlurScaleMul_->set_isConstant(GL_TRUE);
  joinShaderInput(radialBlurScaleMul_);

  vignetteInner_ = ref_ptr<ShaderInput1f>::alloc("vignetteInner");
  vignetteInner_->setUniformData(0.7f);
  vignetteInner_->set_isConstant(GL_TRUE);
  joinShaderInput(vignetteInner_);

  vignetteOuter_ = ref_ptr<ShaderInput1f>::alloc("vignetteOuter");
  vignetteOuter_->setUniformData(1.5f);
  vignetteOuter_->set_isConstant(GL_TRUE);
  joinShaderInput(vignetteOuter_);
}

const ref_ptr<ShaderInput1f>& Tonemap::blurAmount() const
{ return blurAmount_; }
const ref_ptr<ShaderInput1f>& Tonemap::effectAmount() const
{ return effectAmount_; }
const ref_ptr<ShaderInput1f>& Tonemap::exposure() const
{ return exposure_; }
const ref_ptr<ShaderInput1f>& Tonemap::gamma() const
{ return gamma_; }
const ref_ptr<ShaderInput1f>& Tonemap::radialBlurSamples() const
{ return radialBlurSamples_; }
const ref_ptr<ShaderInput1f>& Tonemap::radialBlurStartScale() const
{ return radialBlurStartScale_; }
const ref_ptr<ShaderInput1f>& Tonemap::radialBlurScaleMul() const
{ return radialBlurScaleMul_; }
const ref_ptr<ShaderInput1f>& Tonemap::vignetteInner() const
{ return vignetteInner_; }
const ref_ptr<ShaderInput1f>& Tonemap::vignetteOuter() const
{ return vignetteOuter_; }
