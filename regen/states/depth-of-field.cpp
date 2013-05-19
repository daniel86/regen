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
: FullscreenPass("regen.post-passes.dof")
{
  joinStatesFront( ref_ptr<TextureState>::alloc(input,"inputTexture") );
  joinStatesFront( ref_ptr<TextureState>::alloc(blurInput,"blurTexture") );
  joinStatesFront( ref_ptr<TextureState>::alloc(depthTexture, "depthTexture") );

  focalDistance_ = ref_ptr<ShaderInput1f>::alloc("focalDistance");
  focalDistance_->setUniformData(0.0f);
  joinShaderInput(focalDistance_);

  focalWidth_ = ref_ptr<ShaderInput2f>::alloc("focalWidth");
  focalWidth_->setUniformData(Vec2f(0.7f,1.0f));
  joinShaderInput(focalWidth_);
}

const ref_ptr<ShaderInput1f>& DepthOfField::focalDistance() const
{ return focalDistance_; }
const ref_ptr<ShaderInput2f>& DepthOfField::focalWidth() const
{ return focalWidth_; }
