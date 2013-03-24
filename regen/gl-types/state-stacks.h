/*
 * state-stacks.h
 *
 *  Created on: 26.02.2013
 *      Author: daniel
 */

#ifndef STATE_STACKS_H_
#define STATE_STACKS_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <regen/utility/stack.h>
#include <regen/algebra/vector.h>

namespace ogle {

/**
 * \brief A value and a timestamp.
 */
template<typename T> struct StampedValue
{
  T v; /**< the value. */
  GLuint stamp; /**< the time stamp. */
  /**
   * @param v_ the value.
   * @param stamp_ the stamp.
   */
  StampedValue(const T &v_, GLuint stamp_) : v(v_), stamp(stamp_) {}
};

template<typename T> void __lockedAtomicValue(T v) {}

/**
 * \brief State stack with single argument apply function.
 */
template<typename T> struct ValueStackAtomic
{
  /**
   * Function to apply the value.
   */
  typedef void (*ApplyValue)(T v);
  /**
   * The actual value stack.
   */
  Stack<T> stack_;
  /**
   * Function to apply the value.
   */
  ApplyValue apply_;
  /**
   * Points to actual apply function when the stack is locked.
   */
  ApplyValue lockedApply_;
  /**
   * Counts number of locks.
   */
  GLint lockCounter_;

  /**
   * @param apply apply a stack value.
   */
  ValueStackAtomic(ApplyValue apply)
  : apply_(apply), lockedApply_(apply), lockCounter_(0) {}

  /**
   * Lock this stack. Until locked push/pop is ignored.
   */
  void lock() {
    ++lockCounter_;
    apply_ = __lockedAtomicValue;
  }
  /**
   * Unlock previously locked stack.
   */
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
    }
  }
  /**
   * Push a value onto the stack.
   * @param v the value.
   */
  void push(const T &v) {
    stack_.push(v);
    apply_(v);
  }
  /**
   * Pop out last value.
   */
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(stack_.top()); }
  }
};

template<typename T> void __lockedValue(const T &v) {}
/**
 * \brief State stack with single argument apply function.
 */
template<typename T> struct ValueStack
{
  /**
   * Function to apply the value.
   */
  typedef void (*ApplyValue)(const T &v);
  /**
   * The actual value stack.
   */
  Stack<T> stack_;
  /**
   * Function to apply the value.
   */
  ApplyValue apply_;
  /**
   * Points to actual apply function when the stack is locked.
   */
  ApplyValue lockedApply_;
  /**
   * Counts number of locks.
   */
  GLint lockCounter_;

  /**
   * @param apply apply a stack value.
   */
  ValueStack(ApplyValue apply)
  : apply_(apply), lockedApply_(apply), lockCounter_(0) {}

  /**
   * Lock this stack. Until locked push/pop is ignored.
   */
  void lock() {
    ++lockCounter_;
    apply_ = __lockedValue;
  }
  /**
   * Unlock previously locked stack.
   */
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
    }
  }
  /**
   * Push a value onto the stack.
   * @param v the value.
   */
  void push(const T &v) {
    stack_.push(v);
    apply_(v);
  }
  /**
   * Pop out last value.
   */
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(stack_.top()); }
  }
};

template<typename T> void __lockedParameter(GLenum key, T v) {}
/**
 * \brief State stack with key-value apply function.
 */
template<typename T> struct ParameterStackAtomic
{
  /**
   * Function to apply the value.
   */
  typedef void (*ApplyValue)(GLenum key, T v);
  /**
   * The actual value stack.
   */
  Stack<T> stack_;
  /**
   * The parameter key.
   */
  GLenum key_;
  /**
   * Function to apply the value.
   */
  ApplyValue apply_;
  /**
   * Points to actual apply function when the stack is locked.
   */
  ApplyValue lockedApply_;
  /**
   * Counts number of locks.
   */
  GLint lockCounter_;

  /**
   * @param key the parameter key.
   * @param apply apply a stack value.
   */
  ParameterStackAtomic(GLenum key, ApplyValue apply)
  : key_(key), apply_(apply), lockedApply_(apply), lockCounter_(0) {}

  /**
   * Lock this stack. Until locked push/pop is ignored.
   */
  void lock() {
    ++lockCounter_;
    apply_ = __lockedParameter;
  }
  /**
   * Unlock previously locked stack.
   */
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
    }
  }
  /**
   * Push a value onto the stack.
   * @param v the value.
   */
  void push(const T &v) {
    stack_.push(v);
    apply_(key_,v);
  }
  /**
   * Pop out last value.
   */
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(key_,stack_.top()); }
  }
};

