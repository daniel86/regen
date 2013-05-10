/*
 * depth-of-field.cpp
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#include <regen/meshes/rectangle.h>

#include "depth-of-field.h"
using namespace regen;

DepthOfField::DepthOfField(
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthTexture)
: FullscreenPass("depth_of_field")
{
  ref_ptr<TextureState> texState;
  texState = ref_ptr<TextureState>::alloc(input,"inputTexture");
  joinStatesFront(texState);
  texState = ref_ptr<TextureState>::alloc(blurInput,"blurTexture");
  joinStatesFront(texState);
  texState = ref_ptr<TextureState>::alloc(depthTexture, "depthTexture");
  joinStatesFront(texState);

  focalDistance_ = ref_ptr<ShaderInput1f>::alloc("focalDistance");
  focalDistance_->setUniformData(0.0f);
  focalDistance_->set_isConstant(GL_TRUE);
  joinShaderInput(focalDistance_);

  focalWidth_ = ref_ptr<ShaderInput2f>::alloc("focalWidth");
  focalWidth_->setUniformData(Vec2f(0.7f,1.0f));
  focalWidth_->set_isConstant(GL_TRUE);
  joinShaderInput(focalWidth_);
}

const ref_ptr<ShaderInput1f>& DepthOfField::focalDistance() const
{ return focalDistance_; }
const ref_ptr<ShaderInput2f>& DepthOfField::focalWidth() const
{ return focalWidth_; }
