/*
 * state-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef STATE_NODE_H_
#define STATE_NODE_H_

#include <ogle/states/state.h>
#include <ogle/states/render-state.h>

/**
 * A node that holds a State.
 */
class StateNode
{
public:
  StateNode();
  StateNode(ref_ptr<State> state);

  /**
   * Hidden nodes do not get enabled/disabled.
   */
  GLboolean isHidden() const;
  /**
   * Hidden nodes do not get enabled/disabled.
   */
  void set_isHidden(GLboolean isHidden);

  /**
   * Sets the parent node.
   * You should call addChild() on the parent too.
   */
  void set_parent(ref_ptr<StateNode> parent);
  /**
   * True is a parent is set.
   */
  GLboolean hasParent() const;
  /**
   * Returns the parent node.
   */
  ref_ptr<StateNode>& parent();

  /**
   * Add a child node.
   * You should call set_parent() on the child too.
   */
  void addChild(ref_ptr<StateNode> child);
  /**
   * Removes a child node.
   */
  void removeChild(ref_ptr<StateNode> state);

  /**
   * List of all child nodes.
   */
  list< ref_ptr<StateNode> >& childs();

  /**
   * Enables the associated state.
   */
  virtual void enable(RenderState *state);
  /**
   * Disables the associated state.
   */
  virtual void disable(RenderState *state);
  /**
   * Enables the associated state, calls
   * traverse() on all children and
   * disables the associated state.
   */
  virtual void traverse(RenderState *state, GLdouble dt);
  /**
   * Let the node hierarchy configure a shader.
   */
  virtual void configureShader(ShaderConfig *cfg);
  virtual void update(GLfloat dt) {};

  ref_ptr<State>& state();
protected:
  ref_ptr<State> state_;
  ref_ptr<StateNode> parent_;
  list< ref_ptr<StateNode> > childs_;
  GLboolean isHidden_;
};

#endif /* STATE_NODE_H_ */
