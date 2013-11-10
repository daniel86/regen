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

ref_ptr<Shader> ShaderNodeProvider::findShader(State *s)
{
  for(list< ref_ptr<State> >::const_reverse_iterator
      it=s->joined().rbegin(); it!=s->joined().rend(); ++it)
  {
    ref_ptr<Shader> out = findShader((*it).get());
    if(out.get()!=NULL) return out;
  }

  ShaderState *shaderState = dynamic_cast<ShaderState*>(s);
  if(shaderState!=NULL) return shaderState->shader();

  HasShader *hasShader = dynamic_cast<HasShader*>(s);
  if(hasShader!=NULL) return hasShader->shaderState()->shader();

  return ref_ptr<Shader>();
}

ref_ptr<Shader> ShaderNodeProvider::findShader(StateNode *n)
{
  ref_ptr<Shader> out = findShader(n->state().get());
  if(out.get()==NULL && n->hasParent()) {
    return findShader(n->parent());
  }
  else {
    return out;
  }
}

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
