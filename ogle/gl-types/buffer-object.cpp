/*
 * buffer-object.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "buffer-object.h"
using namespace ogle;

BufferObject::BufferObject(
    CreateBufferFunc createBuffers,
    ReleaseBufferFunc releaseBuffers,
    GLuint numBuffers)
: ids_( new GLuint[numBuffers] ),
  numBuffers_( numBuffers ),
  bufferIndex_( 0 ),
  releaseBuffers_( releaseBuffers )
{
  createBuffers(numBuffers_, ids_);
}
BufferObject::~BufferObject()
{
  releaseBuffers_(numBuffers_, ids_);
  delete[] ids_;
}

void BufferObject::nextBuffer()
{
  bufferIndex_ = (bufferIndex_+1) % numBuffers_;
}
GLuint BufferObject::bufferIndex() const
{
  return bufferIndex_;
}
void BufferObject::set_bufferIndex(GLuint bufferIndex)
{
  bufferIndex_ = bufferIndex % numBuffers_;
}

GLuint BufferObject::numBuffers() const
{
  return numBuffers_;
}
GLuint BufferObject::id() const
{
  return ids_[bufferIndex_];
}
GLuint* BufferObject::ids() const
{
  return ids_;
}

/////////////

RectBufferObject::RectBufferObject(
    CreateBufferFunc createBuffers,
    ReleaseBufferFunc releaseBuffers,
    GLuint numBuffers)
: BufferObject(createBuffers, releaseBuffers, numBuffers)
{
}
void RectBufferObject::set_size(GLuint width, GLuint height)
{
  width_ = width;
  height_ = height;
}
GLuint RectBufferObject::width() const
{
  return width_;
}
GLuint RectBufferObject::height() const
{
  return height_;
}
