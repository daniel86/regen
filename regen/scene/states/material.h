/*
 * material.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_MATERIAL_H_
#define REGEN_SCENE_MATERIAL_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/fbo-state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates Material's.
   */
  class MaterialStateProvider : public StateProcessor {
  public:
    MaterialStateProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_MATERIAL_H_ */
