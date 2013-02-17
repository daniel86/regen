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
#include <ogle/utility/string-util.h>
#include <ogle/utility/gl-error.h>

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

ShadowRenderState::ShadowRenderState(const ref_ptr<Texture> &texture)
: RenderState(),
  texture_(texture)
{
  // create depth only render target for updating the shadow maps
  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture_->id(), 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLboolean ShadowRenderState::isStateHidden(State *state)
{
  return (
      dynamic_cast<DepthState*>(state)!=NULL ||
      dynamic_cast<CullDisableState*>(state)!=NULL ||
      dynamic_cast<CullEnableState*>(state)!=NULL ||
      state->isHidden());
}

void ShadowRenderState::enable()
{
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, texture_->width(), texture_->height());
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void ShadowRenderState::pushTexture(TextureState *tex) {
  // only textures mapped to geometry must be added
  switch(tex->mapTo()) {
  case MAP_TO_CUSTOM:
  case MAP_TO_HEIGHT:
  case MAP_TO_DISPLACEMENT:
    RenderState::pushTexture(tex);
    break;
  default:
    break;
  }
}

///////////
//////////


Mat4f ShadowMap::biasMatrix_ = Mat4f(
  0.5, 0.0, 0.0, 0.0,
  0.0, 0.5, 0.0, 0.0,
  0.0, 0.0, 0.5, 0.0,
  0.5, 0.5, 0.5, 1.0 );

ShadowMap::ShadowMap(const ref_ptr<Light> &light, const ref_ptr<Texture> &texture)
: Animation(), State(), light_(light), texture_(texture)
{
  texture_->bind();
  texture_->set_filter(GL_NEAREST,GL_NEAREST);
  texture_->set_wrapping(GL_CLAMP_TO_EDGE);
  texture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);

  shadowMap_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(texture_)));
  shadowMap_->set_name("shadowMap");
  shadowMap_->set_mapping(MAPPING_CUSTOM);
  shadowMap_->setMapTo(MAP_TO_CUSTOM);

  shadowMapSize_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("shadowMapSize"));
  shadowMapSize_->setUniformData(512.0f);

  // avoid shadow acne
  setCullFrontFaces(GL_TRUE);
  //setPolygonOffset(1.1, 4096.0);
}

const ref_ptr<TextureState>& ShadowMap::shadowMap() const
{
  return shadowMap_;
}

void ShadowMap::set_shadowMapSize(GLuint shadowMapSize)
{
  texture_->bind();
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->texImage();
  shadowMapSize_->setUniformData((float)shadowMapSize);
}
const ref_ptr<ShaderInput1f>& ShadowMap::shadowMapSize() const
{
  return shadowMapSize_;
}

void ShadowMap::set_internalFormat(GLenum internalFormat)
{
  texture_->bind();
  texture_->set_internalFormat(internalFormat);
  texture_->texImage();
}
void ShadowMap::set_pixelType(GLenum pixelType)
{
  texture_->bind();
  texture_->set_pixelType(pixelType);
  texture_->texImage();
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

void ShadowMap::animate(GLdouble dt){}
GLboolean ShadowMap::useGLAnimation() const {
  return GL_TRUE;
}
GLboolean ShadowMap::useAnimation() const {
  return GL_FALSE;
}

void ShadowMap::drawDebugHUD(
    GLenum textureTarget,
    GLenum textureCompareMode,
    GLuint numTextures,
    GLuint textureID,
    const string &fragmentShader)
{
  if(debugShader_.get() == NULL) {
    debugShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
    map<string, string> shaderConfig;
    map<GLenum, string> shaderNames;
    shaderNames[GL_FRAGMENT_SHADER] = fragmentShader;
    shaderNames[GL_VERTEX_SHADER] = "shadow_mapping.debug.vs";
    debugShader_->createSimple(shaderConfig,shaderNames);
    debugShader_->shader()->compile();
    debugShader_->shader()->link();

    debugLayerLoc_ = glGetUniformLocation(debugShader_->shader()->id(), "in_shadowLayer");
    debugTextureLoc_ = glGetUniformLocation(debugShader_->shader()->id(), "in_shadowMap");
  }

  glDisable(GL_DEPTH_TEST);
  glUseProgram(debugShader_->shader()->id());
  glUniform1i(debugTextureLoc_, 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(textureTarget, textureID);
  glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_MODE, GL_NONE);

  for(GLuint i=0; i<numTextures; ++i) {
    glViewport(130*i, 0, 128, 128);
    glUniform1f(debugLayerLoc_, float(i));

    glBegin(GL_QUADS);
    glVertex3f(-1.0, -1.0, 0.0);
    glVertex3f( 1.0, -1.0, 0.0);
    glVertex3f( 1.0,  1.0, 0.0);
    glVertex3f(-1.0,  1.0, 0.0);
    glEnd();
  }

  // reset states
  glTexParameteri(textureTarget, GL_TEXTURE_COMPARE_MODE, textureCompareMode);
  glEnable(GL_DEPTH_TEST);
}
