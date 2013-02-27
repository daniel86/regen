/*
 * polygon-offset-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "polygon-offset-state.h"
#include <ogle/states/render-state.h>

PolygonOffsetState::PolygonOffsetState(GLfloat factor, GLfloat units)
: State(), factor_(factor), units_(units)
{
}
void PolygonOffsetState::enable(RenderState *state)
{
  state->pushToggle(RenderState::POLYGON_OFFSET_FILL, GL_TRUE);
  state->polygonOffset().push(Vec2f(factor_, units_));
}
void PolygonOffsetState::disable(RenderState *state)
{
  state->popToggle(RenderState::POLYGON_OFFSET_FILL);
}
