/*
 * vbo-animation.cpp
 *
 *  Created on: 21.03.2011
 *      Author: daniel
 */

#include <limits.h>
#include <string.h>

#include "vbo-animation.h"
#include "animation-buffer.h"

VBOAnimation::VBOAnimation(MeshState &p)
: Animation(),
  destinationOffset_(0),
  destinationSize_(0),
  snapshot_(),
  snapshotSize_(0),
  animationBuffer_(NULL),
  attributeState_(p)
{
  list< ref_ptr<VertexAttribute> > *attributes = attributeState_.attributesPtr();
  GLuint attributeOffset;

  // get size and offset of animation buffer.
  // size equals the sum of all attribute sizes of the
  // primitive. The offset equals the minimal offset
  // of an attribute of this primitive.
  if(attributes->empty()) {
    destinationOffset_ = 0;
    destinationSize_ = 0;
  } else {
    destinationOffset_ = UINT_MAX;
    destinationSize_ = 0;
    for(list< ref_ptr<VertexAttribute> >::iterator
        it  = attributes->begin(); it != attributes->end(); ++it)
    {
      attributeOffset = (*it)->offset();
      if(destinationOffset_ > attributeOffset) {
        destinationOffset_ = attributeOffset;
      }
      destinationSize_ += (*it)->size();
    }
  }
}

void VBOAnimation::makeSnapshot() {
  if(animationBuffer_==NULL) {
    ERROR_LOG("unable to create snapshot without buffer set.");
    return;
  }
  clearSnapshot();

  bool isMapped = animationBuffer_->mapped();

  if(!isMapped) {
    animationBuffer_->lock();
    animationBuffer_->map();
  }

  // copy data of mapped animation buffer into dst array
  void *src = ((GLubyte*)animationData_) +
      (destinationOffset() - sourceOffset_);
  void *dst = (void*) new GLubyte[destinationSize_];
  GLuint &numBytesToCopy = destinationSize_;
  memcpy(dst, src, numBytesToCopy);

  snapshot_ = ref_ptr<GLfloat>::manage( (GLfloat*) dst );
  snapshotSize_ = destinationSize_;
  if(!isMapped) {
    animationBuffer_->unmap();
    animationBuffer_->unlock();
  }
}

void VBOAnimation::clearSnapshot() {
  if(snapshot_.get()) {
    snapshot_ = ref_ptr< GLfloat >( );
  }
}

ref_ptr<GLfloat> VBOAnimation::snapshot() {
  return snapshot_;
}

GLuint VBOAnimation::snapshotSize() const {
  return snapshotSize_;
}

GLboolean VBOAnimation::bufferChanged() const
{
  return bufferChanged_;
}
void VBOAnimation::set_bufferChanged(bool v)
{
  bufferChanged_ = v;
}

void VBOAnimation::set_data(void *data, GLuint offset)
{
  animationData_ = data;
  sourceOffset_ = offset;
}

void VBOAnimation::set_animationBuffer(AnimationBuffer *buffer)
{
  animationBuffer_ = buffer;
}

MeshState& VBOAnimation::attributeState()
{
  return attributeState_;
}

GLuint VBOAnimation::destinationOffset()
{
  return destinationOffset_;
}
GLuint VBOAnimation::destinationSize()
{
  return destinationSize_;
}

GLuint VBOAnimation::destinationBuffer()
{
  list< ref_ptr<VertexAttribute> > *attributes = attributeState_.attributesPtr();
  if(attributes->empty()) {
    return 0;
  } else {
    return attributes->front()->buffer();
  }
}

vector<VecXf> VBOAnimation::getFloatAttribute(
    AttributeIteratorConst it)
{
  return getFloatAttribute(
      it,
      (GLfloat*) animationData_,
      (*it)->offset() - sourceOffset_ );
}
vector<VecXf> VBOAnimation::getFloatAttribute(
    AttributeIteratorConst it, GLfloat *vals)
{
  return getFloatAttribute(
      it,
      vals,
      (*it)->offset() - destinationOffset_ );
}
vector<VecXf> VBOAnimation::getFloatAttribute(
    AttributeIteratorConst it, GLfloat *vals, GLuint offset)
{
  vector<VecXf> maps(attributeState_.numVertices());

  // walk through data and collect the requested attribute.
  GLuint j=0;
  for(GLuint i=0; i<(*it)->size(); i+=(*it)->stride())
  {
    maps[j++] = VecXf(
      vals + (i+offset)/sizeof(GLfloat),
      (*it)->valsPerElement() );
  }

  return maps;
}

void VBOAnimation::animate(GLdouble dt)
{
  bufferChanged_ = animateVBO(dt);
}
