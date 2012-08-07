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
  fogColorUniform_ = ref_ptr<UniformVec4>::manage(
      new UniformVec4("fogColor", 1, Vec4f(0.1, 0.55, 1.0, 1.0)));
  joinStates( fogColorUniform_ );

  fogEndUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("fogEnd", 1, far));
  joinStates( fogEndUniform_ );

  fogScaleUniform_ = ref_ptr<UniformFloat>::manage(
      new UniformFloat("fogScale", 1, 1.0 / far*1.35));
  joinStates( fogScaleUniform_ );
}

void Fog::set_fogColor(const Vec4f &color)
{
  fogColorUniform_->set_value(color);
}
void Fog::set_fogEnd(float end)
{
  fogEndUniform_->set_value(end);
}
void Fog::set_fogScale(float scale)
{
  fogScaleUniform_->set_value(scale);
}

void Fog::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setUseFog(true);
}
