/*
 * state-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef STATE_NODE_H_
#define STATE_NODE_H_

#include <ogle/states/state.h>
#include <ogle/shader/shader-generator.h>
#include <ogle/states/render-state.h>

class StateNode
{
public:
  StateNode();
  StateNode(ref_ptr<State> &state);

  bool isHidden() const;
  void set_isHidden(bool isHidden);

  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
  virtual void traverse(RenderState *state);
  virtual void configureShader(ShaderConfiguration *cfg);

  void set_parent(ref_ptr<StateNode> &parent);
  ref_ptr<StateNode>& parent();
  bool hasParent() const;

  void addChild(ref_ptr<StateNode> &child);
  void removeChild(ref_ptr<StateNode> &state);

  list< ref_ptr<StateNode> >& childs();

  ref_ptr<State>& state();
protected:
  ref_ptr<State> state_;
  ref_ptr<StateNode> parent_;
  list< ref_ptr<StateNode> > childs_;
  bool isHidden_;
};

#endif /* STATE_NODE_H_ */
