/*
 * tbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "tbo.h"

TextureBufferObject::TextureBufferObject(GLenum texelFormat)
: Texture()
{
  targetType_ = GL_TEXTURE_BUFFER;
  samplerType_ = "samplerBuffer";
  texelFormat_ = texelFormat;
}

void TextureBufferObject::attach(const ref_ptr<VertexBufferObject> &storage)
{
  attachedVBO_ = storage;
  glTexBuffer(targetType_, texelFormat_, storage->id());
}
void TextureBufferObject::attach(GLuint storage)
{
  attachedVBO_ = ref_ptr<VertexBufferObject>();
  glTexBuffer(targetType_, texelFormat_, storage);
}

void TextureBufferObject::texImage() const
{
}
