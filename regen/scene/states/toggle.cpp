/*
 * toggle.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "toggle.h"
using namespace regen::scene;
using namespace regen;

#define REGEN_TOGGLE_STATE_CATEGORY "toggle"

ToggleStateProvider::ToggleStateProvider()
: StateProcessor(REGEN_TOGGLE_STATE_CATEGORY)
{}

void ToggleStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  if(!input.hasAttribute("key")) {
    REGEN_WARN("Ignoring " << input.getDescription() << " without key attribute.");
    return;
  }
  state->joinStates(ref_ptr<ToggleState>::alloc(
      input.getValue<RenderState::Toggle>("key",RenderState::CULL_FACE),
      input.getValue<bool>("value",true)));
}


