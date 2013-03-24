/*
 * vao.cpp
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#include "vao.h"
using namespace regen;

VertexArrayObject::VertexArrayObject()
: BufferObject(glGenVertexArrays, glDeleteVertexArrays)
{
}

void VertexArrayObject::bind() const
{
  glBindVertexArray(id());
}

void VertexArrayObject::unbind() const
{
  glBindVertexArray(0);
}
