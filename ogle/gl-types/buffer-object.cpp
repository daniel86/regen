/*
 * buffer-object.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "buffer-object.h"

BufferObject::BufferObject(
    CreateBufferFunc createBuffers,
    ReleaseBufferFunc releaseBuffers,
    GLuint numBuffers)
: bufferIndex_(0),
  numBuffers_(numBuffers),
  releaseBuffers_(releaseBuffers)
{
  refCount_ = new GLuint;
  *refCount_ = 1;

  ids_ = new GLuint[numBuffers_];
  createBuffers(numBuffers_, ids_);
}
BufferObject::~BufferObject()
{
  unref();
}

void BufferObject::unref()
{
  if(*refCount_ == 1) {
    releaseBuffers_(numBuffers_, ids_);
    delete refCount_;
    delete[] ids_;
  } else {
    *refCount_ -= 1;
  }
}

void BufferObject::setGLResources(BufferObject &other)
{
  unref();
  refCount_ = other.refCount_;
  ids_ = other.ids_;
  numBuffers_ = other.numBuffers_;
  bufferIndex_ = other.bufferIndex_;
  releaseBuffers_ = other.releaseBuffers_;
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
