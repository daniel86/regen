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
#include <regen/utility/gl-util.h>
#include <regen/gl-types/vertex-attribute.h>

#include "vbo.h"
using namespace regen;

static void getPositionFreeBlockStack(
    VBOBlock *value,
    OrderedStack<VBOBlock*>::Node *top,
    OrderedStack<VBOBlock*>::Node **left,
    OrderedStack<VBOBlock*>::Node **right)
{
  // free space of this group
  unsigned int size = value->size;
  OrderedStack<VBOBlock*>::Node *l = NULL, *r = NULL;

  // search for first group with less space then this group
  for(r = top; r != NULL; r = r->next)
  {
    if(r->value->size <= size) {
      *left = l;
      *right = r;
      return;
    }
    l = r;
  }
  *left = l;
  *right = NULL;
}

VertexBufferObject::VertexBufferObject(Usage usage, GLuint bufferSize)
: BufferObject(glGenBuffers,glDeleteBuffers),
  usage_(usage),
  bufferSize_(bufferSize)
{
  freeList_.set_getPosition(getPositionFreeBlockStack);
  freeList_.set_emptyValue(NULL, true);

  // create initial empty free block with specified size.
  VBOBlock *initialBlock = new VBOBlock;
  initialBlock->start = 0;
  initialBlock->end = bufferSize;
  initialBlock->size = bufferSize;
  initialBlock->left = NULL;
  initialBlock->right = NULL;
  initialBlock->node = freeList_.push(initialBlock);

  RenderState::get()->copyWriteBuffer().push(id());
  if(bufferSize_>0) {
    set_bufferData(GL_COPY_WRITE_BUFFER, bufferSize_, NULL);
  }
  RenderState::get()->copyWriteBuffer().pop();
}

VertexBufferObject::~VertexBufferObject()
{
  // delete allocated blocks
  for(list<VBOBlock*>::iterator
      jt = allocatedBlocks_.begin(); jt != allocatedBlocks_.end(); ++jt)
  {
    delete (*jt);
  }
  allocatedBlocks_.clear();
  // delete free blocks
  for(OrderedStack<VBOBlock*>::Node*
      n=freeList_.topNode(); n!=NULL; n=freeList_.topNode())
  {
    delete n->value;
    freeList_.pop();
  }
}

void VertexBufferObject::resize(GLuint bufferSize)
{
  // delete allocated blocks
  for(list<VBOBlock*>::iterator
      jt=allocatedBlocks_.begin(); jt!=allocatedBlocks_.end(); allocatedBlocks_.begin())
  { free(jt); }
  // delete free blocks
  for(OrderedStack<VBOBlock*>::Node*
      n=freeList_.topNode(); n!=NULL; n=freeList_.topNode())
  {
    delete n->value;
    freeList_.pop();
  }

  bufferSize_ = bufferSize;
  // create initial empty free block with specified size.
  VBOBlock *initialBlock = new VBOBlock;
  initialBlock->start = 0;
  initialBlock->end = bufferSize;
  initialBlock->size = bufferSize;
  initialBlock->left = NULL;
  initialBlock->right = NULL;
  initialBlock->node = freeList_.push(initialBlock);

  RenderState::get()->copyWriteBuffer().push(id());
  if(bufferSize_>0) {
    set_bufferData(GL_COPY_WRITE_BUFFER, bufferSize_, NULL);
  }
  RenderState::get()->copyWriteBuffer().pop();
}

GLuint VertexBufferObject::bufferSize() const
{
  return bufferSize_;
}

VertexBufferObject::Usage VertexBufferObject::usage() const
{
  return usage_;
}

