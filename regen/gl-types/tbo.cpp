/*
 * tbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "tbo.h"
using namespace regen;

TextureBufferObject::TextureBufferObject(GLenum texelFormat)
: Texture()
{
  targetType_ = GL_TEXTURE_BUFFER;
  samplerType_ = "samplerBuffer";
  texelFormat_ = texelFormat;
}
TextureBufferObject::~TextureBufferObject()
{
  if(attachedVBO_.get()) {
    attachedVBO_->free(attachedVBORef_);
  }
}

void TextureBufferObject::attach(
    const ref_ptr<VertexBufferObject> &storage,
    VertexBufferObject::Reference ref)
{
  // XXX: use glTexBufferRange instead to allow VBO pools!
  if(attachedVBO_.get()) {
    attachedVBO_->free(ref);
  }
  attachedVBO_ = storage;
  attachedVBORef_ = ref;
  glTexBuffer(targetType_, texelFormat_, storage->id());
}
void TextureBufferObject::attach(GLuint storage)
{
  if(attachedVBO_.get()) {
    attachedVBO_->free(attachedVBORef_);
  }
  attachedVBO_ = ref_ptr<VertexBufferObject>();
  glTexBuffer(targetType_, texelFormat_, storage);
}

void TextureBufferObject::texImage() const
{
}
