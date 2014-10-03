/*
 * sky.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_SKY_NODE_H_
#define REGEN_SCENE_SKY_NODE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>
#include <regen/states/state-configurer.h>
#include <regen/scene/resource-manager.h>
#include <regen/scene/nodes/shader.h>

#define REGEN_MESH_SKY_CATEGORY "sky"

#include <regen/meshes/mesh-state.h>

namespace regen {
namespace scene {
  class SkyNodeProvider : public NodeProcessor {
  public:
    SkyNodeProvider()
    : NodeProcessor(REGEN_MESH_SKY_CATEGORY)
    {}

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &parent)
    {
      ref_ptr<Sky> skyNode = parser->getResources()->getSky(parser,input.getName());
      if(skyNode.get()==NULL) {
        REGEN_WARN("Unable to load sky for '" << input.getDescription() << "'.");
        return;
      }

      parent->addChild(skyNode);
      StateConfigurer stateConfigurer;
      stateConfigurer.addNode(skyNode.get());
      skyNode->createShader(RenderState::get(),stateConfigurer.cfg());
    }
  };
}}

#endif /* REGEN_SCENE_SKY_NODE_H_ */
