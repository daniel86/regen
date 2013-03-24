/*
 * rbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "rbo.h"
using namespace ogle;

RenderBufferObject::RenderBufferObject(GLuint numBuffers)
: RectBufferObject(glGenRenderbuffers, glDeleteRenderbuffers, numBuffers),
  targetType_(GL_RENDERBUFFER),
  format_(GL_RGBA)
{
}

void RenderBufferObject::set_format(GLenum format)
{
  format_ = format;
}
GLenum RenderBufferObject::targetType() const
{
  return targetType_;
}

void RenderBufferObject::storageMS(GLuint numMultisamples) const
{
  glRenderbufferStorageMultisample(
      GL_RENDERBUFFER, numMultisamples, format_, width_, height_);
}

void RenderBufferObject::storage() const
{
  glRenderbufferStorage(
      GL_RENDERBUFFER, format_, width_, height_);
}

void RenderBufferObject::bind() const
{
  glBindRenderbuffer(targetType_, ids_[bufferIndex_]);
}
