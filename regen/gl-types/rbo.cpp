/*
 * rbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "rbo.h"
using namespace regen;

RBO::RBO(GLuint numBuffers)
: GLRectangle(glGenRenderbuffers, glDeleteRenderbuffers, numBuffers),
  targetType_(GL_RENDERBUFFER),
  format_(GL_RGBA)
{
}

void RBO::set_format(GLenum format)
{
  format_ = format;
}
GLenum RBO::targetType() const
{
  return targetType_;
}

void RBO::storageMS(GLuint numMultisamples) const
{
  glRenderbufferStorageMultisample(
      GL_RENDERBUFFER, numMultisamples, format_, width_, height_);
}

void RBO::storage() const
{
  glRenderbufferStorage(
      GL_RENDERBUFFER, format_, width_, height_);
}

void RBO::bind() const
{
  glBindRenderbuffer(targetType_, ids_[objectIndex_]);
}
