/*
 * filter-sequence.h
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#ifndef REGEN_SCENE_FILTER_SEQUENCE_H_
#define REGEN_SCENE_FILTER_SEQUENCE_H_

#include <regen/scene/scene-parser.h>
#include <regen/scene/scene-input.h>
#include <regen/scene/input-processors.h>

#include <regen/states/filter.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates FilterSequence nodes.
   */
  class FilterSequenceNodeProvider : public NodeProcessor {
  public:
    FilterSequenceNodeProvider();

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<StateNode> &state);
  };
}}

#endif /* REGEN_SCENE_FILTER_SEQUENCE_H_ */
