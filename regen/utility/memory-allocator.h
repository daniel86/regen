/*
 * memory-allocator.h
 *
 *  Created on: 26.04.2013
 *      Author: daniel
 */

#ifndef MEMORY_ALLOCATOR_H_
#define MEMORY_ALLOCATOR_H_

#include <stdlib.h>

namespace regen {
  // TODO: handle case when allocators become FREE again ?
  //    - maybe a lot of small allocators stay unusable when large requests made
  //    - avoid using actual allocator

  /**
   * \brief A pool of memory allocators.
   *
   * Each allocator manages contiguous pre-allocated memory.
   * If no allocator can handle a request a new allocator is
   * automatically created.
   *
   * Allocators must implement alloc,free,size,maxSpace and a constructor
   * taking an uint defining the amount of managed memory.
   * The actual unit of the managed memory is arbitrary, it can refer to
   * KBs, MBs, pages or whatever you need to manage.
   * For example if you define your unit as pages you can be sure that no one
   * can allocate blocks smaller then a page-size.
   *
   * Allocators don't actually allocate anything. You have to implement
   * the memory semantic for yourself by attaching allocator nodes
   * to objects which do the allocation once on construction
   * and the deallocation once on destruction.
   */
  template<typename ActualAllocatorType,
           typename ActualAllocatorRef,
           typename VirtualAllocatorType,
           typename VirtualAllocatorRef> class AllocatorPool
  {
  public:
    /**
     * \brief Doubly linked node containing an allocator.
     */
    struct Node {
      /**
       * @param _pool the pool that contains this node.
       * @param size the allocator size.
       */
      Node(AllocatorPool *_pool, unsigned int size, unsigned int alignment)
      : pool(_pool), allocator(size,alignment), prev(NULL), next(NULL) {}
      AllocatorPool *pool;               //!< the allocator pool
      VirtualAllocatorType allocator;    //!< the allocator
      ActualAllocatorRef   allocatorRef; //!< the allocator actual reference
      Node *prev;                        //!< allocator with bigger maxSpace
      Node *next;                        //!< allocator with smaller maxSpace
    };
    /**
     * \brief Reference to allocated memory.
     */
    struct Reference {
      Node *allocatorNode;               //!< the allocator
      VirtualAllocatorRef allocatorRef;  //!< the allocator virtual reference
    };

    AllocatorPool()
    : allocators_(NULL), minSize_(4u*1024u*1024u), minSizeUnaligned_(minSize_), alignment_(1)
    {}
    ~AllocatorPool()
    {
      for(Node *n=allocators_; n!=NULL;)
      {
        Node *buf = n;
        n = n->next;
        // free actual memory
        ActualAllocatorType::deleteAllocator(index_,buf->allocatorRef);
        delete buf;
      }
    }

    /**
     * @param size min size of automatically instantiated allocators.
     */
    void set_minSize(unsigned int size)
    {
      minSizeUnaligned_ = size;
      minSize_ = align(minSizeUnaligned_);
    }
    void set_alignment(unsigned int alignment)
    {
      alignment_ = alignment;
      minSize_ = align(minSizeUnaligned_);
    }
    unsigned int align(unsigned int v)
    {
      return v + (alignment_-v)%alignment_;
    }

    /**
     * Instantiate a new allocator and add it to the pool.
     * @param size the allocator size.
     */
    Node* createAllocator(unsigned int size)
    {
      unsigned int actualSize = align(size>minSize_ ? size : minSize_);
      Node *x = new Node(this,actualSize,alignment_);
      x->prev = NULL;
      x->next = allocators_;
      // allocate actual memory
      x->allocatorRef = ActualAllocatorType::createAllocator(index_,actualSize);
      if(allocators_) allocators_->prev = x;
      allocators_ = x;
      sortInForward(x);
      return x;
    }

    /**
     * Choose an allocator that has enough free space.
     * @param size minimum free space of the allocator.
     * @return chosen allocator.
     */
    Node* chooseAllocator(unsigned int size)
    {
      // find allocator with smallest maxSpace and maxSpace>size
      Node *min=NULL;
      for(Node *n=allocators_; n!=NULL && n->allocator.maxSpace()>size; n=n->next)
      { min = n; }
      return min;
    }

    /**
     * Clear the given allocator. The allocator state will be FREE afterwards.
     * The actual memory stays allocated.
     * @param n an allocator.
     */
    void clear(Node *n)
    {
      n->allocator.clear();
      if(n->next) n->next->prev = n->prev;
      if(n->prev) n->prev->next = n->next;
      // sort in front to back
      n->prev = NULL;
      n->next = allocators_;
      allocators_->prev = n;
      allocators_ = n;
      sortInForward(n);
    }

    /**
     * Allocate virtual memory managed by an allocator.
     * @param size number of bytes to allocate.
     * @return reference of allocated block
     */
    Reference alloc(unsigned int _size)
    {
      unsigned int size = align(_size);
      AllocatorPool::Reference ref;
      // find allocator with smallest maxSpace and maxSpace>size
      Node *min = chooseAllocator(size);
      if(min==NULL) {
        ref.allocatorNode = NULL;
      }
      else if(min->allocator.alloc(size, &ref.allocatorRef)) {
        ref.allocatorNode = min;
        sortInForward(min);
      }
      else {
        ref.allocatorNode = NULL;
      }
      return ref;
    }

    /**
     * Allocate virtual memory managed by an allocator.
     * @param n allocator that is used.
     * @param size number of bytes to allocate.
     * @return reference of allocated block
     */
    Reference alloc(Node *n, unsigned int _size)
    {
      unsigned int size = align(_size);
      AllocatorPool::Reference ref;
      if(n->allocator.maxSpace()<size) {
        ref.allocatorNode = NULL;
      }
      else if(n->allocator.alloc(size, &ref.allocatorRef)) {
        ref.allocatorNode = n;
        sortInForward(n);
      }
      else {
        ref.allocatorNode = NULL;
      }
      return ref;
    }

    /**
     * Free previously allocated virtual memory.
     * @param ref reference of allocated block
     */
    void free(Reference &ref)
    {
      ref.allocatorNode->allocator.free(ref.allocatorRef);
      sortInBackward(ref.allocatorNode);
      ref.allocatorNode = NULL;
    }
    /**
     * @return the allocator index. semantic is on to you.
     */
    unsigned int index() const { return index_; }
    /**
     * @param index the allocator index. semantic is on to you.
     */
    void set_index(unsigned int index) { index_ = index; }

  protected:
    Node *allocators_;
    unsigned int minSize_;
    unsigned int minSizeUnaligned_;
    unsigned int alignment_;
    unsigned int index_;

    void sortInForward(Node *resizedNode)
    {
      unsigned int space = resizedNode->allocator.maxSpace();
      for(Node *n=resizedNode->next;
          n!=NULL && n->allocator.maxSpace()>space;
          n=n->next)
      { swap(n->prev,n); }
      // update head
      while(allocators_->prev) allocators_=allocators_->prev;
    }
    void sortInBackward(Node *resizedNode)
    {
      unsigned int space = resizedNode->allocator.maxSpace();
      for(Node *n=resizedNode->prev;
          n!=NULL && n->allocator.maxSpace()<space;
          n=n->prev)
      { swap(n->next,n); }
      // update head
      while(allocators_->prev) allocators_=allocators_->prev;
    }

    void swap(Node *n0, Node *n1)
    {
      if(n1->next) n1->next->prev=n0;
      if(n0->prev) n0->prev->next=n1;
      n1->prev = n0->prev;
      n0->next = n1->next;
      n0->prev = n1;
      n1->next = n0;
    }
  };

