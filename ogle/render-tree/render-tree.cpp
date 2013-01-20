/*
 * render-tree.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <queue>

#include "render-tree.h"
#include <ogle/utility/stack.h>
#include <ogle/animations/animation-manager.h>
#include <ogle/utility/gl-error.h>
#include <ogle/render-tree/shader-configurer.h>

static inline bool isShaderInputState(State *s)
{
  return dynamic_cast<ShaderInputState*>(s)!=NULL;
}

static inline void traverseTree(RenderState *rs, StateNode *node)
{
  if(rs->isNodeHidden(node)) { return; }

  node->enable(rs);
  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    traverseTree(rs, it->get());
  }
  node->disable(rs);
}

RenderTree::RenderTree()
{
  rootNode_ = ref_ptr<StateNode>::manage(new StateNode);
}

const ref_ptr<StateNode>& RenderTree::rootNode() const
{
  return rootNode_;
}

map<string, ref_ptr<ShaderInput> > RenderTree::collectParentInputs(StateNode &node)
{
  return ShaderConfigurer::configure(&node).inputs_;
}

void RenderTree::traverse(RenderState *rs, GLdouble dt)
{
  traverse(rs, rootNode_.get(), dt);
  handleGLError("after RenderTree::traverse");
}

void RenderTree::traverse(RenderState *rs, StateNode *node, GLdouble dt)
{
  traverseTree(rs, node);
}
