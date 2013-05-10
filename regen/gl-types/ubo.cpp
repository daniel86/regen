/*
 * ubo.cpp
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#include "ubo.h"
using namespace regen;

UBO::UBO()
: GLObject(glGenBuffers,glDeleteBuffers),
  blockSize_(0)
{
}

UBO::Layout UBO::layout() const
{
  return layout_;
}

void UBO::set_layout(Layout layout)
{
  layout_ = layout;
}

GLuint UBO::getBlockIndex(GLuint shader, char* blockName) const
{
  return glGetUniformBlockIndex(shader, blockName);
}

void UBO::bindBlock(
    GLuint shader, GLuint blockIndex, GLuint bindingPoint) const
{
  glUniformBlockBinding(shader, blockIndex, bindingPoint);
}

void UBO::setData(byte *data, GLuint size)
{
  blockSize_ = size;
  glBufferData(GL_UNIFORM_BUFFER, size, data, GL_DYNAMIC_DRAW);
}

void UBO::setSubData(byte *data, GLuint offset, GLuint size) const
{
  glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
}