  /////////////////////////////////
  /////////////////////////////////
  /////////////////////////////////

  /**
   * \brief Implements a variant of the buddy memory allocation algorithm for virtual memory allocation.
   *
   * Dynamic memory allocation is not cheap. GPU memory is precious.
   * For these reasons this class provides memory management with
   * the intention to provide fast alloc() and free() functions
   * and keeping the fragmentation in the memory as low as possible without
   * moving any memory.
   *
   * The algorithm uses a binary tree to partition the pre-allocated memory.
   * When memory is allocated the algorithm searches for a `free` node that
   * offers enough space for the request.
   * The chosen node is cut in halves until half the node size does not
   * fit the request anymore. Then the node is cut into one `full` node that
   * fits the request exactly and another  `free` node for the remaining space.
   * No internal fragmentation occurs using this implementation.
   * External fragmentation can happen when partitions are to small
   * to fit allocation requests for a long time.
   * Allocating some relative small chunks of memory helps in keeping the
   * fragmentation costs low.
   *
   * Addresses are only used relative to the start address within the allocator.
   * If the allocated memory refers to any RAM you have to offset the
   * pre-allocated data pointer with the relative addresses used
   * within this class.
   */
  class BuddyAllocator
  {
  public:
    /**
     * \brief the virtual address.
     */
    typedef unsigned int Reference;
    /**
     * The current allocator state.
     */
    enum State {
      FREE,  //!< no allocate active
      FULL,  //!< no more allocate possible
      PARTIAL//!< some allocates active but space left
    };

    /**
     * @param size number of pre-allocated bytes.
     */
    BuddyAllocator(unsigned int size, unsigned int alignment);

    /**
     * @return The current allocator state.
     */
    State allocaterState() const;
    /**
     * @return number of pre-allocated bytes.
     */
    unsigned int size() const;
    /**
     * @return maximum contiguous space that can be allocated.
     */
    unsigned int maxSpace() const;

    /**
     * Allocate memory managed by the allocator.
     * @param size number of bytes to allocate.
     * @param addressRet relative address of allocated memory
     * @return false if not enough space left.
     */
    bool alloc(unsigned int size, unsigned int *addressRet);
    /**
     * Free previously allocated memory.
     * @param address address of the memory block to free
     */
    void free(unsigned int address);

    /**
     * Clear this allocator. The allocator state will be FREE afterwards.
     */
    void clear();

  protected:
    struct BuddyNode {
      BuddyNode(
          unsigned int address,
          unsigned int size,
          BuddyNode *parent);
      State state;
      unsigned int address;
      unsigned int size;
      unsigned int maxSpace;
      BuddyNode *leftChild;
      BuddyNode *rightChild;
      BuddyNode *parent;
    };
    BuddyNode *buddyTree_;
    unsigned int alignment_;

    unsigned int createPartition(BuddyNode *n, unsigned int size);
    void computeMaxSpace(BuddyNode *n);
    void clear(BuddyNode *n);
  };
}

#endif /* MEMORY_ALLOCATOR_H_ */
