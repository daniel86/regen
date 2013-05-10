/*
 * vao.cpp
 *
 *  Created on: 07.08.2012
 *      Author: daniel
 */

#include "vao.h"
using namespace regen;

VAO::VAO()
: GLObject(glGenVertexArrays, glDeleteVertexArrays)
{
}
