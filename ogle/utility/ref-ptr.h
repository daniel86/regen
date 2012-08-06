/*
 * ref-ptr.h
 *
 *  Created on: 22.03.2011
 *      Author: daniel
 */

#ifndef REF_PTR_H_
#define REF_PTR_H_

#include <iostream>

/**
 * Adds auto reference Management to a pointer.
 * Note that only ref_ptr should manage the memory of the data pointer.
 * Else you get double free corruption.
 * Do not instantiate two ref_ptr with the same data pointer.
 * Copying a ref_ptr is valid.
 */
template<class T> class ref_ptr
{
public:
  /**
   * Manage a pointer with reference counting.
   * You should never add the same pointer twice with manage().
   */
  static ref_ptr<T> manage(T *ptr)
  {
    ref_ptr<T> ref = ref_ptr<T>();
    ref.managePtr(ptr);
    return ref;
  }

  /**
   * Init without data and reference counter.
   */
  ref_ptr() : ptr_(NULL), refCount_(NULL)
  {
  }

  /**
   * Init from other ref_ptr,
   * both will share same data and reference counter afterwards.
   */
  ref_ptr(const ref_ptr<T> &other) : ptr_(other.ptr_), refCount_(other.refCount_)
  {
    if(ptr_ != NULL) {
      ref();
    }
  }
  /**
   * Copy constructor.
   * Takes a reference on the data pointer of the other ref_ptr.
   */
  template<typename K>
  ref_ptr(ref_ptr<K> &other) : ptr_(other.get()), refCount_(other.refCount())
  {
    if(ptr_ != NULL) {
      ref();
    }
  }

  /**
   * Destructor unreferences if data pointer set.
   */
  ~ref_ptr()
  {
    if(ptr_ != NULL) {
      unref();
    }
  }

  /**
   * Access data pointer.
   * Note: If no data set you will get a null pointer here.
   */
  T* operator->() const
  {
    return ptr_;
  }


  /**
   * Set from other ref_ptr,
   * both will share same data and reference counter afterwards.
   * Old data gets unreferenced.
   */
  ref_ptr& operator=(const ref_ptr<T> &other)
  {
    if(ptr_ != NULL) {
      unref();
    }
    ptr_ = other.ptr_;
    refCount_ = other.refCount_;
    if(ptr_ != NULL) {
      ref();
    }
    return *this;
  }

  /**
   * Compares ref_ptr by data pointer.
   */
  bool operator==(const ref_ptr<T> &other) const
  {
    return ptr_ == other.ptr_;
  }
  /**
   * Compares ref_ptr by data pointer.
   */
  bool operator<(const ref_ptr<T> &other) const
  {
    return ptr_ < other.ptr_;
  }

  /**
   * Returns referenced pointer.
   * Do not delete, this is done in ref_ptr destructor.
   * Returns NULL if no data pointer set.
   */
  T* get() const
  {
    return ptr_;
  }

  /**
   * Pointer to reference counter.
   */
  unsigned int* refCount() const
  {
    return refCount_;
  }

private:
  T *ptr_;
  unsigned int *refCount_;

  void managePtr(T *ptr)
  {
    if(ptr_ != NULL) {
      unref();
    }
    ptr_ = ptr;
    if(ptr_ != NULL) {
      refCount_ = new unsigned int;
      *refCount_ = 1;
    } else {
      refCount_ = NULL;
    }
  }
  void ref()
  {
    *refCount_ += 1;
  }
  void unref()
  {
    *refCount_ -= 1;
    if(*refCount_ == 0) {
      delete refCount_;
      refCount_ = NULL;

      delete ptr_;
      ptr_ = NULL;
    }
  }
};


#endif /* REF_PTR_H_ */
