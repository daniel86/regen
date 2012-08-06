/*
 * state-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "state-node.h"

StateNode::StateNode()
: state_(ref_ptr<State>::manage(new State)),
  isHidden_(false)
{
}

StateNode::StateNode(ref_ptr<State> &state)
: state_(state),
  isHidden_(false)
{
}

bool StateNode::isHidden() const
{
  return isHidden_;
}
void StateNode::set_isHidden(bool isHidden)
{
  isHidden_ = isHidden;
}

ref_ptr<State>& StateNode::state()
{
  return state_;
}

void StateNode::set_parent(ref_ptr<StateNode> &parent)
{
  parent_ = parent;
}
ref_ptr<StateNode>& StateNode::parent()
{
  return parent_;
}
bool StateNode::hasParent() const
{
  return parent_.get()!=NULL;
}

void StateNode::addChild(ref_ptr<StateNode> &child)
{
  childs_.push_back(child);
}

void StateNode::removeChild(ref_ptr<StateNode> &child)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=childs_.begin(); it!=childs_.end(); ++it)
  {
    if(it->get() == child.get())
    {
      childs_.erase(it);
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
  state_->enable(state);
}

void StateNode::disable(RenderState *state)
{
  state_->disable(state);
}

void StateNode::traverse(RenderState *state)
{
  if(isHidden()) { return; }
  state_->enable(state);
  for(list< ref_ptr<StateNode> >::iterator
      it=childs_.begin(); it!=childs_.end(); ++it)
  {
    traverse(state);
  }
  state_->disable(state);
}

void StateNode::configureShader(ShaderConfiguration *cfg)
{
  if(parent_.get()!=NULL) {
    parent_->configureShader(cfg);
  }
  state_->configureShader(cfg);
}
