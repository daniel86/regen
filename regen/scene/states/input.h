/*
 * input.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_INPUT_H_
#define REGEN_SCENE_INPUT_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/gl-types/shader-input.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates ShaderInput's.
   */
  class InputStateProvider : public StateProcessor {
  public:
    /**
     * Processes SceneInput and creates ShaderInput.
     * @return
     */
    static ref_ptr<ShaderInput> createShaderInput(SceneInputNode &input);

    InputStateProvider();
    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_INPUT_H_ */
