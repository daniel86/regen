/*
 * depth.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "depth.h"
using namespace regen::scene;
using namespace regen;

#define REGEN_DEPTH_STATE_CATEGORY "depth"

DepthStateProvider::DepthStateProvider()
: StateProcessor(REGEN_DEPTH_STATE_CATEGORY)
{}

void DepthStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();

  depth->set_useDepthTest(input.getValue<bool>("test",true));
  depth->set_useDepthWrite(input.getValue<bool>("write",true));

  if(input.hasAttribute("range")) {
    Vec2f range = input.getValue<Vec2f>("range",Vec2f(0.0f));
    depth->set_depthRange(range.x, range.y);
  }

  if(input.hasAttribute("function")) {
    depth->set_depthFunc(glenum::compareFunction(input.getValue("function")));
  }

  state->joinStates(depth);
}
