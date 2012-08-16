/*
 * render-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "render-state.h"

#include <ogle/utility/gl-error.h>

RenderState::RenderState()
: numInstances_(1),
  numInstancedAttributes_(0),
  textureCounter_(-1)
{
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &maxTextureUnits_);
  textureArray = new Stack<ShaderTexture>[maxTextureUnits_];
  DEBUG_LOG("GL_MAX_TEXTURE_IMAGE_UNITS_ARB=" << maxTextureUnits_);
}

RenderState::~RenderState()
{
  delete[] textureArray;
}

void RenderState::pushVBO(VertexBufferObject *vbo)
{
  vbos.push(vbo);
  vbo->bind(GL_ARRAY_BUFFER);
  vbo->bind(GL_ELEMENT_ARRAY_BUFFER);
}
void RenderState::popVBO()
{
  vbos.pop();
  if(!vbos.isEmpty()) {
    // re-enable VBO from parent node
    VertexBufferObject *parent = vbos.top();
    parent->bind(GL_ARRAY_BUFFER);
    parent->bind(GL_ELEMENT_ARRAY_BUFFER);
  }
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
  // apply uniforms and attributes from parent nodes.
  for(list<ShaderInput*>::const_iterator
      it=uniforms_.begin(); it!=uniforms_.end(); ++it)
  {
    shader->applyUniform(*it);
  }
  for(list<ShaderInput*>::const_iterator
      it=attributes_.begin(); it!=attributes_.end(); ++it)
  {
    shader->applyAttribute(*it);
  }
  for(set< Stack<ShaderTexture>* >::const_iterator
      it=activeTextures.begin(); it!=activeTextures.end(); ++it)
  {
    shader->applyTexture((*it)->top());
  }
}
void RenderState::popShader()
{
  shaders.pop();
  if(!shaders.isEmpty()) {
    // re-enable Shader from parent node
    Shader *parent = shaders.top();
    glUseProgram(parent->id());
    // also re-apply uniforms and attributes from parent nodes.
    for(list<ShaderInput*>::const_iterator
        it=uniforms_.begin(); it!=uniforms_.end(); ++it)
    {
      parent->applyUniform(*it);
    }
    for(list<ShaderInput*>::const_iterator
        it=attributes_.begin(); it!=attributes_.end(); ++it)
    {
      parent->applyAttribute(*it);
    }
    for(set< Stack<ShaderTexture>* >::const_iterator
        it=activeTextures.begin(); it!=activeTextures.end(); ++it)
    {
      parent->applyTexture((*it)->top());
    }
  }
}

//////////////////////////

GLuint RenderState::nextTextureUnit()
{
  if(textureCounter_ < maxTextureUnits_) {
    textureCounter_ += 1;
  }
  return textureCounter_;
}

void RenderState::releaseTextureUnit()
{
  Stack<ShaderTexture> &queue = textureArray[textureCounter_];
  if(queue.isEmpty()) {
    textureCounter_ -= 1;
  }
}

void RenderState::pushTexture(GLuint unit, Texture *tex)
{
  Stack<ShaderTexture> &queue = textureArray[unit];
  queue.push(ShaderTexture(tex,unit));
  activeTextures.insert(&queue);

  glActiveTexture(GL_TEXTURE0 + unit);
  tex->bind();
  if(!shaders.isEmpty()) {
    shaders.top()->applyTexture(ShaderTexture(tex, unit));
  }
}
void RenderState::popTexture(GLuint unit)
{
  Stack<ShaderTexture> &queue = textureArray[unit];
  queue.pop();

  if(!queue.isEmpty()) {
    glActiveTexture(GL_TEXTURE0 + unit);
    queue.top().tex->bind();
    if(!shaders.isEmpty()) {
      shaders.top()->applyTexture(queue.top());
    }
  } else {
    activeTextures.erase(&queue);
  }
}

//////////////////////////

void RenderState::pushShaderInput(ShaderInput *in)
{
  Stack<ShaderInputData> &inputStack = inputs_[in->name()];
  if(!inputStack.isEmpty())
  {
    // overwriting previously defined input.
    // this could also overwrite the type. For example a parent
    // uniform could be overwritten to be an instanced attribute
    // for a subtree
    ShaderInputData &parent = inputStack.topPtr();
    parent.inList.erase(parent.inIterator);

    // remember num instances for the draw call
    if(parent.in->numInstances()>1) {
      numInstancedAttributes_ -= 1;
      if(numInstancedAttributes_==0) {
        numInstances_ = 1;
      }
    }
  }

  list<ShaderInput*> *inList;
  if(in->numInstances()>1)
  {
    // instanced attribute
    attributes_.push_front(in);
    inList = &attributes_;

    // remember num instances for the draw call
    numInstances_ = in->numInstances();
    numInstancedAttributes_ += 1;

    if(!shaders.isEmpty()) { shaders.top()->applyAttribute(in); }
  }
  else if(in->numVertices()>1)
  {
    // per vertex attribute
    attributes_.push_front(in);
    inList = &attributes_;

    if(!shaders.isEmpty()) { shaders.top()->applyAttribute(in); }
  }
  else if(!in->isConstant())
  {
    // uniform
    uniforms_.push_front(in);
    inList = &uniforms_;

    if(!shaders.isEmpty()) { shaders.top()->applyUniform(in); }
  }
  else
  {
    constants_.push_front(in);
    inList = &constants_;
  }

  inputStack.push(ShaderInputData(in,*inList,inList->begin()));
}
void RenderState::popShaderInput(const string &name)
{
  Stack<ShaderInputData> &inputStack = inputs_[name];

  { // pop the top element
    ShaderInputData &popped = inputStack.topPtr();

    // remember num instances for the draw call
    if(popped.in->numInstances()>1) {
      numInstancedAttributes_ -= 1;
      if(numInstancedAttributes_==0) {
        numInstances_ = 1;
      }
    }

    popped.inList.erase(popped.inIterator);
    inputStack.pop();
  }

  // reactivate new top stack member
  if(!inputStack.isEmpty())
  {
    ShaderInputData &reactivated = inputStack.topPtr();
    reactivated.inList.push_front(reactivated.in);
    reactivated.inIterator = reactivated.inList.begin();

    if(reactivated.in->numInstances()>1)
    {
      // remember num instances for the draw call
      numInstancedAttributes_ += 1;
      numInstances_ = reactivated.in->numInstances();
      // re-apply the instanced attribute
      if(!shaders.isEmpty()) { shaders.top()->applyAttribute(reactivated.in); }
    } else if(!shaders.isEmpty()) {
      // re-apply input
      if(reactivated.in->numVertices()>1) {
        shaders.top()->applyAttribute(reactivated.in);
      } else if(!reactivated.in->isConstant()) {
        shaders.top()->applyUniform(reactivated.in);
      }
    }
  }
}
GLuint RenderState::numInstances() const
{
  return numInstances_;
}
