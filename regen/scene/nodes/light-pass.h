/*
 * light-pass.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_LIGHT_PASS_H_
#define REGEN_SCENE_LIGHT_PASS_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/light-pass.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates LightPass nodes.
   */
  class LightPassNodeProvider : public NodeProcessor {
  public:
    LightPassNodeProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &state);
  };
}}

#endif /* REGEN_SCENE_LIGHT_PASS_H_ */
