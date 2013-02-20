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
#include <ogle/states/shader-configurer.h>
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/shadows/directional-shadow-map.h>

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
  // TODO: downsample before blurring aka downsample twice
  // XXX: no inverse matrices provided
  // XXX: viewport uniform not right during traversal
  fbo_ = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(shadowMapSize,shadowMapSize,GL_NONE));

  textureQuad_ = ref_ptr<MeshState>::cast(Rectangle::getUnitQuad());

  shadowMapSizeUniform_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowMapSize"));
  shadowMapSizeUniform_->setUniformData((GLfloat)shadowMapSize);
  joinShaderInput(ref_ptr<ShaderInput>::cast(shadowMapSizeUniform_));
  shadowMapSize_ = shadowMapSize;

  setCullFrontFaces(GL_TRUE);
}

void ShadowMap::set_depthTexture(
    const ref_ptr<Texture> &tex, const string &samplerType)
{
  fbo_->bind();
  depthTexture_ = tex;
  depthTexture_->bind();
  depthTexture_->set_wrapping(GL_CLAMP_TO_EDGE);
  depthTexture_->set_size(shadowMapSize_, shadowMapSize_);
  depthTexture_->set_filter(GL_LINEAR,GL_LINEAR);
  depthTexture_->texImage();
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthTexture_->id(), 0);

  depthTextureState_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(depthTexture_)));
  depthTextureState_->set_name("shadowTexture");
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

void ShadowMap::createMomentsTexture()
{
  if(momentsTexture_.get()) { return; }

  fbo_->bind();

  string samplerTypeName;
  switch(samplerType()) {
  case GL_TEXTURE_CUBE_MAP:
    samplerTypeName = "samplerCube";
    momentsTexture_ = ref_ptr<Texture>::manage(new TextureCube);
    break;
  case GL_TEXTURE_2D_ARRAY: {
    samplerTypeName = "sampler2DArray";
    momentsTexture_ = ref_ptr<Texture>::manage(new Texture2DArray);
    Texture2DArray *tex = (Texture2DArray*) momentsTexture_.get();
    tex->set_depth(DirectionalShadowMap::numSplits());
    break;
  }
  default:
    samplerTypeName = "sampler2D";
    momentsTexture_ = ref_ptr<Texture>::manage(new Texture2D);
    break;
  }
  momentsTexture_->set_size(shadowMapSize_, shadowMapSize_);
  momentsTexture_->set_format(GL_RGBA);
  momentsTexture_->set_internalFormat(GL_RGBA16F);
  momentsTexture_->bind();
  momentsTexture_->set_wrapping(GL_CLAMP_TO_EDGE);
  momentsTexture_->set_filter(GL_LINEAR, GL_LINEAR);
  momentsTexture_->texImage();
  momentsAttachment_ = fbo_->addColorAttachment(*momentsTexture_.get());

  momentsTextureState_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(momentsTexture_)));
  momentsTextureState_->set_name("shadowTexture");
  momentsTextureState_->set_mapping(MAPPING_CUSTOM);
  momentsTextureState_->setMapTo(MAP_TO_CUSTOM);
  momentsTextureState_->set_samplerType(samplerTypeName);
}

void ShadowMap::set_computeMoments()
{
  if(momentsCompute_.get()) { return; }

  createMomentsTexture();

  momentsCompute_ = ref_ptr<ShaderState>::manage(new ShaderState);
  ShaderConfigurer cfg;
  cfg.addState(depthTextureState_.get());
  cfg.addState(textureQuad_.get());
  switch(samplerType()) {
  case GL_TEXTURE_CUBE_MAP:
    cfg.define("IS_CUBE_SHADOW", "TRUE");
    break;
  case GL_TEXTURE_2D_ARRAY:
    cfg.define("IS_ARRAY_SHADOW", "TRUE");
    break;
  default:
    cfg.define("IS_2D_SHADOW", "TRUE");
    break;
  }
  momentsCompute_->createShader(cfg.cfg(), "shadow_mapping.moments");
  momentsLayer_ = momentsCompute_->shader()->uniformLocation("shadowLayer");
}

void ShadowMap::set_useMomentBlurFilter()
{
  if(!momentsTexture_.get()) { set_computeMoments(); }
  if(momentsBlur_.get()) { return; }

  // XXX blur cube/array

  momentsBlurScale_ = 0.5;
  GLfloat momentsBlurSize = shadowMapSize_*momentsBlurScale_;
  momentsBlur_ = ref_ptr<BlurState>::manage(new BlurState(
      momentsTexture_, Vec2ui(momentsBlurSize)));
  momentsBlur_->set_sigma(2.5f);
  momentsBlur_->set_numPixels(4.0f);

  ShaderConfigurer shaderConfigurer;
  shaderConfigurer.addState(momentsBlur_.get());
  momentsBlur_->createShader(shaderConfigurer.cfg());

  // use blur result for sampling
  momentsTextureState_->set_texture(momentsBlur_->blurTexture());
  shadowMapSizeUniform_->setUniformData(momentsBlurSize);
}

const ref_ptr<TextureState>& ShadowMap::shadowDepth() const
{
  return depthTextureState_;
}
const ref_ptr<TextureState>& ShadowMap::shadowMoments() const
{
  return momentsTextureState_;
}
const ref_ptr<BlurState>& ShadowMap::momentsBlur() const
{
  return momentsBlur_;
}

void ShadowMap::set_shadowMapSize(GLuint shadowMapSize)
{
  shadowMapSizeUniform_->setUniformData((GLfloat)shadowMapSize);
  shadowMapSize_ = shadowMapSize;

  depthTexture_->bind();
  depthTexture_->set_size(shadowMapSize, shadowMapSize);
  depthTexture_->texImage();
  fbo_->resize(shadowMapSize,shadowMapSize);

  if(momentsTexture_.get()) {
    momentsTexture_->bind();
    momentsTexture_->set_size(shadowMapSize, shadowMapSize);
    momentsTexture_->texImage();
  }
  if(momentsBlur_.get()) {
    GLfloat momentsBlurSize = shadowMapSize*momentsBlurScale_;
    momentsBlur_->framebuffer()->resize(momentsBlurSize, momentsBlurSize);
    shadowMapSizeUniform_->setUniformData(momentsBlurSize);
  }

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

  glDrawBuffer(GL_NONE);
  glClear(GL_DEPTH_BUFFER_BIT);
  enable(&depthRenderState_);
  computeDepth();
  disable(&depthRenderState_);

  // compute moments from depth texture
  if(momentsTexture_.get()) {
    GLint *channel = depthTextureState_->channelPtr();
    *channel = filteringRenderState_.nextTexChannel();
    depthTexture_->activateBind(*channel);
    depthTexture_->set_compare(GL_NONE, GL_LEQUAL);

    glDrawBuffer(momentsAttachment_);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    computeMoment();
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    depthTexture_->bind();
    depthTexture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);
    filteringRenderState_.releaseTexChannel();
  }
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
