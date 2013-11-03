/*
 * blend.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_BLEND_H_
#define REGEN_SCENE_BLEND_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/blend-state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates BlendState's.
   */
  class BlendStateProvider : public StateProcessor {
  public:
    BlendStateProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* BLEND_H_ */
