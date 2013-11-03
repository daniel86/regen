/*
 * direct-shading.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_DIRECT_SHADING_H_
#define REGEN_SCENE_DIRECT_SHADING_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/direct-shading.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates DirectShading nodes.
   */
  class DirectShadingNodeProvider : public NodeProcessor {
  public:
    DirectShadingNodeProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &state);
  };
}}

#endif /* REGEN_SCENE_DIRECT_SHADING_H_ */
