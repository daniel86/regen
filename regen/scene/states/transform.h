/*
 * transform.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_TRANSFORM_H_
#define REGEN_SCENE_TRANSFORM_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/model-transformation.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates Transform state.
   */
  class TransformStateProvider : public StateProcessor {
  public:
    /**
     * Creates PhysicalObject from SceneInputNode.
     * @param input The SceneInputNode.
     * @param motion The motion state that is used for pose synchronization.
     * @return The physical Object created or a null reference.
     */
    static ref_ptr<PhysicalProps> createPhysicalObject(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<btMotionState> &motion);

    TransformStateProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &state);
  };
}}

#endif /* REGEN_SCENE_TRANSFORM_H_ */
