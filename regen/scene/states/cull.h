/*
 * cull.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_CULL_H_
#define REGEN_SCENE_CULL_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/atomic-states.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates CullState.
   */
  class CullStateProvider : public StateProcessor {
  public:
    CullStateProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_CULL_H_ */
