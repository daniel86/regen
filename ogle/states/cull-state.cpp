/*
 * cull-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "cull-state.h"

CullEnableState::CullEnableState()
: State()
{
}
void CullEnableState::enable(RenderState *state)
{
  glGetBooleanv(GL_CULL_FACE, &culled_);
  if(!culled_) {
    glEnable(GL_CULL_FACE);
  }
}
void CullEnableState::disable(RenderState *state)
{
  if(!culled_) {
    glDisable(GL_CULL_FACE);
  }
}

CullDisableState::CullDisableState()
: State()
{
}
void CullDisableState::enable(RenderState *state)
{
  glDisable(GL_CULL_FACE);
}
void CullDisableState::disable(RenderState *state)
{
  glEnable(GL_CULL_FACE);
}

CullFrontFaceState::CullFrontFaceState()
: State()
{
}
void CullFrontFaceState::enable(RenderState *state)
{
  glCullFace(GL_FRONT);
}
void CullFrontFaceState::disable(RenderState *state)
{
  glCullFace(GL_BACK);
}
