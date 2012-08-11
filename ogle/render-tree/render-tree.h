/*
 * render-tree.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef RENDER_TREE_H_
#define RENDER_TREE_H_

#include <ogle/render-tree/state-node.h>
#include <ogle/states/attribute-state.h>
#include <ogle/states/vbo-state.h>

typedef bool (*NodeHiddenFunc)(StateNode*,void*);

/**
 * A tree with StateNode's.
 * For rendering a depth first traversal is used.
 */
class RenderTree
{
public:
  /**
   * Creates a render tree for previously created nodes.
   */
  RenderTree(ref_ptr<StateNode> &node);
  /**
   * Creates empty tree (only with a root node)
   */
  RenderTree();

  /**
   * The root of the tree.
   * You may want to join some global states to the node.
   */
  ref_ptr<StateNode>& rootNode();

  /**
   * Add a child to a node and automatically add attributes
   * to a VBOState of the tree. If no parent VBO set then
   * one is generated.
   */
  void addChild(
      ref_ptr<StateNode> parent,
      ref_ptr<StateNode> child,
      GLboolean generateVBONode=true);
  /**
   * Removes previously added node.
   */
  void remove(ref_ptr<StateNode> node);

  /**
   * Tree traverse starting from root node.
   * The nodes are processed depth first and
   * for each node StateNode::enable is called before the child nodes
   * are processed and StateNode::disable afterwards.
   */
  void traverse(RenderState *state);
  /**
   * Tree traverse starting from given node, all parent states are ignored.
   * The nodes are processed depth first and
   * for each node StateNode::enable is called before the child nodes
   * are processed and StateNode::disable afterwards.
   */
  void traverse(RenderState *state, ref_ptr<StateNode> node);

  /**
   * Supposed to update states with data generated
   * in other threads.
   *
   * This also updates AnimationManager and synchronizes
   * the rendering thread with the animation step
   * (the faster one waits on the slower one each frame).
   *
   * If AttributeState's have new attributes set then
   * the data is re-added to a VBO.
   * This is done very last in this call so that
   * State::update implementation can set new attributes.
   *
   * It might be a good idea to call this after swap
   * buffers each frame.
   */
  void updateStates(GLfloat dt);

  /**
   * Generates a shader program for the given nodes
   * and parents.
   * Usually you want to call generateShader at leafs where all
   * information is available.
   * At least a camera and a attribute state should be available
   * through node or nodes parents.
   */
  ref_ptr<Shader> generateShader(StateNode &node);
  /**
   * Generates a shader program for the given nodes
   * and parents.
   * Usually you want to call generateShader at leafs where all
   * information is available.
   * At least a camera and a attribute state should be available
   * through node or nodes parents.
   * This variant is used so that you could use custom
   * ShaderConfiguration, ShaderGenerator implementations.
   */
  ref_ptr<Shader> generateShader(
      StateNode &node,
      ShaderGenerator *gen,
      ShaderConfiguration *cfg);

protected:
  ref_ptr<StateNode> rootNode_;

  RenderTree(const RenderTree&);

  void removeFromParentVBO(
      ref_ptr<StateNode> parent,
      ref_ptr<StateNode> child,
      const set< AttributeState* > &geomNodes);
  bool addToParentVBO(
      ref_ptr<StateNode> parent,
      ref_ptr<StateNode> child,
      set< AttributeState* > &geomNodes);
  void findUnhandledGeomNodes(
      ref_ptr<StateNode> node,
      set< AttributeState* > &ret);
  GLboolean hasUnhandledGeometry(State *s);
};

#endif /* RENDER_TREE_H_ */
