/*
 * state-node.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef STATE_NODE_H_
#define STATE_NODE_H_

#include <regen/gl-types/render-state.h>
#include <regen/states/state.h>
#include <regen/states/model-transformation.h>
#include <regen/camera/camera.h>

namespace regen {
  /**
   * \brief A node that holds a State.
   */
  class StateNode
  {
  public:
    StateNode();
    /**
     * @param state the state object.
     */
    StateNode(const ref_ptr<State> &state);
    virtual ~StateNode() {}

    /**
     * @return Node name. Has no semantics.
     */
    const string &name() const;
    /**
     * @param name Node name. Has no semantics.
     */
    void set_name(const string &name);

    /**
     * Removes all children.
     */
    void clear();
    /**
     * @return the state object.
     */
    const ref_ptr<State>& state() const;

    /**
     * @return is the node hidden.
     */
    GLboolean isHidden() const;
    /**
     * @param isHidden is the node hidden.
     */
    void set_isHidden(GLboolean isHidden);

    /**
     * @return true if a parent is set.
     */
    GLboolean hasParent() const;
    /**
     * @return the parent node.
     */
    StateNode *parent() const;
    /**
     * @param parent the parent node.
     */
    void set_parent(StateNode *parent);

    /**
     * Add a child node to the end of the child list.
     */
    void addChild(const ref_ptr<StateNode> &child);
    /**
     * Add a child node to the start of the child list.
     */
    void addFirstChild(const ref_ptr<StateNode> &child);
    /**
     * Removes a child node.
     */
    void removeChild(StateNode *child);

    /**
     * @return list of all child nodes.
     */
    list< ref_ptr<StateNode> >& childs();

    ref_ptr<Camera> getParentCamera();

    /**
     * Scene graph traversal.
     */
    virtual void traverse(RenderState *rs);

  protected:
    ref_ptr<State> state_;
    StateNode *parent_;
    list< ref_ptr<StateNode> > childs_;
    GLboolean isHidden_;
    string name_;
  };
} // namespace

namespace regen {
  /**
   * \brief Provides some global uniforms and keeps
   * a reference on the render state.
   */
  class RootNode : public StateNode
  {
  public:
    RootNode();
    /**
     * Initialize node. Should be called when GL context setup.
     */
    void init();
    /**
     * Tree traversal.
     * @param dt time difference to last traversal.
     */
    void render(GLdouble dt);
    /**
     * Do something after render call.
     * @param dt time difference to last traversal.
     */
    void postRender(GLdouble dt);

  protected:
    ref_ptr<ShaderInput1f> timeDelta_;
  };
} // namespace

namespace regen {
  /**
   * \brief Adds the possibility to traverse child tree
   * n times.
   */
  class LoopNode : public StateNode
  {
  public:
    /**
     * @param numIterations The number of iterations.
     */
    LoopNode(GLuint numIterations);
    /**
     * @param state Associated state.
     * @param numIterations The number of iterations.
     */
    LoopNode(const ref_ptr<State> &state, GLuint numIterations);

    /**
     * @return The number of iterations.
     */
    GLuint numIterations() const;
    /**
     * @param numIterations The number of iterations.
     */
    void set_numIterations(GLuint numIterations);

    // Override
    virtual void traverse(RenderState *rs);

  protected:
    GLuint numIterations_;
  };
} // namespace

namespace regen {
  /**
   * \brief Compares nodes by distance to camera.
   */
  class NodeEyeDepthComparator
  {
  public:
    /**
     * @param cam the perspective camera.
     * @param frontToBack sort front to back or back to front
     */
    NodeEyeDepthComparator(const ref_ptr<Camera> &cam, GLboolean frontToBack);

    /**
     * @param worldPosition the world position.
     * @return world position camera distance.
     */
    GLfloat getEyeDepth(const Vec3f &worldPosition) const;
    /**
     * @param n a node.
     * @return a model view matrix.
     */
    ModelTransformation* findModelTransformation(StateNode *n) const;
    /**
     * Do the comparison.
     */
    bool operator()(ref_ptr<StateNode> &n0, ref_ptr<StateNode> &n1) const;
  protected:
    ref_ptr<Camera> cam_;
    GLint mode_;
  };
} // namespace

#endif /* STATE_NODE_H_ */
