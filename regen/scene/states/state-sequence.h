/*
 * state-sequence.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_STATE_SEQUENCE_H_
#define REGEN_SCENE_STATE_SEQUENCE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates StateSequence's.
   */
  class StateSequenceNodeProvider : public StateProcessor {
  public:
    StateSequenceNodeProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_STATE_SEQUENCE_H_ */
