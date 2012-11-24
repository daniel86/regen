/*
 * cull-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "cull-state.h"

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
