/*
 * render-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "render-state.h"

RenderState::RenderState()
{
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &maxTextureUnits_);
  textureArray = new Stack<ShaderTexture>[maxTextureUnits_];
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
  for(list<Uniform*>::const_iterator
      it=uniforms.begin(); it!=uniforms.end(); ++it)
  {
    shader->applyUniform(*it);
  }
  for(map< string, Stack<VertexAttribute*> >::iterator
      it=attributes.begin(); it!=attributes.end(); ++it)
  {
    shader->applyAttribute(it->second.top());
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
    for(list<Uniform*>::const_iterator
        it=uniforms.begin(); it!=uniforms.end(); ++it)
    {
      parent->applyUniform(*it);
    }
    for(map< string, Stack<VertexAttribute*> >::iterator
        it=attributes.begin(); it!=attributes.end(); ++it)
    {
      parent->applyAttribute(it->second.top());
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

void RenderState::pushUniform(Uniform *uniform)
{
  uniforms.push_back(uniform);
  if(!shaders.isEmpty()) {
    shaders.top()->applyUniform(uniform);
  }
}
void RenderState::popUniform()
{
  uniforms.pop_back();
  if(uniforms.size()>0 && !shaders.isEmpty()) {
    shaders.top()->applyUniform(uniforms.back());
  }
}

//////////////////////////

void RenderState::pushAttribute(VertexAttribute *att)
{
  Stack<VertexAttribute*> &attributeStack = attributes[att->name()];
  if(!attributeStack.isEmpty() && attributeStack.top()->numInstances()>1)
  {
    numInstancedAttributes_ -= 1;
    if(numInstancedAttributes_==0) {
      numInstances_ = 1;
    }
  }
  attributeStack.push(att);
  if(!shaders.isEmpty()) {
    shaders.top()->applyAttribute(att);
  }
  if(att->numInstances()>1)
  {
    numInstances_ = att->numInstances();
    numInstancedAttributes_ += 1;
  }
}
void RenderState::popAttribute(const string &name)
{
  Stack<VertexAttribute*> &attribute = attributes[name];
  if(attribute.top()->numInstances()>1)
  {
    numInstancedAttributes_ -= 1;
    if(numInstancedAttributes_==0) {
      numInstances_ = 1;
    }
  }
  attribute.pop();
  if(!attribute.isEmpty()) {
    if(attribute.top()->numInstances()>1)
    {
      numInstancedAttributes_ += 1;
      numInstances_ = attribute.top()->numInstances();
    }
    if(!shaders.isEmpty()) {
      // if there is a parent attribute with same name
      // and a shder parent, then apply the parent
      // attribute for the activated shader.
      shaders.top()->applyAttribute(attribute.top());
    }
  }
}
GLuint RenderState::numInstances() const
{
  return numInstances_;
}