template<typename T> void __lockedIndexed(GLuint i, const T &v) {}
/**
 * \brief State stack with indexed apply function.
 *
 * This means there is an apply function that applies to all indices
 * and there is an apply that can apply to an ondividual index.
 */
template<typename T> struct IndexedValueStack
{
  /**
   * A stack containing stamped values.
   */
  typedef Stack< StampedValue<T> > IndexedStack;
  /**
   * Function to apply the value to all indices.
   */
  typedef void (*ApplyValue)(const T &v);
  /**
   * Function to apply the value to a single index.
   */
  typedef void (*ApplyValueIndexed)(GLuint i, const T &v);

  /**
   * Number of indices.
   */
  GLuint numIndices_;
  /**
   * A stack containing stamped values applied to all indices.
   */
  IndexedStack stack_;
  /**
   * A stack array containing stamped values applied to individual indices.
   */
  IndexedStack *stackIndex_;
  /**
   * Counts number of pushes to any stack and number of pushes to indexed stacks only.
   */
  Vec2ui counter_;

  /**
   * Function to apply the value to all indices.
   */
  ApplyValue apply_;
  /**
   * Function to apply the value to a single index.
   */
  ApplyValueIndexed applyi_;
  /**
   * Points to actual apply function when the stack is locked.
   */
  ApplyValue lockedApply_;
  /**
   * Points to actual apply function when the stack is locked.
   */
  ApplyValueIndexed lockedApplyi_;
  /**
   * Counts number of locks.
   */
  GLint lockCounter_;

  /**
   * @param maxDrawBuffers number of indices
   * @param apply apply a stack value to all indices.
   * @param applyi apply a stack value to a single index.
   */
  IndexedValueStack(GLuint maxDrawBuffers,
        ApplyValue apply, ApplyValueIndexed applyi)
  : numIndices_(maxDrawBuffers),
    apply_(apply), applyi_(applyi)
  {
    stackIndex_ = new IndexedStack[maxDrawBuffers];
    counter_.x = 0;
    counter_.y = 0;
  }
  ~IndexedValueStack()
  {
    delete []stackIndex_;
  }

  /**
   * Lock this stack. Until locked push/pop is ignored.
   */
  void lock() {
    ++lockCounter_;
    apply_ = __lockedValue;
    applyi_ = __lockedIndexed;
  }
  /**
   * Unlock previously locked stack.
   */
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
      applyi_ = lockedApplyi_;
    }
  }
  /**
   * Push a value onto the stack.
   * Applies to all indices.
   * @param v_ the value.
   */
  void push(const T &v_)
  {
    StampedValue<T> v(v_,counter_.x);
    stack_.push(v);
    apply_(v_);
    // count number of equation pushes
    counter_.x += 1;
  }
  /**
   * Push a value onto the stack with given index.
   * @param index the index.
   * @param v_ the value.
   */
  void push(GLuint index, const T &v_)
  {
    StampedValue<T> v(v_,counter_.x);
    stackIndex_[index].push(v);
    applyi_(index,v_);
    // count number of equation pushes
    counter_.x += 1;
    counter_.y += 1;
  }
  /**
   * Pop out last value that was applied to all indices.
   */
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(stack_.top().v); }
    // if there are indexed equations pushed we may
    // have to re-enable them here...
    if(counter_.y>0) {
      // Indexed states only applied with stamp>lastEqStamp
      GLuint lastStamp = (stack_.isEmpty() ? -1 : stack_.top().stamp);
      // Loop over all indexed stacks and compare stamps of top element.
      for(register GLuint i=0; i<numIndices_; ++i) {
        Stack< StampedValue<T> > &stacki = stackIndex_[i];
        if(stacki.isEmpty()) { continue; }
        const StampedValue<T>& top = stacki.top();
        if(top.stamp>lastStamp) { applyi_(i,top.v); }
      }
    }
    // count number of equation pushes
    counter_.x -= 1;
  }
  /**
   * Pop out last value at given index.
   * @param index the value index.
   */
  void pop(GLuint index) {
    Stack< StampedValue<T> > &stack = stackIndex_[index];
    stack.pop();
    // find timestamps for unindexed equation and for the last indexed equation
    GLint lastStamp = (stack_.isEmpty() ? -1 : stack_.top().stamp);
    GLint lastStampi = (stack.isEmpty() ? -1 : stack.top().stamp);
    // reset to equation with latest stamp
    if(lastStamp > lastStampi) { apply_(stack_.top().v); }
    else if(lastStampi >= 0) { applyi_(index,stack.top().v); }
    // count number of equation pushes
    counter_.x -= 1;
    counter_.y -= 1;
  }
};

} // end ogle namespace

#endif /* STATE_STACKS_H_ */
