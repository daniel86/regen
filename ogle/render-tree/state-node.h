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
#include <ogle/states/model-transformation.h>
#include <ogle/states/camera.h>

class StateNode;

/**
 * Can be used to sort children of a node by
 * eye space z distance to camera. This might be useful
 * for order dependent transparency handling.
 */
class NodeEyeDepthComparator
{
public:
  /**
   * @frontToBack: sort front to back or back to front ?
   */
  NodeEyeDepthComparator(ref_ptr<PerspectiveCamera> &cam, GLboolean frontToBack);

  /**
   * Calculate eye depth given by world position.
   */
  GLfloat getEyeDepth(const Vec3f &worldPosition) const;
  /**
   * Finds first child state that defines a model view matrix.
   */
  ModelTransformationState* findModelTransformation(StateNode *n) const;

  bool operator()(ref_ptr<StateNode> &n0, ref_ptr<StateNode> &n1) const;
protected:
  ref_ptr<PerspectiveCamera> cam_;
  GLint mode_;
};

/**
 * A node that holds a State.
 */
class StateNode
{
public:
  StateNode();
  StateNode(ref_ptr<State> state);
  virtual ~StateNode() {}

  /**
   * Hidden nodes do not get enabled/disabled.
   */
  GLboolean isHidden() const;
  /**
   * Hidden nodes do not get enabled/disabled.
   */
  void set_isHidden(GLboolean isHidden);

  /**
   * True is a parent is set.
   */
  GLboolean hasParent() const;
  /**
   * Returns the parent node.
   */
  StateNode *parent();

  virtual void set_parent(StateNode *parent);
  /**
   * Add a child node.
   * You should call set_parent() on the child too.
   */
  virtual void addChild(ref_ptr<StateNode> child);
  /**
   * Removes a child node.
   */
  virtual void removeChild(ref_ptr<StateNode> state);

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
   * Let the node hierarchy configure a shader.
   */
  virtual void configureShader(ShaderConfig *cfg);

  ref_ptr<State>& state();
protected:
  ref_ptr<State> state_;
  StateNode *parent_;
  list< ref_ptr<StateNode> > childs_;
  GLboolean isHidden_;
};

#endif /* STATE_NODE_H_ */
