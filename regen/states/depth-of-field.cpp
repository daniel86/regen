/*
 * depth-of-field.cpp
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#include <ogle/meshes/rectangle.h>

#include "depth-of-field.h"
using namespace ogle;

DepthOfField::DepthOfField(
    const ref_ptr<Texture> &input,
    const ref_ptr<Texture> &blurInput,
    const ref_ptr<Texture> &depthTexture)
: FullscreenPass("depth_of_field")
{
  ref_ptr<TextureState> texState;
  texState = ref_ptr<TextureState>::manage(new TextureState(input,"inputTexture"));
  joinStatesFront(ref_ptr<State>::cast(texState));
  texState = ref_ptr<TextureState>::manage(new TextureState(blurInput,"blurTexture"));
  joinStatesFront(ref_ptr<State>::cast(texState));
  texState = ref_ptr<TextureState>::manage(new TextureState(depthTexture, "depthTexture"));
  joinStatesFront(ref_ptr<State>::cast(texState));

  focalDistance_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("focalDistance"));
  focalDistance_->setUniformData(0.0f);
  focalDistance_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(focalDistance_));

  focalWidth_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("focalWidth"));
  focalWidth_->setUniformData(Vec2f(0.7f,1.0f));
  focalWidth_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(focalWidth_));
}

const ref_ptr<ShaderInput1f>& DepthOfField::focalDistance() const
{ return focalDistance_; }
const ref_ptr<ShaderInput2f>& DepthOfField::focalWidth() const
{ return focalWidth_; }
