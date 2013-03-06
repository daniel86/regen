/*
 * vao.cpp
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#include "vao.h"
using namespace ogle;

VertexArrayObject::VertexArrayObject()
: BufferObject(glGenVertexArrays, glDeleteVertexArrays)
{
}

void VertexArrayObject::bind() const
{
  glBindVertexArray(id());
}

void VertexArrayObject::bindZero() const
{
  glBindVertexArray(0);
}
