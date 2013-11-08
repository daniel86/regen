/*
 * shader.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "shader.h"
using namespace regen::scene;
using namespace regen;

#include <regen/states/state-configurer.h>
#include <regen/states/shader-state.h>
#include <regen/scene/resource-manager.h>

#define REGEN_SHADER_NODE_CATEGORY "shader"

ShaderNodeProvider::ShaderNodeProvider()
: NodeProcessor(REGEN_SHADER_NODE_CATEGORY)
{}

void ShaderNodeProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<StateNode> &parent)
{
  if(!input.hasAttribute("key") && !input.hasAttribute("code")) {
    REGEN_WARN("Ignoring " << input.getDescription() << " without shader input.");
    return;
  }
  ref_ptr<ShaderState> shaderState = ref_ptr<ShaderState>::alloc();
  parent->state()->joinStates(shaderState);

  const string shaderKey = input.hasAttribute("key") ?
      input.getValue("key") : input.getValue("code");
  StateConfigurer stateConfigurer;
  stateConfigurer.addNode(parent.get());
  shaderState->createShader(stateConfigurer.cfg(), shaderKey);
}