VBOBlockIterator VertexBufferObject::endIterator()
{
  return allocatedBlocks_.end();
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

void VertexBufferObject::set_bufferData(GLenum target, GLuint size, void *data)
{
  glBufferData(target, size, data, usage_);
  bufferSize_ = size;
}

void VertexBufferObject::set_bufferSubData(GLenum target, GLuint offset, GLuint size, void *data) const
{
  glBufferSubData(target, offset, size, data);
}

void VertexBufferObject::data(GLenum target, GLuint offset, GLuint size, void *data) const
{
  glGetBufferSubData(target, offset, size, data);
}

GLvoid* VertexBufferObject::map(GLenum target, GLenum accessFlags) const
{
  return glMapBuffer(target, accessFlags);
}

GLvoid* VertexBufferObject::map(
    GLenum target, GLuint offset, GLuint size,
    GLenum accessFlags) const
{
  return glMapBufferRange(target, offset, size, accessFlags);
}

void VertexBufferObject::unmap(GLenum target) const
{
  glUnmapBuffer(target);
}

GLboolean VertexBufferObject::canAllocate(list<GLuint> &s, GLuint sizeSum)
{
  if(maxContiguousSpace()>sizeSum) { return true; }

  list<GLuint> sizes(s);
  for(OrderedStack<VBOBlock*>::Node *n=freeList_.topNode(); n!=NULL; n=n->next)
  {
    VBOBlock *block = n->value;
    GLint blockSize = block->size;
    while(true)
    {
      if(sizes.size()==0) { return true; }
      blockSize -= sizes.front();
      if(blockSize<0) { break; }
      sizes.pop_front();
    }
  }
  return (sizes.size()==0);
}

GLuint VertexBufferObject::maxContiguousSpace() const
{
  if(freeList_.topConst()==NULL) {
    return 0u;
  } else {
    // returns free space of block with maximal size in the free list
    return freeList_.topConst()->size;
  }
}

GLuint VertexBufferObject::attributeStructSize(
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

VBOBlockIterator VertexBufferObject::allocateBlock(GLuint blockSize)
{
  VBOBlock *largestBlock = freeList_.top();

  if(blockSize > largestBlock->size) {
    return allocatedBlocks_.end();
  }
  // pop the largest block out of the stack
  // Node: largestBlock->node is invalid afterwards!
  freeList_.pop();

  // create a block holding the attributes
  VBOBlock *allocatedBlock = new VBOBlock;
  allocatedBlock->start = largestBlock->start;
  allocatedBlock->end = largestBlock->start + blockSize;
  allocatedBlock->size = blockSize;
  allocatedBlock->left = largestBlock->left;
  if(allocatedBlock->left != NULL) {
    allocatedBlock->left->right = allocatedBlock;
  }
  allocatedBlock->node = NULL;

  // find iterator to VBOBlock instance
  allocatedBlocks_.push_back(allocatedBlock);
  list<VBOBlock*>::iterator needle = allocatedBlocks_.end();
  --needle;

  if(blockSize < largestBlock->size) {
    GLuint restSize = largestBlock->size-blockSize;
    // if there is some space left in the free block taken by the primitive
    VBOBlock *newFreeBlock = new VBOBlock;
    newFreeBlock->start = allocatedBlock->end;
    newFreeBlock->end = allocatedBlock->end + restSize;
    newFreeBlock->size = restSize;
    newFreeBlock->left = allocatedBlock;
    newFreeBlock->right = largestBlock->right;
    newFreeBlock->node = freeList_.push( newFreeBlock );
    allocatedBlock->right = newFreeBlock;
  } else {
    allocatedBlock->right = largestBlock->right;
  }
  if(allocatedBlock->right != NULL) {
    allocatedBlock->right->left = allocatedBlock;
  }
  delete largestBlock;

  return needle;
}

VBOBlockIterator VertexBufferObject::allocateInterleaved(
    const list< ref_ptr<VertexAttribute> > &attributes)
{
  if(attributes.size()==0) { return allocatedBlocks_.end(); }
  GLuint bufferSize = attributeStructSize(attributes);
  VBOBlockIterator blockIt = allocateBlock(bufferSize);
  if(blockIt==allocatedBlocks_.end()) { return blockIt; }
  // set buffer sub data
  RenderState::get()->copyWriteBuffer().push(id());
  addAttributesInterleaved(
      (*blockIt)->start,
      (*blockIt)->end,
      attributes, blockIt
  );
  RenderState::get()->copyWriteBuffer().pop();
  return blockIt;
}

VBOBlockIterator VertexBufferObject::allocateSequential(const list< ref_ptr<VertexAttribute> > &attributes)
{
  if(attributes.size()==0) { return allocatedBlocks_.end(); }
  GLuint bufferSize = attributeStructSize(attributes);
  VBOBlockIterator blockIt = allocateBlock(bufferSize);
  if(blockIt==allocatedBlocks_.end()) { return blockIt; }
  // set buffer sub data
  RenderState::get()->copyWriteBuffer().push(id());
  addAttributesSequential(
      (*blockIt)->start,
      (*blockIt)->end,
      attributes, blockIt
  );
  RenderState::get()->copyWriteBuffer().pop();
  return blockIt;
}

VBOBlockIterator VertexBufferObject::allocateSequential(const ref_ptr<VertexAttribute> &att)
{
  list< ref_ptr<VertexAttribute> > atts;
  atts.push_back(att);
  return allocateSequential(atts);
}

void VertexBufferObject::free(VBOBlockIterator &jt)
{
  if(jt==allocatedBlocks_.end()) { return; }

  VBOBlock *block = *jt;
  allocatedBlocks_.erase(jt);

  // add primitive space to free list !
  VBOBlock *left = block->left;
  VBOBlock *right = block->right;
  // join left block if it is free
  if(left!=NULL && left->node!=NULL) {
    block->left = left->left;
    if(block->left != NULL) {
      block->left->right = block;
    }
    block->start = left->start;
    block->size += left->size;
    freeList_.remove(left->node->value);
    delete left;
  }

  // join right block if it is free
  if(right!=NULL && right->node!=NULL) {
    block->right = right->right;
    if(right->right != NULL) {
      right->right->left = block;
    }
    block->end = right->end;
    block->size += right->size;
    freeList_.remove(right->node->value);
    delete right;
  }

  block->node = freeList_.push(block);
}

void VertexBufferObject::addAttributesSequential(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<VertexAttribute> > &attributes,
    VBOBlockIterator blockIterator)
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

  set_bufferSubData(GL_COPY_WRITE_BUFFER, startByte, bufferSize, data);
  delete []data;
}

void VertexBufferObject::addAttributesInterleaved(
    GLuint startByte,
    GLuint endByte,
    const list< ref_ptr<VertexAttribute> > &attributes,
    VBOBlockIterator blockIterator)
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

  set_bufferSubData(GL_COPY_WRITE_BUFFER, startByte, bufferSize, data);
  delete []data;
}

