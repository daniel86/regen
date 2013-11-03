/*
 * fullscreen-pass.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "fullscreen-pass.h"
using namespace regen::scene;
using namespace regen;

#include <regen/states/state-configurer.h>

#define REGEN_FULLSCREEN_PASS_NODE_CATEGORY "fullscreen-pass"

FullscreenPassNodeProvider::FullscreenPassNodeProvider()
: NodeProcessor(REGEN_FULLSCREEN_PASS_NODE_CATEGORY)
{}

void FullscreenPassNodeProvider::processInput(
    SceneParser *parser,
    SceneInputNode &input,
    const ref_ptr<StateNode> &parent)
{
  if(!input.hasAttribute("shader")) {
    REGEN_WARN("Missing shader attribute for " << input.getDescription() << ".");
    return;
  }
  const string shaderKey = input.getValue("shader");

  ref_ptr<FullscreenPass> fs = ref_ptr<FullscreenPass>::alloc(shaderKey);
  ref_ptr<StateNode> node = ref_ptr<StateNode>::alloc(fs);
  parent->addChild(node);

  StateConfigurer shaderConfigurer;
  shaderConfigurer.addNode(node.get());
  fs->createShader(shaderConfigurer.cfg());
}
