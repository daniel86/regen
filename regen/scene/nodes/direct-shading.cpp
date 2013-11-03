/*
 * direct-shading.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "direct-shading.h"
using namespace regen::scene;
using namespace regen;

#include <regen/scene/nodes/scene-node.h>
#include <regen/scene/resource-manager.h>

#define REGEN_DIRECT_SHADING_NODE_CATEGORY "direct-shading"

DirectShadingNodeProvider::DirectShadingNodeProvider()
: NodeProcessor(REGEN_DIRECT_SHADING_NODE_CATEGORY)
{}

void DirectShadingNodeProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<StateNode> &parent)
{
  ref_ptr<SceneInputNode> lightNode = input.getFirstChild("direct-lights");
  ref_ptr<SceneInputNode> passNode  = input.getFirstChild("direct-pass");
  if(lightNode.get()==NULL) {
    REGEN_WARN("Missing direct-lights attribute for " << input.getDescription() << ".");
    return;
  }
  if(passNode.get()==NULL) {
    REGEN_WARN("Missing direct-pass attribute for " << input.getDescription() << ".");
    return;
  }

  ref_ptr<DirectShading> shadingState = ref_ptr<DirectShading>::alloc();
  ref_ptr<StateNode> shadingNode = ref_ptr<StateNode>::alloc(shadingState);
  parent->addChild(shadingNode);

  if(input.hasAttribute("ambient")) {
    shadingState->ambientLight()->setVertex(0,
        input.getValue<Vec3f>("ambient",Vec3f(0.1f)));
  }

  // load lights
  const list< ref_ptr<SceneInputNode> > &childs = lightNode->getChildren();
  for(list< ref_ptr<SceneInputNode> >::const_iterator
      it=childs.begin(); it!=childs.end(); ++it)
  {
    ref_ptr<SceneInputNode> n = *it;
    if(n->getCategory()!="light") {
      REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
      continue;
    }

    ref_ptr<Light> light = parser->getResources()->getLight(parser,n->getName());
    if(light.get()==NULL) {
      REGEN_WARN("Unable to find Light for '" << n->getDescription() << "'.");
      continue;
    }

    ShadowMap::FilterMode shadowFiltering =
        n->getValue<ShadowMap::FilterMode>("shadow-filter",ShadowMap::FILTERING_NONE);

    if(n->hasAttribute("shadow-map")) {
      const string shadowMapID(n->getValue("shadow-map"));
      ref_ptr<ShadowMap> shadowMap = parser->getResources()->getShadowMap(parser,shadowMapID);
      if(shadowMap.get()==NULL) {
        shadingState->addLight(light);
      }
      else {
        shadingState->addLight(light,shadowMap,shadowFiltering);
      }
    }
    else {
      shadingState->addLight(light);
    }
  }

  // parse passNode
  SceneNodeProcessor().processInput(parser, *passNode.get(), shadingNode);
}
