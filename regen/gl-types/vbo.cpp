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
#include <regen/gl-types/render-state.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/shader-input.h>

#include "vbo.h"
using namespace regen;

namespace regen {
  ostream& operator<<(ostream &out, const VBO::Usage &mode)
  {
    switch(mode) {
    case VBO::USAGE_DYNAMIC:  return out << "DYNAMIC";
    case VBO::USAGE_STATIC:   return out << "STATIC";
    case VBO::USAGE_STREAM:   return out << "STREAM";
    case VBO::USAGE_FEEDBACK: return out << "FEEDBACK";
    case VBO::USAGE_TEXTURE:  return out << "TEXTURE";
    case VBO::USAGE_UNIFORM:  return out << "UNIFORM";
    case VBO::USAGE_LAST:     return out << "DYNAMIC";
    }
    return out;
  }
  istream& operator>>(istream &in, VBO::Usage &mode)
  {
    string val;
    in >> val;
    boost::to_upper(val);
    if(val == "DYNAMIC")        mode = VBO::USAGE_DYNAMIC;
    else if(val == "STATIC")    mode = VBO::USAGE_STATIC;
    else if(val == "STREAM")    mode = VBO::USAGE_STREAM;
    else if(val == "FEEDBACK")  mode = VBO::USAGE_FEEDBACK;
    else if(val == "TEXTURE")   mode = VBO::USAGE_TEXTURE;
    else if(val == "UNIFORM")   mode = VBO::USAGE_UNIFORM;
    else {
      REGEN_WARN("Unknown VBO usage '" << val << "'. Using default DYNAMIC blending.");
      mode = VBO::USAGE_DYNAMIC;
    }
    return in;
  }
}

/////////////////////
/////////////////////

VBO::VBOPool* VBO::dataPools_=NULL;

GLuint VBO::attributeSize(
    const list< ref_ptr<ShaderInput> > &attributes)
{
  if(attributes.size()>0) {
    GLuint structSize = 0;
    for(list< ref_ptr<ShaderInput> >::const_iterator
        it = attributes.begin(); it != attributes.end(); ++it)
    {
      structSize += (*it)->inputSize();
    }
    return structSize;
  }
  return 0;
}

void VBO::copy(
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

VBO::VBOPool* VBO::memoryPool(Usage usage)
{
  return &dataPools_[(int)usage];
}

void VBO::createMemoryPools()
{
  if(dataPools_!=NULL) return;

  dataPools_ = new VBOPool[USAGE_LAST];
  for(int i=0; i<USAGE_LAST; ++i)
  { dataPools_[i].set_index(i); }

#ifdef GL_ARB_texture_buffer_range
  dataPools_[USAGE_TEXTURE].set_alignment(
      getGLInteger(GL_TEXTURE_BUFFER_OFFSET_ALIGNMENT));
  // XXX: hack that forces to use separate buffers with each alloc :/
  //          - nothing drawn when minSize set to a few MB
  //          - not allowed to have multiple samplerBuffer's with same buffer object ?
  //          - different format not ok ?
  dataPools_[USAGE_TEXTURE].set_minSize(1);
#else
  dataPools_[USAGE_TEXTURE].set_alignment(16);
  // no glTexBufferRange available. We have to use
  // separate buffer objects for each texture buffer.
  // because it is not possible to attach with offset without glTexBufferRange.
  dataPools_[USAGE_TEXTURE].set_minSize(1);
#endif
}

void VBO::destroyMemoryPools()
{
  if(dataPools_==NULL) return;

  delete []dataPools_;
  dataPools_ = NULL;
}

/////////////////////
/////////////////////

GLboolean VBO::Reference::isNullReference() const
{ return allocatedSize_==0u; }
GLuint VBO::Reference::allocatedSize() const
{ return allocatedSize_; }
// virtual address is the virtual allocator reference
GLuint VBO::Reference::address() const
{ return poolReference_.allocatorRef * poolReference_.allocatorNode->pool->alignment(); }
// GL buffer handle is the actual allocator reference
GLuint VBO::Reference::bufferID() const
{ return poolReference_.allocatorNode->allocatorRef; }
VBO* VBO::Reference::vbo() const
{ return vbo_; }

VBO::Reference::~Reference()
{
  // memory in pool is marked as free when reference desturctor is called
  if(dataPools_ && poolReference_.allocatorNode!=NULL) {
    VBOPool *pool = poolReference_.allocatorNode->pool;
    pool->free(poolReference_);
    poolReference_.allocatorNode = NULL;
  }
}

/////////////////////
/////////////////////

GLuint VBO::VBOAllocator::createAllocator(GLuint poolIndex, GLuint size)
{
  RenderState *rs = RenderState::get();
  // create buffer
  GLuint ref;
  glGenBuffers(1, &ref);
  // and allocate GPU memory
  switch((Usage)poolIndex) {
  case USAGE_DYNAMIC:
    rs->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    rs->arrayBuffer().pop();
    break;
  case USAGE_STATIC:
    rs->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STATIC_DRAW);
    rs->arrayBuffer().pop();
    break;
  case USAGE_STREAM:
    rs->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_STREAM_DRAW);
    rs->arrayBuffer().pop();
    break;
  case USAGE_FEEDBACK:
    rs->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    rs->arrayBuffer().pop();
    break;
  case USAGE_UNIFORM:
    rs->arrayBuffer().push(ref);
    glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    rs->arrayBuffer().pop();
    break;
  case USAGE_TEXTURE:
    rs->textureBuffer().push(ref);
    glBufferData(GL_TEXTURE_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
    rs->textureBuffer().pop();
    break;
  case USAGE_LAST:
    break;
  }
  return ref;
}
void VBO::VBOAllocator::deleteAllocator(GLenum usage, GLuint ref)
{
  glDeleteBuffers(1, &ref);
}

