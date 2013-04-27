/*
 * vbo-interleaved.cpp
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#include <stdlib.h>
#include <cstdio>
#include <cstring>

#include <regen/utility/logging.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/vertex-attribute.h>

#include "vbo.h"
using namespace regen;

/////////////////////
/////////////////////

VertexBufferObject::VBOPool VertexBufferObject::staticDataPool_;
VertexBufferObject::VBOPool VertexBufferObject::dynamicDataPool_;
VertexBufferObject::VBOPool VertexBufferObject::streamDataPool_;
VertexBufferObject::Usage VertexBufferObject::defaultUsage_=USAGE_DYNAMIC;

void VertexBufferObject::set_defaultUsage(VertexBufferObject::Usage v)
{ defaultUsage_ = v; }

VertexBufferObject::VBOPool* VertexBufferObject::allocatorPool(Usage bufferUsage)
{
  switch(bufferUsage) {
  case USAGE_DYNAMIC: return &dynamicDataPool_;
  case USAGE_STATIC:  return &staticDataPool_;
  case USAGE_STREAM:  return &streamDataPool_;
  }
  return &dynamicDataPool_;
}

VertexBufferObject::VBOPool::Node* VertexBufferObject::getAllocator(GLuint size, Usage bufferUsage)
{
  VBOPool *pool = allocatorPool(bufferUsage);

  VBOPool::Node *n = pool->chooseAllocator(size);
  if(n==NULL) {
    n = pool->createAllocator(size);
    // XXX: VBO pool howto delete destructor makes no sense
    //autoAllocatedBuffers_.insert(new VertexBufferObject(usage,n));
  }

  return n;
}

VertexBufferObject::Reference VertexBufferObject::allocateInterleaved(
    const list< ref_ptr<VertexAttribute> > &attributes, Usage bufferUsage)
{
  VBOPool::Node *n = getAllocator(attributeSize(attributes), bufferUsage);
  VertexBufferObject *vbo = (VertexBufferObject*)n->userData;

  return vbo->allocateInterleaved(attributes);
}

VertexBufferObject::Reference VertexBufferObject::allocateSequential(
    const list< ref_ptr<VertexAttribute> > &attributes, Usage bufferUsage)
{
  VBOPool::Node *n = getAllocator(attributeSize(attributes), bufferUsage);
  VertexBufferObject *vbo = (VertexBufferObject*)n->userData;

  return vbo->allocateSequential(attributes);
}
VertexBufferObject::Reference VertexBufferObject::allocateSequential(
    const ref_ptr<VertexAttribute> &att, Usage bufferUsage)
{
  list< ref_ptr<VertexAttribute> > atts;
  atts.push_back(att);
  return allocateSequential(atts,bufferUsage);
}

GLuint VertexBufferObject::attributeSize(
    const list< ref_ptr<VertexAttribute> > &attributes)
{
  if(attributes.size()>0) {
    GLuint structSize = 0;
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        it = attributes.begin(); it != attributes.end(); ++it)
    {
      structSize += (*it)->size();
    }
    return structSize;
  }
  return 0;
}

void VertexBufferObject::free(VertexBufferObject::Reference &jt)
{
  VBOPool::Node *n=jt.allocatorNode;
  n->pool->free(jt);
}

void VertexBufferObject::copy(
    GLuint from,
    GLuint to,
    GLuint size,
    GLuint offset,
    GLuint toOffset)
{
  RenderState *rs = RenderState::get();
  rs->copyReadBuffer().push(from);
  rs->copyWriteBuffer().push(to);
  glCopyBufferSubData(
      GL_COPY_READ_BUFFER,
      GL_COPY_WRITE_BUFFER,
      offset,
      toOffset,
      size);
  rs->copyReadBuffer().pop();
  rs->copyWriteBuffer().pop();
}

/////////////////////
/////////////////////

VertexBufferObject::VertexBufferObject(
    Usage usage, GLuint bufferSize)
: BufferObject(glGenBuffers,glDeleteBuffers),
  usage_(usage),
  bufferSize_(bufferSize)
{
  // TODO: if FREE allocator with enough space is there use it instead of creating a new one.
  //    - then multiple VBO's could be using the same pool. At least only one can define userData right now.
  //    - i had problems using same transform feedback and array buffer. had to use different ones.
  //      Seems it must be avoided to mix transform feedback and array buffers.
  VBOPool *pool = allocatorPool(usage_);
  allocatorNode_ = pool->createAllocator(bufferSize);
  allocatorNode_->userData = this;
  // XXX: maybe GPU memory allready allocated?
  allocateGPUMemory();
}
VertexBufferObject::VertexBufferObject(Usage usage, VBOPool::Node *n)
: BufferObject(glGenBuffers,glDeleteBuffers),
  usage_(usage),
  bufferSize_(n->allocator.size()),
  allocatorNode_(n)
{
  allocatorNode_->userData = this;
  // XXX: maybe GPU memory allready allocated?
  allocateGPUMemory();
}
VertexBufferObject::~VertexBufferObject()
{
}

void VertexBufferObject::allocateGPUMemory()
{
  RenderState::get()->copyWriteBuffer().push(id());
  glBufferData(GL_COPY_WRITE_BUFFER, bufferSize_, NULL, usage_);
  RenderState::get()->copyWriteBuffer().pop();
}

void VertexBufferObject::resize(GLuint bufferSize)
{
  VBOPool *pool = allocatorPool(usage_);

  pool->clear(allocatorNode_);
  allocatorNode_ = pool->createAllocator(bufferSize_);
  bufferSize_ = allocatorNode_->allocator.size();
  // XXX: maybe GPU memory allready allocated?
  allocateGPUMemory();
}

VertexBufferObject::Reference VertexBufferObject::allocateInterleaved(
    const list< ref_ptr<VertexAttribute> > &attributes)
{
  GLuint bufferSize = attributeSize(attributes);
  if(bufferSize==0) {
    Reference nullRef;
    nullRef.allocatorNode=NULL;
    return nullRef;
  }
  Reference ref = allocatorNode_->pool->alloc(allocatorNode_,bufferSize);
  if(ref.allocatorNode==NULL) return ref;

  GLuint offset = ref.allocatorRef;
  // set buffer sub data
  uploadInterleaved(offset, offset+bufferSize, attributes, ref);
  return ref;
}

VertexBufferObject::Reference VertexBufferObject::allocateBlock(GLuint size)
{
  return allocatorNode_->pool->alloc(allocatorNode_,size);
}
VertexBufferObject::Reference VertexBufferObject::allocateSequential(
    const list< ref_ptr<VertexAttribute> > &attributes)
{
  GLuint bufferSize = attributeSize(attributes);
  if(bufferSize==0) {
    Reference nullRef;
    nullRef.allocatorNode=NULL;
    return nullRef;
  }
  Reference ref = allocatorNode_->pool->alloc(allocatorNode_,bufferSize);
  if(ref.allocatorNode==NULL) return ref;

  GLuint offset = ref.allocatorRef;
  // set buffer sub data
  uploadSequential(offset, offset+bufferSize, attributes, ref);
  return ref;
}
VertexBufferObject::Reference VertexBufferObject::allocateSequential(
    const ref_ptr<VertexAttribute> &att)
{
  list< ref_ptr<VertexAttribute> > atts;
  atts.push_back(att);
  return allocateSequential(atts);
}

void VertexBufferObject::uploadSequential(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<VertexAttribute> > &attributes,
    Reference blockIterator)
{
  GLuint bufferSize = endByte-startByte;
  GLuint currOffset = 0;
  byte *data = new byte[bufferSize];

  for(list< ref_ptr<VertexAttribute> >::const_iterator
      jt = attributes.begin(); jt != attributes.end(); ++jt)
  {
    VertexAttribute *att = jt->get();
    att->set_offset( currOffset+startByte );
    att->set_stride( att->elementSize() );
    att->set_buffer( id(), blockIterator );
    // copy data
    if(att->dataPtr()) {
      std::memcpy(
          data+currOffset,
          att->dataPtr(),
          att->size()
          );
    }
    currOffset += att->size();
  }

  RenderState::get()->copyWriteBuffer().push(id());
  set_bufferSubData(GL_COPY_WRITE_BUFFER, startByte, bufferSize, data);
  RenderState::get()->copyWriteBuffer().pop();
  delete []data;
}

void VertexBufferObject::uploadInterleaved(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<VertexAttribute> > &attributes,
    Reference blockIterator)
{
  GLuint bufferSize = endByte-startByte;
  GLuint currOffset = startByte;
  // get the attribute struct size
  GLuint attributeVertexSize = 0;
  GLuint numVertices = attributes.front()->numVertices();
  byte *data = new byte[bufferSize];

  for(list< ref_ptr<VertexAttribute> >::const_iterator
      jt = attributes.begin(); jt != attributes.end(); ++jt)
  {
    VertexAttribute *att = jt->get();

    att->set_buffer( id(), blockIterator );
    if(att->divisor()==0) {
      attributeVertexSize += att->elementSize();

      att->set_offset( currOffset );
      currOffset += att->elementSize();
    }
  }

  currOffset = (currOffset-startByte)*numVertices;
  for(list< ref_ptr<VertexAttribute> >::const_iterator
      jt = attributes.begin(); jt != attributes.end(); ++jt)
  {
    VertexAttribute *att = jt->get();
    if(att->divisor()==0) {
      att->set_stride( attributeVertexSize );
    } else {
      // add instanced attributes to the end of the buffer
      att->set_stride( att->elementSize() );
      att->set_offset( currOffset+startByte );
      if(att->dataPtr()) {
        std::memcpy(
            data+currOffset,
            att->dataPtr(),
            att->size()
            );
      }
      currOffset += att->size();
    }
  }

  GLuint count = 0;
  for(GLuint i=0; i<numVertices; ++i)
  {
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        jt = attributes.begin(); jt != attributes.end(); ++jt)
    {
      VertexAttribute *att = jt->get();
      if(att->divisor()!=0) { continue; }

      // size of a value for a single vertex in bytes
      GLuint valueSize = att->valsPerElement()*att->dataTypeBytes();
      // copy data
      if(att->dataPtr()) {
        std::memcpy(
            data+count,
            att->dataPtr() + i*valueSize,
            valueSize
            );
      }
      count += valueSize;
    }
  }

  RenderState::get()->copyWriteBuffer().push(id());
  set_bufferSubData(GL_COPY_WRITE_BUFFER, startByte, bufferSize, data);
  RenderState::get()->copyWriteBuffer().pop();
  delete []data;
}

GLuint VertexBufferObject::bufferSize() const
{ return bufferSize_; }
VertexBufferObject::Usage VertexBufferObject::usage() const
{ return usage_; }

