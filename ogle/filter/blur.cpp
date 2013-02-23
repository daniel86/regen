/*
 * blur.cpp
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#include "blur.h"

BlurSeparableFilter::BlurSeparableFilter(Direction dir, GLfloat scale)
: Filter("blurSeparable", scale), dir_(dir)
{
  sigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  sigma_->setUniformData(2.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sigma_));

  numPixels_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("numBlurPixels"));
  numPixels_->setUniformData(4.0f);
  joinShaderInput(ref_ptr<ShaderInput>::cast(numPixels_));

  if(dir == HORITONTAL) {
    shaderDefine("BLUR_HORIZONTAL", "TRUE");
  } else {
    shaderDefine("BLUR_HORIZONTAL", "FALSE");
  }
}

const ref_ptr<ShaderInput1f>& BlurSeparableFilter::sigma() const
{
  return sigma_;
}
const ref_ptr<ShaderInput1f>& BlurSeparableFilter::numPixels() const
{
  return numPixels_;
}
