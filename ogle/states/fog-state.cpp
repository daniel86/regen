/*
 * fog.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "fog-state.h"

Fog::Fog(GLfloat far)
: State()
{
  fogColor_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("fogColor"));
  fogColor_->setUniformData(Vec4f(0.1, 0.55, 1.0, 1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(fogColor_) );

  fogEnd_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogEnd"));
  fogEnd_->setUniformData(far);
  joinShaderInput( ref_ptr<ShaderInput>::cast(fogEnd_) );

  fogScale_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogScale"));
  fogScale_->setUniformData(1.0 / far*1.35);
  joinShaderInput( ref_ptr<ShaderInput>::cast(fogScale_) );
}

string Fog::name()
{
  return "Fog";
}

ref_ptr<ShaderInput4f>& Fog::fogColor()
{
  return fogColor_;
}
ref_ptr<ShaderInput1f>& Fog::fogEnd()
{
  return fogEnd_;
}
ref_ptr<ShaderInput1f>& Fog::fogScale()
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

void Fog::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setUseFog();
}
