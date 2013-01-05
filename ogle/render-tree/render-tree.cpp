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
static inline bool isVBOState(State *s)
{
  return dynamic_cast<VBOState*>(s)!=NULL;
}

static VBOState* getVBOState(State *s)
{
  if(isVBOState(s)) {
    return (VBOState*)s;
  }
  for(list< ref_ptr<State> >::const_reverse_iterator
      it=s->joined().rbegin(); it!=s->joined().rend(); ++it)
  {
    return getVBOState(it->get());
  }
  return NULL;
}

static void collectOrphanAttributes(
    State *state,
    list<ShaderInputState*> &orphanAttributes)
{
  if(isShaderInputState(state)) {
    ShaderInputState *attState = (ShaderInputState*)(state);
    if( !attState->isBufferSet() ) {
      orphanAttributes.push_back(attState);
    }
  }
  for(list< ref_ptr<State> >::const_iterator
      it=state->joined().begin(); it!=state->joined().end(); ++it)
  {
    collectOrphanAttributes(it->get(), orphanAttributes);
  }
}

static void collectOrphanAttributes(
    StateNode *node,
    list<ShaderInputState*> &orphanAttributes)
{
  State *state = node->state().get();
  collectOrphanAttributes(state,orphanAttributes);
  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    collectOrphanAttributes(it->get(), orphanAttributes);
  }
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

VBOState* RenderTree::getParentVBO(StateNode *node)
{
  VBOState *vboState = getVBOState(node->state().get());
  if(vboState!=NULL) {
    // there is a parent node that is a vbo node
    // try to add the geometry data to this vbo.
    // if it does not fit there is no possibility left to
    // use an existing vbo and a new one must be created
    // for the geometry data.
    // for this case parent also does not contain child nodes
    // with vbo's with enough space
    return vboState;
  } else if(node->hasParent()) {
    return getParentVBO(node->parent());
  } else {
    return NULL;
  }
}

void RenderTree::traverse(RenderState *rs, GLdouble dt)
{
  traverse(rs, rootNode_.get(), dt);
  handleGLError("after RenderTree::traverse");
}

void RenderTree::traverse(RenderState *rs, StateNode *node, GLdouble dt)
{
  // TODO: better do this somewhere else/on the fly
  list<ShaderInputState*> orphanAttributes;
  collectOrphanAttributes(node, orphanAttributes);
  if(!orphanAttributes.empty()){
    VBOState *vboParent = getParentVBO(node);

    // for the case the application has not defined a VBO state before
    // tree traversal we automatically create a VBO at the root node
    if(vboParent==NULL) {
      vboParent = new VBOState(6*1048576, VertexBufferObject::USAGE_DYNAMIC);
      node->state()->joinStates(ref_ptr<State>::manage(vboParent));
      INFO_LOG("VBO created at root node of the render tree.");
    }

    // add orphan attributes to the VBO
    vboParent->add(orphanAttributes, GL_TRUE);
  }

  traverseTree(rs, node);
}
