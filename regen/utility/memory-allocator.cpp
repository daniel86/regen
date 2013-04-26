/*
 * memory-allocator.cpp
 *
 *  Created on: 26.04.2013
 *      Author: daniel
 */

#include "memory-allocator.h"
#include <regen/utility/logging.h>

using namespace regen;

BuddyAllocator::BuddyNode::BuddyNode(
    unsigned int _address, unsigned int _size,
    BuddyNode *_parent)
: state(FREE),
  address(_address), size(_size), maxSpace(_size),
  leftChild(NULL), rightChild(NULL), parent(_parent)
{}

BuddyAllocator::BuddyAllocator(unsigned int size)
{
  buddyTree_ = new BuddyNode(0u, size, NULL);
}

BuddyAllocator::State BuddyAllocator::allocaterState() const
{ return buddyTree_->state; }
unsigned int BuddyAllocator::size() const
{ return buddyTree_->size; }
unsigned int BuddyAllocator::maxSpace() const
{ return buddyTree_->maxSpace; }

void BuddyAllocator::computeMaxSpace(BuddyNode *n)
{
  // must recompute maxSpace for given node and all parents
  for(BuddyNode *t=n; t!=NULL; t=t->parent)
  {
    t->maxSpace = max(t->leftChild->maxSpace, t->rightChild->maxSpace);
  }
}

unsigned int BuddyAllocator::createPartition(BuddyNode *n, unsigned int size)
{
  REGEN_ASSERT(n->state==FREE);
  REGEN_ASSERT(n->size>size);

  if(size == n->size) {
    // the buddy node fits perfectly with the requested size.
    // Just mark the node as full and return the address.
    n->state = FULL;
    n->maxSpace = 0;
    return n->address;
  }
  else if(size*2 > n->size) {
    // half of the node size is not enough for the requested memory.
    // Create a node that fits exactly with the requested size
    // and the rest of the node space remains free for allocation.
    // No intern fragmentation occurs.
    unsigned int size0 = size;
    unsigned int size1 = n->size-size;
    n->state = PARTIAL;
    n->maxSpace = size1;
    n->leftChild = new BuddyNode(n->address, size0, n);
    n->leftChild->state = FULL;
    n->leftChild->maxSpace = 0;
    n->rightChild = new BuddyNode(n->address+size0, size1, n);
    return n->address;
  }
  else {
    // half of the node size is enough for the requested memory.
    // Split the node with half size and continue for left child.
    unsigned int size0 = n->size/2;
    unsigned int size1 = n->size-size0;
    n->state = PARTIAL;
    n->maxSpace = size1;
    return createPartition(n->leftChild, size);
  }
}

bool BuddyAllocator::alloc(unsigned int size, unsigned int *x)
{
  if(buddyTree_->maxSpace<size) return false;

  BuddyNode *n=buddyTree_;
  while(1)
  {
    switch(n->state) {
    case FREE:
      // free node with enough space. Create partitions inside the
      // node to avoid internal fragmentation.
      *x = createPartition(n,size);
      computeMaxSpace(n);
      return true;
    case PARTIAL:
      // walk down the tree until we reach a FREE node.
      // prefer left nodes over right nodes
      if(n->leftChild->maxSpace>size)
      { n=n->leftChild; }
      else
      { n=n->rightChild; }
      break;
    default:
      REGEN_ASSERT(0);//should not be reached
      return false;
    }
  }
  return false;
}

void BuddyAllocator::free(unsigned int address)
{
  // find the node that allocates given address
  BuddyNode *t=buddyTree_;
  while(t->rightChild) {
    t = (address>=t->rightChild->address) ? t->rightChild : t->leftChild;
  }

  // mark as free space
  t->state = FREE;
  t->maxSpace = t->size;
  // join parents with both childs becoming FREE with this call
  while(t->parent)
  {
    if(t->parent->leftChild->state == FREE &&
       t->parent->rightChild->state == FREE)
    {
      t = t->parent;
      delete t->leftChild;
      delete t->rightChild;
      // mark as free space
      t->state = FREE;
      t->maxSpace = t->size;
    }
    else
    {
      computeMaxSpace(t->parent);
      break;
    }
  }
}
