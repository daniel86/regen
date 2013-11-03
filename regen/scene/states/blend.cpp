/*
 * blend.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "blend.h"
using namespace regen::scene;
using namespace regen;

#define REGEN_BLEND_STATE_CATEGORY "blend"

BlendStateProvider::BlendStateProvider()
: StateProcessor(REGEN_BLEND_STATE_CATEGORY)
{}

void BlendStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  ref_ptr<BlendState> blend = ref_ptr<BlendState>::alloc(
      input.getValue<BlendMode>("mode", BLEND_MODE_SRC));
  if(input.hasAttribute("color")) {
    blend->setBlendColor(input.getValue<Vec4f>("color",Vec4f(0.0f)));
  }
  if(input.hasAttribute("equation")) {
    blend->setBlendEquation(glenum::blendFunction(
        input.getValue<string>("equation","ADD")));
  }
  state->joinStates(blend);
}
