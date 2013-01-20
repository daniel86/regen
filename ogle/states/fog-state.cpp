/*
 * fog.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "fog-state.h"

Fog::Fog(GLfloat far)
: ShaderInputState()
{
  fogColor_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("fogColor"));
  fogColor_->setUniformData(Vec4f(0.1, 0.55, 1.0, 1.0));
  setInput( ref_ptr<ShaderInput>::cast(fogColor_) );

  fogEnd_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogEnd"));
  fogEnd_->setUniformData(far);
  setInput( ref_ptr<ShaderInput>::cast(fogEnd_) );

  fogScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogScale"));
  fogScale_->setUniformData(1.0 / far*1.35);
  setInput( ref_ptr<ShaderInput>::cast(fogScale_) );
  // prepend '#define HAS_FOG' to loaded shaders
  shaderDefine("HAS_FOG", "TRUE");
}

const ref_ptr<ShaderInput4f>& Fog::fogColor() const
{
  return fogColor_;
}
const ref_ptr<ShaderInput1f>& Fog::fogEnd() const
{
  return fogEnd_;
}
const ref_ptr<ShaderInput1f>& Fog::fogScale() const
{
  return fogScale_;
}

void Fog::set_fogColor(const Vec4f &color)
{
  fogColor_->setUniformData(color);
}
void Fog::set_fogEnd(GLfloat end)
{
  fogEnd_->setUniformData(end);
}
void Fog::set_fogScale(GLfloat scale)
{
  fogScale_->setUniformData(scale);
}
