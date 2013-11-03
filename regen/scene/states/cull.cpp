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
  const string mode = input.getValue<string>("mode", "back");
  state->joinStates(ref_ptr<CullFaceState>::alloc(glenum::cullFace(mode)));
}
