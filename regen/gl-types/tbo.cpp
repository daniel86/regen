/*
 * tbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "tbo.h"
using namespace regen;

#include <regen/gl-types/gl-util.h>

TextureBufferObject::TextureBufferObject(GLenum texelFormat)
: Texture()
{
  targetType_ = GL_TEXTURE_BUFFER;
  samplerType_ = "samplerBuffer";
  texelFormat_ = texelFormat;
}

void TextureBufferObject::attach(const ref_ptr<VertexBufferObject> &vbo, VBOReference &ref)
{
  attachedVBO_ = vbo;
  attachedVBORef_ = ref;
#ifdef GL_ARB_texture_buffer_range
  glTexBufferRange(
      targetType_,
      texelFormat_,
      ref->bufferID(),
      ref->address(),
      ref->allocatedSize());
#else
  glTexBuffer(targetType_, texelFormat_, ref->bufferID());
#endif
  GL_ERROR_LOG();
}
void TextureBufferObject::attach(GLuint storage)
{
  attachedVBO_ = ref_ptr<VertexBufferObject>();
  attachedVBORef_ = VBOReference();
  glTexBuffer(targetType_, texelFormat_, storage);
  GL_ERROR_LOG();
}
void TextureBufferObject::attach(GLuint storage, GLuint offset, GLuint size)
{
  attachedVBO_ = ref_ptr<VertexBufferObject>();
  attachedVBORef_ = VBOReference();
#ifdef GL_ARB_texture_buffer_range
  glTexBufferRange(targetType_, texelFormat_, storage, offset, size);
#else
  glTexBuffer(targetType_, texelFormat_, storage);
#endif
  GL_ERROR_LOG();
}

void TextureBufferObject::texImage() const
{
}
