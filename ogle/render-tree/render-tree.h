/*
 * render-tree.h
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#ifndef RENDER_TREE_H_
#define RENDER_TREE_H_

#include <state-node.h>
#include <attribute-state.h>
#include <vbo-state.h>

typedef bool (*NodeHiddenFunc)(StateNode*,void*);

class RenderTree
{
public:
  RenderTree(ref_ptr<StateNode> &node);
  RenderTree();
  ~RenderTree();

  ref_ptr<StateNode>& rootNode();

  void addChild(
      ref_ptr<StateNode> &parent,
      ref_ptr<StateNode> &child,
      bool generateVBONode=true);
  void remove(ref_ptr<StateNode> &node);

  void traverse(RenderState *state);
  void traverse(RenderState *state, ref_ptr<StateNode> &node);

  void optimize();

  ref_ptr<Shader> generateShader(
      StateNode &node);
  ref_ptr<Shader> generateShader(
      StateNode &node,
      ShaderConfiguration *cfg);

protected:
  ref_ptr<StateNode> rootNode_;

  RenderTree(const RenderTree&);

  void removeFromParentVBO(
      ref_ptr<StateNode> &parent,
      ref_ptr<StateNode> &child,
      const list< ref_ptr<AttributeState> > &geomNodes);
  bool addToParentVBO(
      ref_ptr<StateNode> &parent,
      ref_ptr<StateNode> &child,
      list< ref_ptr<AttributeState> > &geomNodes);
  void findUnhandledGeomNodes(
      ref_ptr<StateNode> &node,
      list< ref_ptr<AttributeState> > &ret);
};

#endif /* RENDER_TREE_H_ */
