/*
 * fbo.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_FBO_H_
#define REGEN_SCENE_FBO_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/fbo-state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates FBOState's.
   */
  class FBOStateProvider : public StateProcessor {
  public:
    FBOStateProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_FBO_H_ */
