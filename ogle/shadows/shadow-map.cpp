/*
 * shadow-map.cpp
 *
 *  Created on: 24.11.2012
 *      Author: daniel
 */

#include "shadow-map.h"

#include <ogle/states/polygon-offset-state.h>
#include <ogle/states/cull-state.h>
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

ShadowRenderState::ShadowRenderState(ref_ptr<Texture> texture)
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

/////////////
/////////////

LayeredShadowRenderState::LayeredShadowRenderState(
    ref_ptr<Texture> texture,
    GLuint maxNumBones,
    GLuint numShadowLayer)
: ShadowRenderState(texture),
  numShadowLayer_(numShadowLayer),
  maxNumBones_(maxNumBones)
{
  map<string, string> shaderConfig;
  map<GLenum, string> shaderNames;

  shaderConfig["NUM_BONES"] = FORMAT_STRING(maxNumBones);
  shaderConfig["NUM_LAYER"] = FORMAT_STRING(numShadowLayer_);
  shaderConfig["GLSL_VERSION"] = "400";

  shaderNames[GL_VERTEX_SHADER] = "shadow-mapping.update.vs";
  //shaderNames[GL_TESS_CONTROL_SHADER] = "shadow-mapping.update.tcs";
  //shaderNames[GL_TESS_EVALUATION_SHADER] = "shadow-mapping.update.tes";
  shaderNames[GL_GEOMETRY_SHADER] = "shadow-mapping.update.gs";
  shaderNames[GL_FRAGMENT_SHADER] = "shadow-mapping.update.fs";

  updateShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  updateShader_->createSimple(shaderConfig,shaderNames);
  updateShader_->shader()->compile();
  updateShader_->shader()->link();

  useTesselationLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_useTesselation");

  modelMatLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_modelMatrix");

  numBoneWeightsLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_numBoneWeights");
  boneMatricesLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_boneMatrices");

  viewMatrixLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_viewMatrix");
  ignoreViewRotationLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_ignoreViewRotation");
  ignoreViewTranslationLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_ignoreViewTranslation");

  projectionMatrixLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_projectionMatrix");

  shadowVPMatricesLoc_ = glGetUniformLocation(
      updateShader_->shader()->id(), "in_shadowViewProjectionMatrix");

  posLocation_ = glGetAttribLocation(
      updateShader_->shader()->id(), "vs_pos");
  boneWeightsLocation_ = glGetAttribLocation(
      updateShader_->shader()->id(), "vs_boneWeights");
  boneIndicesLocation_ = glGetAttribLocation(
      updateShader_->shader()->id(), "vs_boneIndices");
}

void LayeredShadowRenderState::enable()
{
  ShadowRenderState::enable();
  glUseProgram(updateShader_->shader()->id());
  updateShader_->shader()->uploadInputs();
}

void LayeredShadowRenderState::set_shadowViewProjectionMatrices(Mat4f *mat) {
  glUniformMatrix4fv(shadowVPMatricesLoc_,
      numShadowLayer_, GL_FALSE, (GLfloat*)mat->x);
}

void LayeredShadowRenderState::set_modelMat(Mat4f *mat) {
  if(mat==NULL) {
    glUniformMatrix4fv(modelMatLoc_, 1, GL_FALSE, identity4f().x);
  } else {
    glUniformMatrix4fv(modelMatLoc_, 1, GL_FALSE, (GLfloat*)mat->x);
  }
}
void LayeredShadowRenderState::set_boneMatrices(Mat4f *mat, GLuint numWeights, GLuint numBones) {
  if(mat==NULL) {
    glUniform1i(numBoneWeightsLoc_, 0);
  } else {
    glUniform1i(numBoneWeightsLoc_, numWeights);
    glUniformMatrix4fv(boneMatricesLoc_,
        numBones, GL_FALSE, (GLfloat*)mat->x);
  }
}

void LayeredShadowRenderState::set_viewMatrix(Mat4f *mat) {
  if(mat!=NULL) {
    glUniformMatrix4fv(viewMatrixLoc_,
        numShadowLayer_, GL_FALSE, (GLfloat*)mat->x);
  }
}
void LayeredShadowRenderState::set_ignoreViewRotation(GLboolean v) {
  glUniform1i(ignoreViewRotationLoc_, v);
}
void LayeredShadowRenderState::set_ignoreViewTranslation(GLboolean v) {
  glUniform1i(ignoreViewTranslationLoc_, v);
}

void LayeredShadowRenderState::set_projectionMatrix(Mat4f *mat) {
  if(mat!=NULL) {
    glUniformMatrix4fv(projectionMatrixLoc_,
        numShadowLayer_, GL_FALSE, (GLfloat*)mat->x);
  }
}

void LayeredShadowRenderState::set_useTesselation(GLboolean v) {
  glUniform1i(useTesselationLoc_, v);
}

void LayeredShadowRenderState::pushShaderInput(ShaderInput *att) {
  if(att->isVertexAttribute()) {
    if(att->name()=="pos") {
      att->enableAttribute(posLocation_);
    } else if(att->name()=="boneWeights") {
      att->enableAttribute(boneWeightsLocation_);
    } else if(att->name()=="boneIndices") {
      att->enableAttribute(boneIndicesLocation_);
    }
  }
}

///////////
//////////


Mat4f ShadowMap::biasMatrix_ = Mat4f(
  0.5, 0.0, 0.0, 0.0,
  0.0, 0.5, 0.0, 0.0,
  0.0, 0.0, 0.5, 0.0,
  0.5, 0.5, 0.5, 1.0 );

ShadowMap::ShadowMap(ref_ptr<Light> light, ref_ptr<Texture> texture)
: Animation(), State(), light_(light), texture_(texture)
{
  //setCullFrontFaces(GL_TRUE);
  setPolygonOffset();

  texture_->bind();
  texture_->set_filter(GL_NEAREST,GL_NEAREST);
  texture_->set_wrapping(GL_CLAMP_TO_EDGE);
  texture_->set_compare(GL_COMPARE_R_TO_TEXTURE, GL_LEQUAL);

  shadowMap_ = ref_ptr<TextureState>::manage(
      new TextureState(ref_ptr<Texture>::cast(texture_)));
  shadowMap_->set_name(FORMAT_STRING("shadowMap"<<light_->id()));
  shadowMap_->set_mapping(MAPPING_CUSTOM);
  shadowMap_->setMapTo(MAP_TO_CUSTOM);

  light_->joinStates(ref_ptr<State>::cast(shadowMap()));
  light_->shaderDefine(
      FORMAT_STRING("LIGHT"<<light_->id()<<"_HAS_SM"), "TRUE");
}

ref_ptr<TextureState>& ShadowMap::shadowMap()
{
  return shadowMap_;
}

void ShadowMap::set_shadowMapSize(GLuint shadowMapSize)
{
  texture_->bind();
  texture_->set_size(shadowMapSize, shadowMapSize);
  texture_->texImage();
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

void ShadowMap::traverse(RenderState *rs)
{
  for(list< ref_ptr<StateNode> >::iterator
      it=caster_.begin(); it!=caster_.end(); ++it)
  {
    traverseTree(rs, it->get());
  }
}

void ShadowMap::animate(GLdouble dt)
{
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
    shaderNames[GL_VERTEX_SHADER] = "shadow-mapping.debug.vs";
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
