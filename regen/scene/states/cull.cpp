/*
 * cull.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "cull.h"
using namespace regen::scene;
using namespace regen;

#include <regen/gl-types/gl-enum.h>

#define REGEN_CULL_STATE_CATEGORY "cull"

CullStateProvider::CullStateProvider()
: StateProcessor(REGEN_CULL_STATE_CATEGORY)
{}

void CullStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  if(input.hasAttribute("mode")) {
    state->joinStates(ref_ptr<CullFaceState>::alloc(
        glenum::cullFace(input.getValue("mode"))));
  }
  if(input.hasAttribute("winding-order")) {
    state->joinStates(ref_ptr<FrontFaceState>::alloc(
        glenum::frontFace(input.getValue("winding-order"))));
  }
}
