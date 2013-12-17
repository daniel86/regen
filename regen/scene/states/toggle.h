/*
 * toggle.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_TOGGLE_H_
#define REGEN_SCENE_TOGGLE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#define REGEN_TOGGLE_STATE_CATEGORY "toggle"

#include <regen/states/atomic-states.h>

namespace regen {
  namespace scene {
    /**
     * Processes SceneInput and creates ToggleState.
     */
    class ToggleStateProvider : public StateProcessor {
    public:
      ToggleStateProvider()
      : StateProcessor(REGEN_TOGGLE_STATE_CATEGORY)
      {}

      // Override
      void processInput(
          SceneParser *parser,
          SceneInputNode &input,
          const ref_ptr<State> &state)
      {
        if(!input.hasAttribute("key")) {
          REGEN_WARN("Ignoring " << input.getDescription() << " without key attribute.");
          return;
        }
        state->joinStates(ref_ptr<ToggleState>::alloc(
            input.getValue<RenderState::Toggle>("key",RenderState::CULL_FACE),
            input.getValue<bool>("value",true)));
      }
    };
  }
}

#endif /* REGEN_SCENE_TOGGLE_H_ */
