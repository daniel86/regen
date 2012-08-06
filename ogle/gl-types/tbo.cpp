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
: Texture(),
  lastStorage_(0)
{
  targetType_ = GL_TEXTURE_BUFFER;
  samplerType_ = samplerType;
  texelFormat_ = texelFormat;
}

void TextureBufferObject::attachStorage(GLuint storage)
{
  lastStorage_ = storage;
  glTexBufferEXT(targetType_, texelFormat_, storage);
}
const GLuint TextureBufferObject::lastStorage() const
{
  return lastStorage_;
}

string TextureBufferObject::samplerType() const
{
  return samplerType_;
}

void TextureBufferObject::texImage() const
{
}