/////////////////////
/////////////////////

VBO::VBO(Usage usage)
: usage_(usage), allocatedSize_(0u)
{
}

ref_ptr<VBO::Reference>& VBO::nullReference()
{
  static ref_ptr<Reference> ref;
  if(ref.get()==NULL) {
    ref = ref_ptr<Reference>::alloc();
    ref->allocatedSize_ = 0;
    ref->vbo_ = NULL;
    ref->poolReference_.allocatorNode = NULL;
  }
  return ref;
}

ref_ptr<VBO::Reference>& VBO::createReference(GLuint numBytes)
{
  VBOPool* memoryPool_ = memoryPool(usage_);
  // get an allocator
  VBOPool::Node *allocator = memoryPool_->chooseAllocator(numBytes);
  if(allocator==NULL)
  { allocator = memoryPool_->createAllocator(numBytes); }

  ref_ptr<Reference> ref = ref_ptr<Reference>::alloc();
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

ref_ptr<VBO::Reference>& VBO::alloc(GLuint numBytes)
{
  return createReference(numBytes);
}

ref_ptr<VBO::Reference>& VBO::alloc(const ref_ptr<ShaderInput> &att)
{
  list< ref_ptr<ShaderInput> > atts;
  atts.push_back(att);
  return allocSequential(atts);
}

ref_ptr<VBO::Reference>& VBO::allocInterleaved(
    const list< ref_ptr<ShaderInput> > &attributes)
{
  GLuint numBytes = attributeSize(attributes);
  ref_ptr<Reference> &ref = createReference(numBytes);
  if(ref->allocatedSize()<numBytes) return ref;
  GLuint offset = ref->address();
  // set buffer sub data
  uploadInterleaved(offset, offset+numBytes, attributes, ref);
  return ref;
}

ref_ptr<VBO::Reference>& VBO::allocSequential(
    const list< ref_ptr<ShaderInput> > &attributes)
{
  GLuint numBytes = attributeSize(attributes);
  ref_ptr<Reference> &ref = createReference(numBytes);
  if(ref->allocatedSize()<numBytes) return ref;
  GLuint offset = ref->address();
  // set buffer sub data
  uploadSequential(offset, offset+numBytes, attributes, ref);
  return ref;
}

void VBO::free(Reference *ref)
{
  if(dataPools_ && ref->vbo_!=NULL) {
    ref->vbo_->allocatedSize_ -= ref->allocatedSize_;
    ref->vbo_->allocations_.erase(ref->it_);
    ref->vbo_ = NULL;
  }
}

void VBO::uploadSequential(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<ShaderInput> > &attributes,
    ref_ptr<Reference> &ref)
{
  GLuint bufferSize = endByte-startByte;
  GLuint currOffset = 0;
  byte *data = new byte[bufferSize];

  for(list< ref_ptr<ShaderInput> >::const_iterator
      jt = attributes.begin(); jt != attributes.end(); ++jt)
  {
    ShaderInput *att = jt->get();
    att->set_offset( currOffset+startByte );
    att->set_stride( att->elementSize() );
    att->set_buffer( ref->bufferID(), ref );
    // copy data
    if(att->dataPtr()) {
      std::memcpy(
          data+currOffset,
          att->dataPtr(),
          att->inputSize()
          );
    }
    currOffset += att->inputSize();
  }

  RenderState::get()->copyWriteBuffer().push(ref->bufferID());
  set_bufferSubData(GL_COPY_WRITE_BUFFER, startByte, bufferSize, data);
  RenderState::get()->copyWriteBuffer().pop();
  delete []data;
}

void VBO::uploadInterleaved(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<ShaderInput> > &attributes,
    ref_ptr<Reference> &ref)
{
  GLuint bufferSize = endByte-startByte;
  GLuint currOffset = startByte;
  // get the attribute struct size
  GLuint attributeVertexSize = 0;
  GLuint numVertices = attributes.front()->numVertices();
  byte *data = new byte[bufferSize];

  for(list< ref_ptr<ShaderInput> >::const_iterator
      jt = attributes.begin(); jt != attributes.end(); ++jt)
  {
    ShaderInput *att = jt->get();

    att->set_buffer( ref->bufferID(), ref );
    if(att->divisor()==0) {
      attributeVertexSize += att->elementSize();

      att->set_offset( currOffset );
      currOffset += att->elementSize();
    }
  }

  currOffset = (currOffset-startByte)*numVertices;
  for(list< ref_ptr<ShaderInput> >::const_iterator
      jt = attributes.begin(); jt != attributes.end(); ++jt)
  {
    ShaderInput *att = jt->get();
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
            att->inputSize()
            );
      }
      currOffset += att->inputSize();
    }
  }

  GLuint count = 0;
  for(GLuint i=0; i<numVertices; ++i)
  {
    for(list< ref_ptr<ShaderInput> >::const_iterator
        jt = attributes.begin(); jt != attributes.end(); ++jt)
    {
      ShaderInput *att = jt->get();
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

GLuint VBO::allocatedSize() const
{ return allocatedSize_; }
VBO::Usage VBO::usage() const
{ return usage_; }

