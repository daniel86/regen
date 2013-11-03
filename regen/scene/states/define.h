/*
 * define.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_DEFINE_H_
#define REGEN_SCENE_DEFINE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates defines in last State.
   */
  class DefineStateProvider : public StateProcessor {
  public:
    DefineStateProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_DEFINE_H_ */
