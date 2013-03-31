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

namespace regen {

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

template<typename StackType, typename ValueType>
static inline void applyInit(StackType *s, const ValueType &v)
{
  s->apply(v);
}
template<typename StackType, typename ValueType>
static inline void applyEmpty(StackType *s, const ValueType &v)
{
  if(v!=s->lastNode_->value_) { s->apply(v); }
  delete s->lastNode_;
  s->lastNode_ = NULL;
}
template<typename StackType, typename ValueType>
static inline void applyFilled(StackType *s, const ValueType &v)
{
  if(v!=s->stack_.top()) { s->apply(v); }
}

/**
 * \brief A stack that keeps track of a state value.
 *
 * Redundant state switches are avoided.
 * ValueType must implement default constructor ValueType() and
 * the != operator.
 * The stack can be locked. Pushes do not change the state value until
 * the stack is locked.
 */
template<typename StackType, typename ValueType, typename ApplyFunc> class StateStack
{
public:
  /**
   * @param apply apply a stack value.
   * @param lockedApply apply a stack value in locked mode.
   */
  StateStack(ApplyFunc apply, ApplyFunc lockedApply)
  : apply_(apply),
    applyPtr_(apply),
    lockedApplyPtr_(lockedApply),
    lockCounter_(0),
    lastNode_(NULL)
  {
    self_ = (StackType*)this;
    doApply_ = &applyInit;
  }
  ~StateStack()
  {
    if(lastNode_) {
      delete lastNode_;
      lastNode_ = NULL;
    }
  }

  /**
   * @return the current state value or the value created by default constructor.
   */
  const ValueType& value()
  {
    if(!stack_.isEmpty()) {
      return stack_.top();
    } else if(lastNode_) {
      return lastNode_->value_;
    } else {
      return zeroValue_;
    }
  }

  /**
   * Push a value onto the stack.
   * @param v the value.
   */
  void push(const ValueType &v)
  {
    doApply_(self_,v);
    stack_.push(v);
    doApply_ = &applyFilled;
  }

  /**
   * Pop out last value.
   */
  void pop()
  {
    typename Stack<ValueType>::Node *top = stack_.topNode();
    typename Stack<ValueType>::Node *next = top->next_;
    if(next) {
      if(stack_.top() != next->value_) {
        self_->apply(next->value_);
      }
      stack_.pop();
    } else {
      // last value. keep the node until next push
      stack_.popKeepNode();
      lastNode_ = top;
      doApply_ = &applyEmpty;
    }
  }

  /**
   * Lock this stack. Until locked push/pop is ignored.
   */
  void lock()
  {
    ++lockCounter_;
    apply_ = lockedApplyPtr_;
  }

  /**
   * Unlock previously locked stack.
   */
  void unlock()
  {
    --lockCounter_;
    if(lockCounter_<1) {
      apply_ = applyPtr_;
    }
  }

  /**
   * @return true if the stack is locked.
   */
  GLboolean isLocked() const
  {
    return lockCounter_>0;
  }

protected:
  // cast to parent class
  StackType *self_;
  // just in case nothing was pushed
  ValueType zeroValue_;
  // The actual value stack.
  Stack<ValueType> stack_;
  // Function to apply the value.
  ApplyFunc apply_;
  // Points to actual apply function when the stack is locked.
  ApplyFunc applyPtr_;
  // Points to locked apply function.
  ApplyFunc lockedApplyPtr_;
  // Counts number of locks.
  GLint lockCounter_;
  // keep last node for empty stacks
  typename Stack<ValueType>::Node *lastNode_;
  // use function pointer to avoid some if statements
  void (*doApply_)(StackType *s, const ValueType &v);

  friend void applyEmpty<StackType,ValueType>(StackType*, const ValueType&);
  friend void applyFilled<StackType,ValueType>(StackType*, const ValueType&);
};

template<typename T> void __lockedValue(const T &v) {}
template<typename T> void __lockedAtomicValue(T v) {}
template<typename T> void __lockedParameter(GLenum key, T v) {}

/**
 * \brief State stack with single argument apply function.
 */
template<typename T> class ValueStackAtomic
: public StateStack<ValueStackAtomic<T>,T,void (*)(T)>
{
public:
  /**
   * @param apply apply a stack value.
   */
  ValueStackAtomic(void (*apply)(T v))
  : StateStack<ValueStackAtomic,T,void (*)(T)>(apply, __lockedAtomicValue) {}
  /**
   * @param v the new state value
   */
  void apply(const T &v) { this->apply_(v); }
};

/**
 * \brief State stack with single argument apply function.
 */
template<typename T> class ValueStack
: public StateStack<ValueStack<T>,T,void (*)(const T&)>
{
public:
  /**
   * @param apply apply a stack value.
   */
  ValueStack(void (*apply)(const T &v))
  : StateStack<ValueStack,T,void (*)(const T&)>(apply, __lockedValue) {}
  /**
   * @param v the new state value
   */
  void apply(const T &v) { this->apply_(v); }
};

/**
 * \brief State stack with key-value apply function.
 *
 * key is first function argument value the second.
 */
template<typename T> class ParameterStackAtomic
: public StateStack<ParameterStackAtomic<T>,T,void (*)(GLenum,T)>
{
public:
  /**
   * @param key the parameter key.
   * @param apply apply a stack value.
   */
  ParameterStackAtomic(GLenum key, void (*apply)(GLenum,T))
  : StateStack<ParameterStackAtomic,T,void (*)(GLenum,T)>(apply, __lockedParameter), key_(key) {}
  /**
   * @param v the new state value
   */
  void apply(const T &v) { this->apply_(key_,v); }
protected:
  GLenum key_;
};

// TODO: avoid redundant calls
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
   * @return true if the stack is locked.
   */
  GLboolean isLocked() const
  {
    return lockCounter_>0;
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
    if(!stack_.isEmpty()) {
      apply_(stack_.top().v);
    }
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
        if(top.stamp>lastStamp) {
          applyi_(i,top.v);
        }
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

} // namespace

#endif /* STATE_STACKS_H_ */
