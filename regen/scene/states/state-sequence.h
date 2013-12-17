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

#define REGEN_STATE_SEQUENCE_NODE_CATEGORY "state-sequence"

#include <regen/states/state.h>

namespace regen {
namespace scene {
  /**
   * Processes SceneInput and creates StateSequence's.
   */
  class StateSequenceNodeProvider : public StateProcessor {
  public:
    StateSequenceNodeProvider()
    : StateProcessor(REGEN_STATE_SEQUENCE_NODE_CATEGORY)
    {}

    // Override
    void processInput(
        SceneParser *parser,
        SceneInputNode &input,
        const ref_ptr<State> &parent)
    {
      ref_ptr<StateSequence> seq = ref_ptr<StateSequence>::alloc();
      parent->joinStates(seq);

      // all states allowed as children
      const list< ref_ptr<SceneInputNode> > &childs = input.getChildren();
      for(list< ref_ptr<SceneInputNode> >::const_iterator
          it=childs.begin(); it!=childs.end(); ++it)
      {
        ref_ptr<SceneInputNode> n = *it;
        ref_ptr<StateProcessor> processor = parser->getStateProcessor(n->getCategory());
        if(processor.get()==NULL) {
          REGEN_WARN("No processor registered for '" << n->getDescription() << "'.");
          return;
        }
        processor->processInput(parser,*n.get(),seq);
      }
    }
  };
}}

#endif /* REGEN_SCENE_STATE_SEQUENCE_H_ */
