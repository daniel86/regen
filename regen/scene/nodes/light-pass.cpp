/*
 * light-pass.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "light-pass.h"
using namespace regen::scene;
using namespace regen;

#include <regen/states/state-configurer.h>
#include <regen/scene/states/input.h>
#include <regen/scene/resource-manager.h>

#define REGEN_LIGHT_PASS_NODE_CATEGORY "light-pass"

LightPassNodeProvider::LightPassNodeProvider()
: NodeProcessor(REGEN_LIGHT_PASS_NODE_CATEGORY)
{}

void LightPassNodeProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<StateNode> &parent)
{
  if(!input.hasAttribute("shader")) {
    REGEN_WARN("Missing shader attribute for " << input.getDescription() << ".");
    return;
  }
  ref_ptr<LightPass> x = ref_ptr<LightPass>::alloc(
      input.getValue<Light::Type>("type", Light::SPOT),
      input.getValue("shader"));
  parent->state()->joinStates(x);

  bool useShadows = input.getValue<bool>("use-shadows", true);
  if(useShadows) {
    x->setShadowFiltering(input.getValue<ShadowMap::FilterMode>(
        "shadow-filter", ShadowMap::FILTERING_NONE));
    x->setShadowLayer(input.getValue<GLuint>("shadow-layer",1));
  }

  const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
  for(list< ref_ptr<SceneInputNode> >::const_iterator
      it=childs.begin(); it!=childs.end(); ++it)
  {
    ref_ptr<SceneInputNode> n = *it;
    ref_ptr<Light> light = parser->getResources()->getLight(parser,n->getName());
    if(light.get()==NULL) {
      REGEN_WARN("Unable to find Light for '" << n->getDescription() << "'.");
      continue;
    }
    list< ref_ptr<ShaderInput> > inputs;
    ref_ptr<ShadowMap> shadowMap;

    if(n->hasAttribute("shadow-map")) {
      if(!useShadows) {
        REGEN_WARN(input.getDescription() <<
            " has no use-shadows attribute but child " << n->getDescription() << " has.");
      }
      else {
        shadowMap = parser->getResources()->getShadowMap(parser,n->getValue("shadow-map"));
        if(shadowMap.get()==NULL) {
          REGEN_WARN("Unable to find ShadowMap for '" << n->getDescription() << "'.");
        }
      }
    }
    else if(useShadows) {
      REGEN_WARN(n->getDescription() << "' has no associated shadow.");
      continue;
    }

    // Each light pass can have a set of ShaderInput's
    const list< ref_ptr<SceneInputNode> > &childs = n->getChildren();
    for(list< ref_ptr<SceneInputNode> >::const_iterator
        it=childs.begin(); it!=childs.end(); ++it)
    {
      ref_ptr<SceneInputNode> m = *it;
      if(m->getCategory()=="input") {
        inputs.push_back(InputStateProvider::createShaderInput(parser,*m.get()));
      }
      else {
        REGEN_WARN("Unhandled node " << m->getDescription() << ".");
      }
    }

    x->addLight(light,shadowMap,inputs);
  }

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(parent.get());
  x->createShader(shaderConfigurer.cfg());
}
