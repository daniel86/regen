/*
 * cull-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "cull-state.h"

// TODO: remove in favor of more generic class....
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
