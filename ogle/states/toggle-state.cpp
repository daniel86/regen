/*
 * toggle-state.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "toggle-state.h"
#include <ogle/states/render-state.h>

ToggleState::ToggleState(RenderState::Toggle key, GLboolean toggle)
: State(), key_(key), toggle_(toggle)
{
}

RenderState::Toggle ToggleState::key() const
{
  return key_;
}
GLboolean ToggleState::toggle() const
{
  return toggle_;
}

void ToggleState::enable(RenderState *state)
{
  state->pushToggle(key_, toggle_);
}
void ToggleState::disable(RenderState *state)
{
  state->popToggle(key_);
}
