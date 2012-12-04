/*
 * state-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "state-node.h"

NodeEyeDepthComparator::NodeEyeDepthComparator(ref_ptr<PerspectiveCamera> &cam, GLboolean frontToBack)
: cam_(cam),
  mode_(((GLint)frontToBack)*2 - 1)
{
}

GLfloat NodeEyeDepthComparator::getEyeDepth(const Vec3f &p) const
{
  const Mat4f &mat = cam_->viewMatrix();
  return mat.x[2]*p.x + mat.x[6]*p.y + mat.x[10]*p.z + mat.x[14];
}

ModelTransformationState* NodeEyeDepthComparator::findModelTransformation(StateNode *n) const
{
  State *nodeState = n->state().get();
  ModelTransformationState *ret = dynamic_cast<ModelTransformationState*>(nodeState);
  if(ret != NULL) { return ret; }

  for(list< ref_ptr<State> >::iterator
      it=nodeState->joined().begin(); it!=nodeState->joined().end(); ++it)
  {
    ModelTransformationState *ret = dynamic_cast<ModelTransformationState*>(it->get());
    if(ret != NULL) { return ret; }
  }

  for(list< ref_ptr<StateNode> >::iterator
      it=n->childs().begin(); it!=n->childs().end(); ++it)
  {
    ret = findModelTransformation(it->get());
    if(ret != NULL) { return ret; }
  }

  return NULL;
}

bool NodeEyeDepthComparator::operator()(ref_ptr<StateNode> &n0, ref_ptr<StateNode> &n1) const
{
  ModelTransformationState *modelMat0 = findModelTransformation(n0.get());
  ModelTransformationState *modelMat1 = findModelTransformation(n1.get());
  if(modelMat0!=NULL && modelMat1!=NULL) {
    GLfloat diff = mode_ * (
        getEyeDepth( modelMat0->translation() ) -
        getEyeDepth( modelMat1->translation() ) );
    return diff<0;
  }
  else if(modelMat0!=NULL) {
    return true;
  }
  else if(modelMat1!=NULL) {
    return false;
  }
  else {
    return n0<n1;
  }
}

/////////

StateNode::StateNode()
: state_(ref_ptr<State>::manage(new State)),
  parent_(NULL),
  isHidden_(GL_FALSE)
{
}

StateNode::StateNode(ref_ptr<State> state)
: state_(state),
  parent_(NULL),
  isHidden_(GL_FALSE)
{
}

GLboolean StateNode::isHidden() const
{
  return isHidden_;
}
void StateNode::set_isHidden(GLboolean isHidden)
{
  isHidden_ = isHidden;
}

ref_ptr<State>& StateNode::state()
{
  return state_;
}

void StateNode::set_parent(StateNode *parent)
{
  parent_ = parent;
}
StateNode* StateNode::parent()
{
  return parent_;
}
GLboolean StateNode::hasParent() const
{
  return parent_!=NULL;
}

void StateNode::addChild(ref_ptr<StateNode> child)
{
  if(child->parent_!=NULL) {
    child->parent_->removeChild(child);
  }
  childs_.push_back(child);
  child->set_parent( this );
}

void StateNode::removeChild(ref_ptr<StateNode> child)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=childs_.begin(); it!=childs_.end(); ++it)
  {
    if(it->get() == child.get())
    {
      childs_.erase(it);
      child->set_parent( NULL );
      break;
    }
  }
}

list< ref_ptr<StateNode> >& StateNode::childs()
{
  return childs_;
}

void StateNode::enable(RenderState *state)
{
  if(!state->isStateHidden(state_.get())) {
    state_->enable(state);
  }
}

void StateNode::disable(RenderState *state)
{
  if(!state->isStateHidden(state_.get())) {
    state_->disable(state);
  }
}

void StateNode::configureShader(ShaderConfig *cfg)
{
  if(parent_!=NULL) {
    parent_->configureShader(cfg);
  }
  state_->configureShader(cfg);
}
