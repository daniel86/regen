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
#include <regen/gl-types/gl-enum.h>

#define REGEN_CULL_STATE_CATEGORY "cull"

#include <regen/states/atomic-states.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates CullState.
   */
  class CullStateProvider : public StateProcessor {
  public:
    CullStateProvider()
    : StateProcessor(REGEN_CULL_STATE_CATEGORY)
    {}

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state)
    {
      if(input.hasAttribute("mode")) {
        state->joinStates(ref_ptr<CullFaceState>::alloc(
            glenum::cullFace(input.getValue("mode"))));
      }
      if(input.hasAttribute("winding-order")) {
        state->joinStates(ref_ptr<FrontFaceState>::alloc(
            glenum::frontFace(input.getValue("winding-order"))));
      }
    }
  };
}}

#endif /* REGEN_SCENE_CULL_H_ */
