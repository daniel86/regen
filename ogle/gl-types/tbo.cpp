/*
 * tbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "tbo.h"

TextureBufferObject::TextureBufferObject(
    GLenum texelFormat,
    const string& samplerType)
: Texture()
{
  targetType_ = GL_TEXTURE_BUFFER_EXT;
  samplerType_ = samplerType;
  texelFormat_ = texelFormat;
}

void TextureBufferObject::attach(ref_ptr<VertexBufferObject> &storage)
{
  attachedVBO_ = storage;
  glTexBuffer(targetType_, texelFormat_, storage->id());
}
void TextureBufferObject::attach(GLuint storage)
{
  attachedVBO_ = ref_ptr<VertexBufferObject>();
  glTexBuffer(targetType_, texelFormat_, storage);
}

string TextureBufferObject::samplerType() const
{
  return samplerType_;
}

void TextureBufferObject::texImage() const
{
}
