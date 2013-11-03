/*
 * buffer-object.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "gl-object.h"
using namespace regen;

GLObject::GLObject(
    CreateObjectFunc createObjects,
    ReleaseObjectFunc releaseObjects,
    GLuint numObjects)
: ids_( new GLuint[numObjects] ),
  numObjects_( numObjects ),
  objectIndex_( 0 ),
  releaseObjects_( releaseObjects )
{
  createObjects(numObjects_, ids_);
}
GLObject::~GLObject()
{
  // XXX: what if the object is currently part of the global GL state?
  //    then another object with same id could be generated and
  //    RenderState would think that the object is active.
  releaseObjects_(numObjects_, ids_);
  delete[] ids_;
}

void GLObject::nextObject()
{ objectIndex_ = (objectIndex_+1) % numObjects_; }
GLuint GLObject::objectIndex() const
{ return objectIndex_; }
void GLObject::set_objectIndex(GLuint bufferIndex)
{ objectIndex_ = bufferIndex % numObjects_; }

GLuint GLObject::numObjects() const
{ return numObjects_; }
GLuint GLObject::id() const
{ return ids_[objectIndex_]; }
GLuint* GLObject::ids() const
{ return ids_; }

/////////////

GLRectangle::GLRectangle(
    CreateObjectFunc createObjects,
    ReleaseObjectFunc releaseObjects,
    GLuint numObjects)
: GLObject(createObjects, releaseObjects, numObjects)
{
}
void GLRectangle::set_rectangleSize(GLuint width, GLuint height)
{
  width_ = width;
  height_ = height;
}

GLuint GLRectangle::width() const
{ return width_; }
GLuint GLRectangle::height() const
{ return height_; }
