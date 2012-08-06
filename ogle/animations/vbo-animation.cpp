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

VBOAnimation::VBOAnimation(GLuint vbo, AttributeState &p)
: Animation(),
  data_(NULL),
  offsetInDataBuffer_(0),
  primitiveSetBufferSize_(0),
  snapshot_(),
  snapshotSize_(0),
  animationBuffer_(NULL),
  mesh_(p),
  primitiveBuffer_(vbo)
{
  list< ref_ptr<VertexAttribute> > *attributes = mesh_.attributesPtr();
  GLuint attributeOffset;

  // get size and offset of animation buffer.
  // size equals the sum of all attribute sizes of the
  // primitive. The offset equals the minimal offset
  // of an attribute of this primitive.
  if(attributes->size() == 0) {
    offsetInDataBuffer_ = 0;
    primitiveSetBufferSize_ = 0;
  } else {
    offsetInDataBuffer_ = UINT_MAX;
    primitiveSetBufferSize_ = 0;
    for(list< ref_ptr<VertexAttribute> >::iterator
        it  = attributes->begin(); it != attributes->end(); ++it)
    {
      attributeOffset = (*it)->offset();
      if(offsetInDataBuffer_ > attributeOffset) {
        offsetInDataBuffer_ = attributeOffset;
      }
      primitiveSetBufferSize_ += (*it)->size();
    }
  }
  DEBUG_LOG("vbo animation(" << this <<
      ") created with offsetInDataBuffer=" << offsetInDataBuffer_
      << " primitiveSetBufferSize=" << primitiveSetBufferSize_);
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
  DEBUG_LOG("    creating snapshot of mapped data"
      << " offsetInDataBufferToAnimationBufferStart=" << data_->offsetInDataBufferToAnimationBufferStart()
      << " offsetInDataBufferToPrimitiveStart=" << offsetInDataBufferToPrimitiveStart()
      << " primitiveSetBufferSize=" << primitiveSetBufferSize_);

  // copy data of mapped animation buffer into dst array
  void *src = ((GLubyte*)data_->data()) + (
      offsetInDataBufferToPrimitiveStart() -
      data_->offsetInDataBufferToAnimationBufferStart());
  void *dst = (void*) new GLubyte[primitiveSetBufferSize_];
  GLuint &numBytesToCopy = primitiveSetBufferSize_;
  memcpy(dst, src, numBytesToCopy);

  snapshot_ = ref_ptr< GLfloat >::manage( (GLfloat*) dst );
  snapshotSize_ = primitiveSetBufferSize_;
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

void VBOAnimation::set_bufferChanged(bool bufferChanged)
{
  data_->set_bufferChanged(bufferChanged);
}
bool VBOAnimation::bufferChanged() const
{
  return data_->bufferChanged();
}

void VBOAnimation::set_data(BufferData *data)
{
  data_ = data;
}
BufferData* VBOAnimation::data()
{
  return data_;
}

void VBOAnimation::set_animationBuffer(AnimationBuffer *buffer)
{
  animationBuffer_ = buffer;
}

GLuint VBOAnimation::offsetInDataBufferToPrimitiveStart()
{
  return offsetInDataBuffer_;
}
GLuint VBOAnimation::primitiveSetBufferSize()
{
  return primitiveSetBufferSize_;
}

GLuint VBOAnimation::primitiveBuffer()
{
  return primitiveBuffer_;
}

vector< VecXf > VBOAnimation::getFloatAttribute(AttributeIteratorConst it)
{
  return getFloatAttribute(it, (float*) data_->data(),
      (*it)->offset() -
      data_->offsetInDataBufferToAnimationBufferStart() );
}
vector< VecXf > VBOAnimation::getFloatAttribute(AttributeIteratorConst it, float *vals)
{
  return getFloatAttribute(it, vals,
      (*it)->offset() -
      offsetInDataBufferToPrimitiveStart() );
}
vector< VecXf > VBOAnimation::getFloatAttribute(AttributeIteratorConst it, float *vals, GLuint offset)
{
  vector< VecXf > maps(mesh_.numVertices());

  // walk through data and collect the requested attribute.
  GLuint j=0;
  for(GLuint i=0; i<(*it)->size(); i+=(*it)->stride())
  {
    maps[j] = ( (VecXf) {
      vals + (i+offset)/sizeof(float),
          (*it)->valsPerElement() }
    );
    j+=1;
  }

  return maps;
}
