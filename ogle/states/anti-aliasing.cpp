/*
 * anti-aliasing.cpp
 *
 *  Created on: 16.02.2013
 *      Author: daniel
 */

#include <ogle/meshes/rectangle.h>

#include "anti-aliasing.h"

AntiAliasing::AntiAliasing(const ref_ptr<Texture> &input)
: State(), input_(input)
{
  spanMax_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("spanMax"));
  spanMax_->setUniformData(8.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(spanMax_));

  reduceMul_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("reduceMul"));
  reduceMul_->setUniformData(1.0f/8.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(reduceMul_));

  reduceMin_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("reduceMin"));
  reduceMin_->setUniformData(1.0f/128.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(reduceMin_));

  luma_ = ref_ptr<ShaderInput3f>::manage(new ShaderInput3f("luma"));
  luma_->setUniformData(Vec3f(0.299, 0.587, 0.114));
  joinShaderInput(ref_ptr<ShaderInput>::cast(luma_));

  ref_ptr<TextureState> texState =
      ref_ptr<TextureState>::manage(new TextureState(input, "inputTexture"));
  joinStates(ref_ptr<State>::cast(texState));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_) );

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

void AntiAliasing::createShader(ShaderConfig &cfg)
{
  shader_->createShader(cfg, "fxaa");
}

const ref_ptr<ShaderInput1f>& AntiAliasing::spanMax() const
{
  return spanMax_;
}
const ref_ptr<ShaderInput1f>& AntiAliasing::reduceMul() const
{
  return reduceMul_;
}
const ref_ptr<ShaderInput1f>& AntiAliasing::reduceMin() const
{
  return reduceMin_;
}
const ref_ptr<ShaderInput3f>& AntiAliasing::luma() const
{
  return luma_;
}
