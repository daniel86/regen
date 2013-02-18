/*
 * render-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "render-state.h"

#include <ogle/utility/gl-error.h>
#include <ogle/states/state.h>
#include <ogle/meshes/mesh-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/state-node.h>

GLint RenderState::maxTextureUnits_ = -1;

RenderState::RenderState()
: isDepthTestEnabled_(GL_TRUE),
  isDepthWriteEnabled_(GL_TRUE),
  textureCounter_(-1),
  useTransformFeedback_(GL_FALSE)
{
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glClearDepth(1.0f);

  boneWeightCount_ = 0u;
  boneCount_ = 0u;
  if(maxTextureUnits_==-1) {
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &maxTextureUnits_);
  }
  textureArray = new Stack< TextureState* >[maxTextureUnits_];
}

void RenderState::set_isDepthTestEnabled(GLboolean v)
{
  isDepthTestEnabled_ = v;
  if(v) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
}
GLboolean RenderState::isDepthTestEnabled()
{
  return isDepthTestEnabled_;
}

void RenderState::set_isDepthWriteEnabled(GLboolean v)
{
  isDepthWriteEnabled_ = v;
  glDepthMask(v);
}
GLboolean RenderState::isDepthWriteEnabled()
{
  return isDepthWriteEnabled_;
}


GLboolean RenderState::isNodeHidden(StateNode *node)
{
  return node->isHidden();
}
GLboolean RenderState::isStateHidden(State *state)
{
  return state->isHidden();
}

RenderState::~RenderState()
{
  delete[] textureArray;
}

GLboolean RenderState::useTransformFeedback() const
{
  return useTransformFeedback_;
}
void RenderState::set_useTransformFeedback(GLboolean toggle)
{
  useTransformFeedback_ = toggle;
}

void RenderState::pushMesh(MeshState *mesh)
{
  mesh->draw(numInstances());
}
void RenderState::popMesh()
{
}

void RenderState::pushFBO(FrameBufferObject *fbo)
{
  fbos.push(fbo);
  fbo->bind();
  fbo->set_viewport();
}
void RenderState::popFBO()
{
  fbos.pop();
  if(!fbos.isEmpty()) {
    // re-enable FBO from parent node
    FrameBufferObject *parent = fbos.top();
    parent->bind();
    parent->set_viewport();
  }
}

void RenderState::pushShader(Shader *shader)
{
  shaders.push(shader);
  glUseProgram(shader->id());
  shader->uploadInputs();
}
void RenderState::popShader()
{
  shaders.pop();
  if(!shaders.isEmpty()) {
    // re-enable Shader from parent node
    Shader *parent = shaders.top();
    glUseProgram(parent->id());
    parent->uploadInputs();
  }
}

//////////////////////////

GLuint RenderState::nextTexChannel()
{
  if(textureCounter_ < maxTextureUnits_) {
    textureCounter_ += 1;
  }
  return textureCounter_;
}
void RenderState::releaseTexChannel()
{
  Stack< TextureState* > &queue = textureArray[textureCounter_];
  if(queue.isEmpty()) {
    textureCounter_ -= 1;
  }
}

void RenderState::pushTexture(TextureState *tex)
{
  GLuint channel = tex->channel();
  Stack< TextureState* > &queue = textureArray[channel];
  queue.push(tex);

  glActiveTexture(GL_TEXTURE0 + channel);
  tex->texture()->bind();
  if(!shaders.isEmpty()) {
    shaders.top()->uploadTexture(channel, tex->name());
  }
}
void RenderState::popTexture(GLuint channel)
{
  Stack< TextureState* > &queue = textureArray[channel];
  queue.pop();
  if(queue.isEmpty()) { return; }

  glActiveTexture(GL_TEXTURE0 + channel);
  queue.top()->texture()->bind();
  if(!shaders.isEmpty()) {
    TextureState *texState = queue.top();
    shaders.top()->uploadTexture(channel, texState->name());
  }
}

void RenderState::set_bones(GLuint numWeights, GLuint numBones)
{
  boneWeightCount_ = numWeights;
  boneCount_ = numBones;
}

//////////////////////////

void RenderState::pushShaderInput(ShaderInput *in)
{
  if(shaders.isEmpty()) { return; }
  Shader *activeShader = shaders.top();

  inputs_[in->name()].push(in);

  if(in->isVertexAttribute()) {
    activeShader->uploadAttribute(in);
  } else if(!in->isConstant()) {
    activeShader->uploadUniform(in);
  }
}
void RenderState::popShaderInput(const string &name)
{
  if(shaders.isEmpty()) { return; }
  Shader *activeShader = shaders.top();

  Stack<ShaderInput*> &inputStack = inputs_[name];
  if(inputStack.isEmpty()) { return; }
  inputStack.pop();

  // reactivate new top stack member
  if(!inputStack.isEmpty()) {
    ShaderInput *reactivated = inputStack.topPtr();
    // re-apply input
    if(reactivated->isVertexAttribute()) {
      activeShader->uploadAttribute(reactivated);
    } else if(!reactivated->isConstant()) {
      activeShader->uploadUniform(reactivated);
    }
  }
}
GLuint RenderState::numInstances() const
{
  if(shaders.isEmpty()) {
    return 1;
  } else {
    return shaders.top()->numInstances();
  }
}
