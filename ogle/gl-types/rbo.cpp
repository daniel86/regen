/*
 * rbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "rbo.h"

RenderBufferObject::RenderBufferObject(GLuint numBuffers)
: RectBufferObject(glGenRenderbuffers, glDeleteRenderbuffers, numBuffers),
  format_(GL_RGBA),
  targetType_(GL_RENDERBUFFER)
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
