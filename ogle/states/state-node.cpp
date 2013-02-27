/*
 * state-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "state-node.h"
#include <ogle/animations/animation-manager.h>

NodeEyeDepthComparator::NodeEyeDepthComparator(
    const ref_ptr<PerspectiveCamera> &cam, GLboolean frontToBack)
: cam_(cam),
  mode_(((GLint)frontToBack)*2 - 1)
{
}

GLfloat NodeEyeDepthComparator::getEyeDepth(const Vec3f &p) const
{
  const Mat4f &mat = cam_->viewMatrix();
  return mat.x[2]*p.x + mat.x[6]*p.y + mat.x[10]*p.z + mat.x[14];
}

ModelTransformation* NodeEyeDepthComparator::findModelTransformation(StateNode *n) const
{
  State *nodeState = n->state().get();
  ModelTransformation *ret = dynamic_cast<ModelTransformation*>(nodeState);
  if(ret != NULL) { return ret; }

  for(list< ref_ptr<State> >::const_iterator
      it=nodeState->joined().begin(); it!=nodeState->joined().end(); ++it)
  {
    ModelTransformation *ret = dynamic_cast<ModelTransformation*>(it->get());
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
  ModelTransformation *modelMat0 = findModelTransformation(n0.get());
  ModelTransformation *modelMat1 = findModelTransformation(n1.get());
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

StateNode::StateNode(const ref_ptr<State> &state)
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

const ref_ptr<State>& StateNode::state() const
{
  return state_;
}

void StateNode::set_parent(StateNode *parent)
{
  parent_ = parent;
}
StateNode* StateNode::parent() const
{
  return parent_;
}
GLboolean StateNode::hasParent() const
{
  return parent_!=NULL;
}

void StateNode::addChild(const ref_ptr<StateNode> &child)
{
  if(child->parent_!=NULL) {
    child->parent_->removeChild(child);
  }
  childs_.push_back(child);
  child->set_parent( this );
}
void StateNode::addFirstChild(const ref_ptr<StateNode> &child)
{
  if(child->parent_!=NULL) {
    child->parent_->removeChild(child);
  }
  childs_.push_front(child);
  child->set_parent( this );
}

void StateNode::removeChild(StateNode *child)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=childs_.begin(); it!=childs_.end(); ++it)
  {
    if(it->get() == child)
    {
      child->set_parent( NULL );
      childs_.erase(it);
      break;
    }
  }
}
void StateNode::removeChild(const ref_ptr<StateNode> &child)
{
  removeChild(child.get());
}

list< ref_ptr<StateNode> >& StateNode::childs()
{
  return childs_;
}

void StateNode::enable(RenderState *state)
{
  state_->enable(state);

}

void StateNode::disable(RenderState *state)
{
  state_->disable(state);
}

//////////////
//////////////

RootNode::RootNode() : StateNode()
{
  timeDelta_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("deltaT"));
  timeDelta_->setUniformData(0.0f);
  state_->joinShaderInput(ref_ptr<ShaderInput>::cast(timeDelta_));

  mousePosition_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("mousePosition"));
  mousePosition_->setUniformData(0.0f);
  state_->joinShaderInput(ref_ptr<ShaderInput>::cast(mousePosition_));
}

void RootNode::set_renderState(const ref_ptr<RenderState> &rs)
{
  rs_ = rs;
}
void RootNode::set_mousePosition(const Vec2f &pos)
{
  mousePosition_->setVertex2f(0, pos);
}

void RootNode::traverse(RenderState *rs, StateNode *node)
{
  node->enable(rs);
  for(list< ref_ptr<StateNode> >::iterator
      it=node->childs().begin(); it!=node->childs().end(); ++it)
  {
    traverse(rs, it->get());
  }
  node->disable(rs);
}

void RootNode::render(GLdouble dt)
{
  handleGLError("before RootNode::render");

  timeDelta_->setUniformData(dt);
  traverse(rs_.get(), this);

  handleGLError("after RootNode::render");
}
void RootNode::postRender(GLdouble dt)
{
  //AnimationManager::get().nextFrame();
  // some animations modify the vertex data,
  // updating the vbo needs a context so we do it here in the main thread..
  AnimationManager::get().updateGraphics(dt);
  // invoke event handler of queued events
  EventObject::emitQueued();
}
