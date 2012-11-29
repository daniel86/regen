/*
 * state-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "state-node.h"

StateNode::StateNode()
: state_(ref_ptr<State>::manage(new State)),
  parent_(NULL),
  isHidden_(GL_FALSE)
{
}

StateNode::StateNode(ref_ptr<State> state)
: state_(state),
  parent_(NULL),
  isHidden_(GL_FALSE)
{
}

GLboolean StateNode::isHidden() const
{
  return isHidden_;
}
void StateNode::set_isHidden(GLboolean isHidden)
{
  isHidden_ = isHidden;
}

ref_ptr<State>& StateNode::state()
{
  return state_;
}

void StateNode::set_parent(StateNode *parent)
{
  parent_ = parent;
}
StateNode* StateNode::parent()
{
  return parent_;
}
GLboolean StateNode::hasParent() const
{
  return parent_!=NULL;
}

void StateNode::addChild(ref_ptr<StateNode> child)
{
  if(child->parent_!=NULL) {
    child->parent_->removeChild(child);
  }
  childs_.push_back(child);
  child->set_parent( this );
}

void StateNode::removeChild(ref_ptr<StateNode> child)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=childs_.begin(); it!=childs_.end(); ++it)
  {
    if(it->get() == child.get())
    {
      childs_.erase(it);
      child->set_parent( NULL );
      break;
    }
  }
}

list< ref_ptr<StateNode> >& StateNode::childs()
{
  return childs_;
}

void StateNode::enable(RenderState *state)
{
  if(!state->isStateHidden(state_.get())) {
    state_->enable(state);
  }
}

void StateNode::disable(RenderState *state)
{
  if(!state->isStateHidden(state_.get())) {
    state_->disable(state);
  }
}

void StateNode::configureShader(ShaderConfig *cfg)
{
  if(parent_!=NULL) {
    parent_->configureShader(cfg);
  }
  state_->configureShader(cfg);
}
