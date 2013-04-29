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

VertexBufferObject::VBOPool* VertexBufferObject::dataPools_=NULL;

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

GLboolean VertexBufferObject::Reference::isNullReference() const
{ return allocatedSize_==0u; }
GLuint VertexBufferObject::Reference::allocatedSize() const
{ return allocatedSize_; }
// virtual address is the virtual allocator reference
GLuint VertexBufferObject::Reference::address() const
{ return poolReference_.allocatorRef; }
// GL buffer handle is the actual allocator reference
GLuint VertexBufferObject::Reference::bufferID() const
{ return poolReference_.allocatorNode->allocatorRef; }

VertexBufferObject::Reference::~Reference()
{
  // memory in pool is marked as free when reference desturctor is called
  if(poolReference_.allocatorNode!=NULL) {
    VBOPool *pool = poolReference_.allocatorNode->pool;
    pool->free(poolReference_);
  }
}

/////////////////////
/////////////////////

GLuint VertexBufferObject::VBOAllocator::createAllocator(GLuint poolIndex, GLuint size)
{
  // create buffer
  GLuint ref;
  glGenBuffers(1, &ref);
  // and allocate GPU memory
  switch((Usage)poolIndex) {
  case USAGE_DYNAMIC:
    RenderState::get()->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    RenderState::get()->arrayBuffer().pop();
    break;
  case USAGE_STATIC:
    RenderState::get()->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
    RenderState::get()->arrayBuffer().pop();
    break;
  case USAGE_STREAM:
    RenderState::get()->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
    RenderState::get()->arrayBuffer().pop();
    break;
  case USAGE_FEEDBACK:
    RenderState::get()->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    RenderState::get()->arrayBuffer().pop();
    break;
  case USAGE_TEXTURE:
    RenderState::get()->textureBuffer().push(ref);
    glBufferData(GL_TEXTURE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    RenderState::get()->textureBuffer().pop();
    break;
  case USAGE_LAST:
    break;
  }
  return ref;
}
void VertexBufferObject::VBOAllocator::deleteAllocator(GLenum usage, GLuint ref)
{
  glDeleteBuffers(1, &ref);
}

/////////////////////
/////////////////////

VertexBufferObject::VertexBufferObject(Usage usage)
: usage_(usage), allocatedSize_(0u)
{
  if(dataPools_==NULL) {
    dataPools_ = new VBOPool[USAGE_LAST];
    for(int i=0; i<USAGE_LAST; ++i)
    { dataPools_[i].set_index(i); }

    GLuint tboAlign = getGLInteger(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT);
    if(tboAlign<1) {
      ERROR_LOG("GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT<=0. VertexBufferObject created without GL context?");
      exit(1); // XXX not so nice here
    }
    dataPools_[USAGE_TEXTURE].set_alignment(tboAlign);
    // XXX: hack that forces to use separate buffers with each alloc :/
    //          - nothing drawn when minSize set to a few MB
    //          - not allowed to have multiple samplerBuffer's with same buffer object ?
    //          - different format not ok ?
    dataPools_[USAGE_TEXTURE].set_minSize(tboAlign);
  }
  memoryPool_ = &dataPools_[(int)usage_];
}

ref_ptr<VertexBufferObject::Reference>& VertexBufferObject::nullReference()
{
  static ref_ptr<Reference> ref;
  if(ref.get()==NULL) {
    ref = ref_ptr<Reference>::manage(new Reference);
    ref->allocatedSize_ = 0;
    ref->vbo_ = NULL;
    ref->poolReference_.allocatorNode = NULL;
  }
  return ref;
}

ref_ptr<VertexBufferObject::Reference>& VertexBufferObject::createReference(GLuint numBytes)
{
  // get an allocator
  VBOPool::Node *allocator = memoryPool_->chooseAllocator(numBytes);
  if(allocator==NULL)
  { allocator = memoryPool_->createAllocator(numBytes); }

  ref_ptr<Reference> ref = ref_ptr<Reference>::manage(new Reference);
  ref->poolReference_ = memoryPool_->alloc(allocator,numBytes);
  if(ref->poolReference_.allocatorNode==NULL)
  { return nullReference(); }

  allocations_.push_front(ref);
  ref->it_ = allocations_.begin();
  ref->allocatedSize_ = numBytes;
  ref->vbo_ = this;

  allocatedSize_ += numBytes;
  return allocations_.front();
}

ref_ptr<VertexBufferObject::Reference>& VertexBufferObject::alloc(GLuint numBytes)
{
  return createReference(numBytes);
}

ref_ptr<VertexBufferObject::Reference>& VertexBufferObject::alloc(const ref_ptr<VertexAttribute> &att)
{
  list< ref_ptr<VertexAttribute> > atts;
  atts.push_back(att);
  return allocSequential(atts);
}

ref_ptr<VertexBufferObject::Reference>& VertexBufferObject::allocInterleaved(
    const list< ref_ptr<VertexAttribute> > &attributes)
{
  GLuint numBytes = attributeSize(attributes);
  ref_ptr<Reference> &ref = createReference(numBytes);
  if(ref->allocatedSize()<numBytes) return ref;
  GLuint offset = ref->address();
  // set buffer sub data
  uploadInterleaved(offset, offset+numBytes, attributes, ref);
  return ref;
}

ref_ptr<VertexBufferObject::Reference>& VertexBufferObject::allocSequential(
    const list< ref_ptr<VertexAttribute> > &attributes)
{
  GLuint numBytes = attributeSize(attributes);
  ref_ptr<Reference> &ref = createReference(numBytes);
  if(ref->allocatedSize()<numBytes) return ref;
  GLuint offset = ref->address();
  // set buffer sub data
  uploadSequential(offset, offset+numBytes, attributes, ref);
  return ref;
}

void VertexBufferObject::free(Reference *ref)
{
  if(ref->vbo_!=NULL) {
    ref->vbo_->allocatedSize_ -= ref->allocatedSize_;
    ref->vbo_->allocations_.erase(ref->it_);
    ref->vbo_ = NULL;
  }
}

void VertexBufferObject::uploadSequential(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<VertexAttribute> > &attributes,
    ref_ptr<Reference> &ref)
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
    att->set_buffer( ref->bufferID(), ref );
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

  RenderState::get()->copyWriteBuffer().push(ref->bufferID());
  set_bufferSubData(GL_COPY_WRITE_BUFFER, startByte, bufferSize, data);
  RenderState::get()->copyWriteBuffer().pop();
  delete []data;
}

void VertexBufferObject::uploadInterleaved(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<VertexAttribute> > &attributes,
    ref_ptr<Reference> &ref)
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

    att->set_buffer( ref->bufferID(), ref );
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

  RenderState::get()->copyWriteBuffer().push(ref->bufferID());
  set_bufferSubData(GL_COPY_WRITE_BUFFER, startByte, bufferSize, data);
  RenderState::get()->copyWriteBuffer().pop();
  delete []data;
}

GLuint VertexBufferObject::allocatedSize() const
{ return allocatedSize_; }
VertexBufferObject::Usage VertexBufferObject::usage() const
{ return usage_; }

