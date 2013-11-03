/*
 * state-sequence.cpp
 *
 *  Created on: Nov 3, 2013
 *      Author: daniel
 */

#include "state-sequence.h"
using namespace regen::scene;
using namespace regen;

#define REGEN_STATE_SEQUENCE_NODE_CATEGORY "state-sequence"

StateSequenceNodeProvider::StateSequenceNodeProvider()
: StateProcessor(REGEN_STATE_SEQUENCE_NODE_CATEGORY)
{}

void StateSequenceNodeProvider::processInput(
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
