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
  bufferSize_(0),
  animationVBO_( VertexBufferObject::USAGE_DYNAMIC, 0 )
{
}

GLboolean AnimationBuffer::isEmpty() const
{
  return animations_.empty();
}

GLboolean AnimationBuffer::bufferChanged() const
{
  for(list<VBOAnimation*>::const_iterator
      it = animations_.begin(); it != animations_.end(); ++it)
  {
    if((*it)->bufferChanged()) { return true; }
  }
  return false;
}

GLboolean AnimationBuffer::mapped() const
{
  return mapped_;
}

AnimationIterator AnimationBuffer::add(
    VBOAnimation *animation,
    GLuint primitiveBuffer)
{
  animation->set_animationBuffer(this);
  animations_.push_back(animation);
  // include vbo data of primitive in animation buffer
  sizeChanged(primitiveBuffer);
  animation->set_data(animationData_, bufferOffset_);
}

void AnimationBuffer::remove(
    AnimationIterator it,
    GLuint primitiveBuffer)
{
  VBOAnimation *anim = *it;
  animations_.erase(it);
  anim->set_animationBuffer(NULL);
  anim->set_data(NULL, 0);
  // update the animation buffer
  anim->lock(); {
    sizeChanged(primitiveBuffer);
  } anim->unlock();
}

void AnimationBuffer::map()
{
  animationVBO_.bind(GL_ARRAY_BUFFER);

  // map animation buffer data to RAM
  animationData_ = animationVBO_.map(
      0, bufferSize_, bufferAccess_);

  mapped_ = true;
}

void AnimationBuffer::unmap()
{
  animationData_ = NULL;
  mapped_ = false;
  animationVBO_.bind(GL_ARRAY_BUFFER);
  animationVBO_.unmap();
}

void AnimationBuffer::copy(GLuint dst)
{
  for(list<VBOAnimation*>::iterator
      it = animations_.begin(); it != animations_.end(); ++it)
  {
    (*it)->set_bufferChanged(false);
  }

  // copy data from animation buffer to primitive data vbo
  GLuint src = animationVBO_.id();
  GLuint numBytesToCopy = bufferSize_;
  GLuint offsetInDstBuffer = bufferOffset_;
  GLuint offsetInSrcBuffer = 0;
  VertexBufferObject::copy(src, dst,
      numBytesToCopy, offsetInSrcBuffer, offsetInDstBuffer);
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

void AnimationBuffer::sizeChanged(GLuint destinationBuffer)
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

    GLuint start = animation->destinationOffset();
    GLuint end = start + animation->destinationSize();

    if(start < newStart) newStart = start;
    if(end > newEnd) newEnd = end;
  }

  GLboolean bufferChanged = false;
  for(list<VBOAnimation*>::iterator
      it = animations_.begin(); it != animations_.end(); ++it)
  {
    if( (*it)->bufferChanged() ) {
      (*it)->set_bufferChanged(false);
      bufferChanged = true;
    }
  }

  // FIXME: i guess this makes problems with free list of vbo
  lock(); {
    // data must get unmapped, we want to copy to animation buffer
    if(mapped_) unmap();

    if(bufferChanged)
    {
      // copy last update to destinationBuffer
      copy(destinationBuffer);
    }

    GLuint oldSize = bufferSize_;
    bufferOffset_ = newStart;
    for(list<VBOAnimation*>::iterator
        it = animations_.begin(); it != animations_.end(); ++it)
    {
      (*it)->set_data(animationData_, bufferOffset_);
    }
    bufferSize_ = newEnd-newStart;

    if(oldSize!=bufferSize_) {
      // resize buffer
      glBindBuffer(GL_COPY_WRITE_BUFFER,
          animationVBO_.id());
      glBufferData(GL_COPY_WRITE_BUFFER,
          bufferSize_, NULL, GL_DYNAMIC_READ);
    }

    // read data from destinationBuffer
    GLuint src = destinationBuffer;
    GLuint dst = animationVBO_.id();
    GLuint numBytesToCopy = bufferSize_;
    GLuint offsetInSrcBuffer = newStart;
    GLuint offsetInDstBuffer = 0;
    VertexBufferObject::copy(src, dst,
        numBytesToCopy,
        offsetInSrcBuffer,
        offsetInDstBuffer);

    // map again to allow animations changing the buffer data
    map();
  } unlock();
}
