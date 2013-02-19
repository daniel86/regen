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
  NodeEyeDepthComparator(const ref_ptr<PerspectiveCamera> &cam, GLboolean frontToBack);

  /**
   * Calculate eye depth given by world position.
   */
  GLfloat getEyeDepth(const Vec3f &worldPosition) const;
  /**
   * Finds first child state that defines a model view matrix.
   */
  ModelTransformation* findModelTransformation(StateNode *n) const;
  /**
   * Do the comparison.
   */
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
  StateNode(const ref_ptr<State> &state);
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
   * The parent node.
   */
  StateNode *parent() const;
  /**
   * The parent node.
   */
  virtual void set_parent(StateNode *parent);

  /**
   * Add a child node to the end of the child list.
   * You should call set_parent() on the child too.
   */
  virtual void addChild(const ref_ptr<StateNode> &child);
  /**
   * Add a child node to the start of the child list.
   * You should call set_parent() on the child too.
   */
  virtual void addFirstChild(const ref_ptr<StateNode> &child);
  /**
   * Removes a child node.
   */
  virtual void removeChild(const ref_ptr<StateNode> &state);
  /**
   * Removes a child node.
   */
  virtual void removeChild(StateNode *child);

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

  const ref_ptr<State>& state() const;
protected:
  ref_ptr<State> state_;
  StateNode *parent_;
  list< ref_ptr<StateNode> > childs_;
  GLboolean isHidden_;
};

/**
 * Root nodes provides some global uniforms and keeps
 * a reference on the render state.
 */
class RootNode : public StateNode
{
public:
  static void traverse(RenderState *rs, StateNode *node);

  RootNode();

  void set_renderState(const ref_ptr<RenderState> &rs);
  void set_mousePosition(const Vec2f &pos);

  virtual void render(GLdouble dt);
  virtual void postRender(GLdouble dt);

protected:
  ref_ptr<RenderState> rs_;

  ref_ptr<ShaderInput1f> timeDelta_;
  ref_ptr<ShaderInput2f> mousePosition_;
};

#endif /* STATE_NODE_H_ */
