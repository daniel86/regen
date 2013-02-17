/*
 * depth-of-field.cpp
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#include <ogle/meshes/rectangle.h>

#include "depth-of-field.h"

DepthOfField::DepthOfField(
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthTexture)
: State()
{
  focalDistance_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("focalDistance"));
  focalDistance_->setUniformData(10.0f);
  focalDistance_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(focalDistance_));

  focalWidth_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("focalWidth"));
  focalWidth_->setUniformData(2.5f);
  focalWidth_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(focalWidth_));

  blurRange_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurRange"));
  blurRange_->setUniformData(5.0f);
  blurRange_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(blurRange_));

  ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(input));
  texState->set_name("inputTexture");
  joinStates(ref_ptr<State>::cast(texState));

  texState = ref_ptr<TextureState>::manage(new TextureState(blurInput));
  texState->set_name("blurTexture");
  joinStates(ref_ptr<State>::cast(texState));

  texState = ref_ptr<TextureState>::manage(new TextureState(depthTexture));
  texState->set_name("depthTexture");
  joinStates(ref_ptr<State>::cast(texState));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_) );

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

void DepthOfField::createShader(ShaderConfig &cfg)
{
  shader_->createShader(cfg, "depth_of_field");
}

const ref_ptr<ShaderInput1f>& DepthOfField::focalDistance() const
{
  return focalDistance_;
}
const ref_ptr<ShaderInput1f>& DepthOfField::focalWidth() const
{
  return focalWidth_;
}
const ref_ptr<ShaderInput1f>& DepthOfField::blurRange() const
{
  return blurRange_;
}
