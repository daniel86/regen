/*
 * scene-node.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef SCENE_NODE_H_
#define SCENE_NODE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/state-node.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates StateNode's.
   */
  class SceneNodeProcessor : public NodeProcessor {
  public:
    SceneNodeProcessor();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &state);

  protected:
    void handleAttributes(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &node);
    void handleChildren(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &node);
    ref_ptr<StateNode> createNode(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &parent);
  };
}}

#endif /* SCENE_NODE_H_ */
