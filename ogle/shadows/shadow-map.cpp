/*
 * shadow-map.cpp
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#include "shadow-map.h"

#include <ogle/states/polygon-offset-state.h>
#include <ogle/states/cull-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/meshes/rectangle.h>

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

GLboolean DepthRenderState::isStateHidden(State *state)
{
  return (
      dynamic_cast<DepthState*>(state)!=NULL ||
      dynamic_cast<CullDisableState*>(state)!=NULL ||
      dynamic_cast<CullEnableState*>(state)!=NULL ||
      state->isHidden());
}

///////////
//////////


Mat4f ShadowMap::biasMatrix_ = Mat4f(
  0.5, 0.0, 0.0, 0.0,
  0.0, 0.5, 0.0, 0.0,
  0.0, 0.0, 0.5, 0.0,
  0.5, 0.5, 0.5, 1.0 );

ShadowMap::ShadowMap(const ref_ptr<Light> &light, GLuint shadowMapSize)
: Animation(), State(), light_(light)
{
  // XXX: viewport uniform not right during traversal
  fbo_ = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(shadowMapSize,shadowMapSize,GL_NONE));
  fbo_->bind();

  textureQuad_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());

  shadowMapSizeUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowMapSize"));
  shadowMapSizeUniform_->setUniformData((GLfloat)shadowMapSize);
  joinShaderInput(ref_ptr<ShaderInput>::cast(shadowMapSizeUniform_));
  shadowMapSize_ = shadowMapSize;

  setCullFrontFaces(GL_TRUE);
}

void ShadowMap::set_depthTexture(
    const ref_ptr<Texture> &tex,
    GLenum compare, const string &samplerType)
{
  depthTexture_ = tex;
  depthTexture_->bind();
  depthTexture_->set_compare(compare, GL_LEQUAL);
  depthTexture_->set_wrapping(GL_CLAMP_TO_EDGE);
  depthTexture_->set_size(shadowMapSize_, shadowMapSize_);
  depthTexture_->set_filter(GL_LINEAR,GL_LINEAR);
  depthTexture_->texImage();
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture_->id(), 0);

  depthTextureState_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(depthTexture_)));
  depthTextureState_->set_name("shadowDepth");
  depthTextureState_->set_mapping(MAPPING_CUSTOM);
  depthTextureState_->setMapTo(MAP_TO_CUSTOM);
  depthTextureState_->set_samplerType(samplerType);
}
void ShadowMap::set_depthFormat(GLenum f)
{
  depthTexture_->bind();
  depthTexture_->set_internalFormat(f);
  depthTexture_->texImage();
}
void ShadowMap::set_depthType(GLenum t)
{
  depthTexture_->bind();
  depthTexture_->set_pixelType(t);
  depthTexture_->texImage();
}

const ref_ptr<TextureState>& ShadowMap::shadowSampler() const
{
  return depthTextureState_;
}

void ShadowMap::set_shadowMapSize(GLuint shadowMapSize)
{
  depthTexture_->bind();
  depthTexture_->set_size(shadowMapSize, shadowMapSize);
  depthTexture_->texImage();
  fbo_->resize(shadowMapSize,shadowMapSize);

  shadowMapSizeUniform_->setUniformData((GLfloat)shadowMapSize);
  shadowMapSize_ = shadowMapSize;
}
const ref_ptr<ShaderInput1f>& ShadowMap::shadowMapSize() const
{
  return shadowMapSizeUniform_;
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

void ShadowMap::addCaster(const ref_ptr<StateNode> &caster)
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

void ShadowMap::traverse(RenderState *rs)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    traverseTree(rs, it->get());
  }
}

void ShadowMap::glAnimate(GLdouble dt)
{
  update();

  fbo_->bind();
  fbo_->set_viewport();
  enable(&depthRenderState_); {
    computeDepth();
  } disable(&depthRenderState_);

  // filters may require post passes
#if 0
  switch(filteringMode_) {
  case FILTERING_VSM:
    enable(&filteringRenderState_); {
      computeMoment();
    } disable(&filteringRenderState_);
    break;
  default:
    break;
  }
#endif
}
void ShadowMap::animate(GLdouble dt)
{
}
GLboolean ShadowMap::useGLAnimation() const
{
  return GL_TRUE;
}
GLboolean ShadowMap::useAnimation() const
{
  return GL_FALSE;
}
