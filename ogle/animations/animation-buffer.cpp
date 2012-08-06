/*
 * animation-buffer.cpp
 *
 *  Created on: 21.03.2011
 *      Author: daniel
 */

#include <limits.h>
#include "animation-buffer.h"

AnimationBuffer::AnimationBuffer(GLenum bufferAccess)
: bufferAccess_(bufferAccess),
  mapped_(false),
  bufferDataChanged_(false),
  animationBufferSizeBytes_(0),
  animationVBO_( GL_COPY_WRITE_BUFFER, GL_DYNAMIC_READ )
{
  DEBUG_LOG("creating animation buffer(" << animationVBO_.id() << ")");
}

AnimationBuffer::~AnimationBuffer()
{
}

GLuint AnimationBuffer::numAnimations() const
{
  return animations_.size();
}

bool AnimationBuffer::bufferChanged() const
{
  return data_.bufferChanged();
}

AnimationIterator AnimationBuffer::add(
    VBOAnimation *animation,
    GLuint primitiveBuffer)
{
  animation->set_animationBuffer(this);
  animations_.push_back(animation);
  // include vbo data of primitive in animation buffer
  sizeChanged(primitiveBuffer);
  animation->set_data(&data_);
}

void AnimationBuffer::remove(
    AnimationIterator it,
    GLuint primitiveBuffer)
{
  VBOAnimation *anim = *it;
  animations_.erase(it);
  anim->set_animationBuffer(NULL);
  anim->set_data(NULL);
  // update the animation buffer
  anim->lock(); {
    sizeChanged(primitiveBuffer);
  } anim->unlock();
}

void AnimationBuffer::map()
{
  animationVBO_.bind(GL_ARRAY_BUFFER);

  // map animation buffer data to RAM
  GLvoid *data = animationVBO_.map(0, animationBufferSizeBytes_, bufferAccess_);
  data_.set_data(data);

  mapped_ = true;
}

void AnimationBuffer::unmap()
{
  data_.set_data(NULL);
  mapped_ = false;
  animationVBO_.bind(GL_ARRAY_BUFFER);
  animationVBO_.unmap();
}

void AnimationBuffer::copy(GLuint dst)
{
  data_.set_bufferChanged(false);

  // copy data from animation buffer to primitive data vbo
  GLuint src = animationVBO_.id();
  GLuint numBytesToCopy = animationBufferSizeBytes_;
  GLuint offsetInDstBuffer = data_.offsetInDataBufferToAnimationBufferStart();
  GLuint offsetInSrcBuffer = 0;
  VertexBufferObject::copy(src, dst, numBytesToCopy, offsetInSrcBuffer, offsetInDstBuffer);
}

void AnimationBuffer::lock()
{
  for(list<VBOAnimation*>::iterator
      it = animations_.begin(); it != animations_.end(); ++it)
  {
    while(!(*it)->try_lock());
  }
}

void AnimationBuffer::unlock()
{
  for(list<VBOAnimation*>::iterator
      it = animations_.begin(); it != animations_.end(); ++it)
  {
    (*it)->unlock();
  }
}

void AnimationBuffer::sizeChanged(GLuint primitiveBuffer)
{
  if(animations_.size() == 0) return;

  // offset in bytes to the start of the first attribute in the data vbo
  GLuint newStart = UINT_MAX;
  // offset in bytes to the end of the last attribute in the data vbo
  GLuint newEnd = 0;

  for(list<VBOAnimation*>::iterator
      it = animations_.begin(); it != animations_.end(); ++it)
  {
    VBOAnimation *animation = *it;

    GLuint start = animation->offsetInDataBufferToPrimitiveStart();
    GLuint end = start + animation->primitiveSetBufferSize();

    if(start < newStart) newStart = start;
    if(end > newEnd) newEnd = end;
  }

  lock(); {
    // data must get unmapped, we want to copy to animation buffer
    if(mapped_) unmap();

    if(data_.bufferChanged()) { // copy last update to primitiveBuffer
      copy(primitiveBuffer);
    }
    data_.set_bufferChanged(false);

    unsigned int oldSize = animationBufferSizeBytes_;
    data_.set_offsetInDataBuffer(newStart);
    animationBufferSizeBytes_ = newEnd-newStart;

    if(oldSize!=animationBufferSizeBytes_) {
      // resize buffer
      glBindBuffer(GL_COPY_WRITE_BUFFER, animationVBO_.id());
      glBufferData(GL_COPY_WRITE_BUFFER, animationBufferSizeBytes_, NULL, GL_DYNAMIC_READ);
      glBindBuffer(GL_COPY_WRITE_BUFFER, 0);
    }

    // read data from primitiveBuffer
    GLuint src = primitiveBuffer;
    GLuint dst = animationVBO_.id();
    GLuint numBytesToCopy = animationBufferSizeBytes_;
    GLuint offsetInSrcBuffer = newStart;
    GLuint offsetInDstBuffer = 0;
    VertexBufferObject::copy(src, dst, numBytesToCopy, offsetInSrcBuffer, offsetInDstBuffer);

    // map again to allow animations changing the buffer data
    map();
  } unlock();
}
