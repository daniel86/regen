/*
 * rbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "rbo.h"

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
