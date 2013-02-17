/*
 * render-tree.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef RENDER_TREE_H_
#define RENDER_TREE_H_

#include <ogle/render-tree/state-node.h>
#include <ogle/states/shader-input-state.h>
#include <ogle/utility/event-object.h>

typedef bool (*NodeHiddenFunc)(StateNode*,void*);

/**
 * A tree with StateNode's.
 * For rendering a depth first traversal is used.
 */
class RenderTree : public EventObject
{
public:
  /**
   * Creates empty tree (only with a root node)
   */
  RenderTree();
  virtual ~RenderTree();

  /**
   * The root of the tree.
   * You may want to join some global states to the node.
   */
  const ref_ptr<StateNode>& rootNode() const;

  /**
   * Tree traverse starting from root node.
   * The nodes are processed depth first and
   * for each node StateNode::enable is called before the child nodes
   * are processed and StateNode::disable afterwards.
   */
  void traverse(RenderState *rs, GLdouble dt);
  /**
   * Tree traverse starting from given node, all parent states are ignored.
   * The nodes are processed depth first and
   * for each node StateNode::enable is called before the child nodes
   * are processed and StateNode::disable afterwards.
   */
  static void traverse(RenderState *rs, StateNode *node, GLdouble dt);

  virtual void render(GLdouble dt);
  virtual void postRender(GLdouble dt);

  map<string, ref_ptr<ShaderInput> > collectParentInputs(StateNode &node);

  void set_renderState(const ref_ptr<RenderState> &rs);

protected:
  ref_ptr<StateNode> rootNode_;
  ref_ptr<ShaderInput1f> timeDelta_;
  ref_ptr<RenderState> rs_;

  RenderTree(const RenderTree&);
};

#endif /* RENDER_TREE_H_ */
