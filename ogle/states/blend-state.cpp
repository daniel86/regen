/*
 * blend-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "blend-state.h"

BlendState::BlendState(
    GLenum sfactor,
    GLenum dfactor)
: State(),
  sfactor_(sfactor),
  dfactor_(dfactor)
{
}

string BlendState::name()
{
  return "BlendState";
}

void BlendState::enable(RenderState *state)
{
  glEnable(GL_BLEND);
  glBlendFunc(sfactor_, dfactor_);
  State::enable(state);
}

void BlendState::disable(RenderState *state)
{
  State::disable(state);
  glDisable (GL_BLEND);
}
