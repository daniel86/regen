/*
 * define.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "define.h"
using namespace regen::scene;
using namespace regen;

#define REGEN_DEFINE_STATE_CATEGORY "define"

DefineStateProvider::DefineStateProvider()
: StateProcessor(REGEN_DEFINE_STATE_CATEGORY)
{}

void DefineStateProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<State> &state)
{
  if(!input.hasAttribute("key")) {
    REGEN_WARN("Ignoring " << input.getDescription() << " without key attribute.");
    return;
  }
  if(!input.hasAttribute("value")) {
    REGEN_WARN("Ignoring " << input.getDescription() << " without value attribute.");
    return;
  }
  ref_ptr<State> s = state;
  while(!s->joined().empty()) {
    s = *s->joined().rbegin();
  }
  s->shaderDefine(
      input.getValue("key"),
      input.getValue("value"));
}
