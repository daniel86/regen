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

VBOAnimation::VBOAnimation(GLuint destinationBuffer, AttributeState &p)
: Animation(),
  destinationBufferOffset_(0),
  destinationBufferSize_(0),
  snapshot_(),
  snapshotSize_(0),
  animationBuffer_(NULL),
  attributeState_(p),
  destinationBuffer_(destinationBuffer)
{
  list< ref_ptr<VertexAttribute> > *attributes = attributeState_.attributesPtr();
  GLuint attributeOffset;

  // get size and offset of animation buffer.
  // size equals the sum of all attribute sizes of the
  // primitive. The offset equals the minimal offset
  // of an attribute of this primitive.
  if(attributes->size() == 0) {
    destinationBufferOffset_ = 0;
    destinationBufferSize_ = 0;
  } else {
    destinationBufferOffset_ = UINT_MAX;
    destinationBufferSize_ = 0;
    for(list< ref_ptr<VertexAttribute> >::iterator
        it  = attributes->begin(); it != attributes->end(); ++it)
    {
      attributeOffset = (*it)->offset();
      if(destinationBufferOffset_ > attributeOffset) {
        destinationBufferOffset_ = attributeOffset;
      }
      destinationBufferSize_ += (*it)->size();
    }
  }
}

void VBOAnimation::makeSnapshot() {
  if(animationBuffer_==NULL) {
    ERROR_LOG("unable to create snapshot wothout buffer set.");
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
  void *dst = (void*) new GLubyte[destinationBufferSize_];
  GLuint &numBytesToCopy = destinationBufferSize_;
  memcpy(dst, src, numBytesToCopy);

  snapshot_ = ref_ptr<GLfloat>::manage( (GLfloat*) dst );
  snapshotSize_ = destinationBufferSize_;
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

AttributeState& VBOAnimation::attributeState()
{
  return attributeState_;
}

GLuint VBOAnimation::destinationOffset()
{
  return destinationBufferOffset_;
}
GLuint VBOAnimation::destinationSize()
{
  return destinationBufferSize_;
}

GLuint VBOAnimation::destinationBuffer()
{
  return destinationBuffer_;
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
      (*it)->offset() - destinationBufferOffset_ );
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
