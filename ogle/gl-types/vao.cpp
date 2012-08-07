/*
 * vao.cpp
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#include "vao.h"

VertexArrayObject::VertexArrayObject()
: BufferObject(glGenVertexArrays, glDeleteVertexArrays)
{
}
