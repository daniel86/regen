/*
 * state.h
 *
 *  Created on: Jan 5, 2014
 *      Author: daniel
 */

#ifndef REGEN_SCENE_STATE_H_
#define REGEN_SCENE_STATE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#define REGEN_STATE_CATEGORY "state"

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates DepthState's.
   */
  class StateProvider : public StateProcessor {
  public:
    StateProvider()
    : StateProcessor(REGEN_STATE_CATEGORY)
    {}

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state)
    {
      if(input.hasAttribute("import")) {
        ref_ptr<State> imported = parser->getResources()->getState(
            parser, input.getValue("import"));
        if(imported.get()==NULL) {
          REGEN_WARN("Unable to import State for '" << input.getDescription() << "'.");
        }
        else {
          state->joinStates(imported);
        }
      }
      else {
        REGEN_WARN("Unable to load State for '" << input.getDescription() << "'.");
      }
    }
  };
}}

#endif /* REGEN_SCENE_STATE_H_ */
