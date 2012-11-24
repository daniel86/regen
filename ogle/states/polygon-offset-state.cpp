/*
 * polygon-offset-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "polygon-offset-state.h"

PolygonOffsetState::PolygonOffsetState(GLfloat factor, GLfloat units)
: State(), factor_(factor), units_(units)
{
}
void PolygonOffsetState::enable(RenderState *state)
{
  glEnable(GL_POLYGON_OFFSET_FILL);
  glPolygonOffset(factor_, units_);
}
void PolygonOffsetState::disable(RenderState *state)
{
  glDisable(GL_POLYGON_OFFSET_FILL);
}
