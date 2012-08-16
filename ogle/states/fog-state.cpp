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
  fogColorUniform_ = ref_ptr<ShaderInput4f>::manage(new ShaderInput4f("fogColor"));
  fogColorUniform_->setUniformData(Vec4f(0.1, 0.55, 1.0, 1.0));
  joinShaderInput( ref_ptr<ShaderInput>::cast(fogColorUniform_) );

  fogEndUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogEnd"));
  fogEndUniform_->setUniformData(far);
  joinShaderInput( ref_ptr<ShaderInput>::cast(fogEndUniform_) );

  fogScaleUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("fogScale"));
  fogScaleUniform_->setUniformData(1.0 / far*1.35);
  joinShaderInput( ref_ptr<ShaderInput>::cast(fogScaleUniform_) );
}

string Fog::name()
{
  return "Fog";
}

void Fog::set_fogColor(const Vec4f &color)
{
  fogColorUniform_->setUniformData(color);
}
void Fog::set_fogEnd(float end)
{
  fogEndUniform_->setUniformData(end);
}
void Fog::set_fogScale(float scale)
{
  fogScaleUniform_->setUniformData(scale);
}

void Fog::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setUseFog();
}
