/*
 * uniform-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "uniform-state.h"

UniformState::UniformState(const ref_ptr<Uniform> &uniform)
: State(),
  uniform_(uniform)
{
}

void UniformState::enable(RenderState *state)
{
  if(uniform_->numInstances()==1) {
    state->pushUniform(uniform_.get());
  } else {
    state->pushAttribute(uniform_->attribute().get());
  }
  State::enable(state);
}

void UniformState::disable(RenderState *state)
{
  State::disable(state);
  if(uniform_->numInstances()==1) {
    state->popUniform();
  } else {
    state->popAttribute(uniform_->attribute()->name());
  }
}

ref_ptr<Uniform>& UniformState::uniform()
{
  return uniform_;
}

void UniformState::configureShader(ShaderConfiguration *cfg)
{
  if(uniform_->numInstances()>1) {
    cfg->setAttribute(uniform_->attribute().get());
  }
}
