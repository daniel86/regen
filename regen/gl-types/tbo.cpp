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
}

void TextureBufferObject::attach(const ref_ptr<VertexBufferObject> &vbo, VBOReference &ref)
{
  attachedVBO_ = vbo;
  attachedVBORef_ = ref;
  glTexBufferRange(
      targetType_,
      texelFormat_,
      ref->bufferID(),
      ref->address(),
      ref->allocatedSize());
}
void TextureBufferObject::attach(GLuint storage)
{
  attachedVBO_ = ref_ptr<VertexBufferObject>();
  attachedVBORef_ = VBOReference();
  glTexBuffer(targetType_, texelFormat_, storage);
}
void TextureBufferObject::attach(GLuint storage, GLuint offset, GLuint size)
{
  attachedVBO_ = ref_ptr<VertexBufferObject>();
  attachedVBORef_ = VBOReference();
  glTexBufferRange(targetType_, texelFormat_, storage, offset, size);
}

void TextureBufferObject::texImage() const
{
}
