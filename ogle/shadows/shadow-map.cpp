/*
 * shadow-map.cpp
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#include "shadow-map.h"

#include <ogle/states/polygon-offset-state.h>
#include <ogle/states/cull-state.h>

static void traverseTree(RenderState *rs, StateNode *node)
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

class ShadowRenderState : public RenderState
{
public:
  virtual void pushFBO(FrameBufferObject *tex) {}
  virtual void popFBO() {}
};

ShadowMap::ShadowMap()
: Animation(), State()
{
  //setCullFrontFaces(GL_TRUE);
  setPolygonOffset();
}

void ShadowMap::setPolygonOffset(GLfloat factor, GLfloat units)
{
  if(polygonOffsetState_.get()) {
    disjoinStates(polygonOffsetState_);
  }
  polygonOffsetState_ = ref_ptr<State>::manage(new PolygonOffsetState(factor,units));
  joinStates(polygonOffsetState_);
}

void ShadowMap::setCullFrontFaces(GLboolean v)
{
  if(cullState_.get()) {
    disjoinStates(cullState_);
  }
  if(v) {
    cullState_ = ref_ptr<State>::manage(new CullFrontFaceState);
    joinStates(cullState_);
  } else {
    cullState_ = ref_ptr<State>();
  }
}

void ShadowMap::addCaster(ref_ptr<StateNode> &caster)
{
  caster_.push_back(caster);
}
void ShadowMap::removeCaster(StateNode *caster)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    ref_ptr<StateNode> &n = *it;
    if(n.get()==caster) {
      caster_.erase(it);
      break;
    }
  }
}

void ShadowMap::traverse()
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    ShadowRenderState rs;
    traverseTree(&rs, it->get());
  }
}

void ShadowMap::animate(GLdouble dt)
{
}

void ShadowMap::updateGraphics(GLdouble dt)
{
  ShadowRenderState rs;
  enable(&rs);
  updateShadow();
  disable(&rs);
}
