/*
 * polygon-offset-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "polygon-offset-state.h"
#include <ogle/states/toggle-state.h>

PolygonOffsetState::PolygonOffsetState(GLfloat factor, GLfloat units)
: State(), factor_(factor), units_(units)
{
  joinStates(ref_ptr<State>::manage(
      new ToggleState(RenderState::POLYGON_OFFSET_FILL, GL_TRUE)));
}
void PolygonOffsetState::enable(RenderState *state)
{
  state->polygonOffset().push(Vec2f(factor_, units_));
}
