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

#include <ogle/utility/stack.h>
#include <ogle/algebra/vector.h>

template<typename T> struct StampedValue
{
  T v;
  GLuint stamp;
  StampedValue(const T &v_, GLuint stamp_) : v(v_), stamp(stamp_) {}
};

template<typename T> void __lockedAtomicValue(T v) {}
template<typename T> struct ValueStackAtomic
{
  typedef void (*ApplyValue)(T v);
  // a stack containing stamped values applied to all indices
  Stack<T> stack_;
  // function to apply the state
  ApplyValue apply_;
  // points to actual apply function when the stack is locked
  ApplyValue lockedApply_;
  GLint lockCounter_;

  ValueStackAtomic(ApplyValue apply)
  : apply_(apply), lockedApply_(apply), lockCounter_(0) {}

  void lock() {
    ++lockCounter_;
    apply_ = __lockedAtomicValue;
  }
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
    }
  }
  void push(const T &v) {
    stack_.push(v);
    apply_(v);
  }
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(stack_.top()); }
  }
};

template<typename T> void __lockedValue(const T &v) {}
template<typename T> struct ValueStack
{
  typedef void (*ApplyValue)(const T &v);
  // a stack containing stamped values applied to all indices
  Stack<T> stack_;
  // function to apply the state
  ApplyValue apply_;
  // points to actual apply function when the stack is locked
  ApplyValue lockedApply_;
  GLint lockCounter_;

  ValueStack(ApplyValue apply)
  : apply_(apply), lockedApply_(apply), lockCounter_(0) {}

  void lock() {
    ++lockCounter_;
    apply_ = __lockedValue;
  }
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
    }
  }
  void push(const T &v) {
    stack_.push(v);
    apply_(v);
  }
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(stack_.top()); }
  }
};

template<typename T> void __lockedParameter(GLenum key, T v) {}
template<typename T> struct ParameterStackAtomic
{
  typedef void (*ApplyValue)(GLenum key, T v);
  // a stack containing stamped values applied to all indices
  Stack<T> stack_;
  GLenum key_;
  // function to apply the state
  ApplyValue apply_;
  // points to actual apply function when the stack is locked
  ApplyValue lockedApply_;
  GLint lockCounter_;

  ParameterStackAtomic(GLenum key, ApplyValue apply)
  : key_(key), apply_(apply), lockedApply_(apply), lockCounter_(0) {}

  void lock() {
    ++lockCounter_;
    apply_ = __lockedParameter;
  }
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
    }
  }
  void push(const T &v) {
    stack_.push(v);
    apply_(key_,v);
  }
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(key_,stack_.top()); }
  }
};

template<typename T> void __lockedIndexed(GLuint i, const T &v) {}
template<typename T> struct IndexedValueStack
{
  typedef Stack< StampedValue<T> > IndexedStack;
  typedef void (*ApplyValue)(const T &v);
  typedef void (*ApplyValueIndexed)(GLuint i, const T &v);

  GLuint maxDrawBuffers_;
  // a stack containing stamped values applied to all indices
  IndexedStack stack_;
  // a stack array containing stamped values applied to individual indices
  IndexedStack *stackIndex_;
  // counts number of pushes to any stack and number of pushes
  // to indexed stacks only
  Vec2ui counter_;

  ApplyValue apply_;
  ApplyValueIndexed applyi_;
  // points to actual apply function when the stack is locked
  ApplyValue lockedApply_;
  ApplyValueIndexed lockedApplyi_;
  GLint lockCounter_;

  IndexedValueStack(GLuint maxDrawBuffers,
        ApplyValue apply, ApplyValueIndexed applyi)
  : maxDrawBuffers_(maxDrawBuffers),
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

  void lock() {
    ++lockCounter_;
    apply_ = __lockedValue;
    applyi_ = __lockedIndexed;
  }
  void unlock() {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = lockedApply_;
      applyi_ = lockedApplyi_;
    }
  }
  void push(const T &v_)
  {
    StampedValue<T> v(v_,counter_.x);
    stack_.push(v);
    apply_(v_);
    // count number of equation pushes
    counter_.x += 1;
  }
  void push(GLuint index, const T &v_)
  {
    StampedValue<T> v(v_,counter_.x);
    stackIndex_[index].push(v);
    applyi_(index,v_);
    // count number of equation pushes
    counter_.x += 1;
    counter_.y += 1;
  }
  void pop() {
    stack_.pop();
    if(!stack_.isEmpty()) { apply_(stack_.top().v); }
    // if there are indexed equations pushed we may
    // have to re-enable them here...
    if(counter_.y>0) {
      // Indexed states only applied with stamp>lastEqStamp
      GLuint lastStamp = (stack_.isEmpty() ? -1 : stack_.top().stamp);
      // Loop over all indexed stacks and compare stamps of top element.
      for(register GLuint i=0; i<maxDrawBuffers_; ++i) {
        Stack< StampedValue<T> > &stacki = stackIndex_[i];
        if(stacki.isEmpty()) { continue; }
        const StampedValue<T>& top = stacki.top();
        if(top.stamp>lastStamp) { applyi_(i,top.v); }
      }
    }
    // count number of equation pushes
    counter_.x -= 1;
  }
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

#endif /* STATE_STACKS_H_ */
