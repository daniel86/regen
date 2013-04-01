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
    doApply_ = &applyFilled;

    stack_.push(v);
  }

  /**
   * Pop out last value.
   */
  void pop()
  {
    typename Stack<ValueType>::Node *top = stack_.topNode();
    typename Stack<ValueType>::Node *next = top->next_;
    if(next) {
      if(stack_.top() != next->value_) { self_->apply(next->value_); }
      stack_.pop();
    }
    else { // last value. keep the node until next push
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

template<typename T> void __lockedAtomicValue(T v) {}
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

template<typename T> void __lockedValue(const T &v) {}
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

template<typename T> void __lockedParameter(GLenum key, T v) {}
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

///////////
///////////

template<typename StackType, typename ValueType>
static inline void applyInitStamped(StackType *s, const ValueType &v)
{
  s->apply_(v);
}
template<typename StackType, typename ValueType>
static inline void applyEmptyStamped(StackType *s, const ValueType &v)
{
  if(v!=s->lastNode_->value_.v) { s->apply_(v); }
  delete s->lastNode_;
  s->lastNode_ = NULL;
}
template<typename StackType, typename ValueType>
static inline void applyFilledStamped(StackType *s, const ValueType &v)
{
  if(v!=s->stack_.top().v) { s->apply_(v); }
}
template<typename StackType, typename ValueType>
static inline void applyInitStampedi(StackType *s, GLuint i, const ValueType &v)
{
  s->applyi_(i,v);
}
template<typename StackType, typename ValueType>
static inline void applyEmptyStampedi(StackType *s, GLuint i, const ValueType &v)
{
  if(v!=s->lastNodei_[i]->value_.v) { s->applyi_(i,v); }
  delete s->lastNodei_[i];
  s->lastNodei_[i] = NULL;
}
template<typename StackType, typename ValueType>
static inline void applyFilledStampedi(StackType *s, GLuint i, const ValueType &v)
{
  if(v!=s->stackIndex_[i].top().v) { s->applyi_(i,v); }
}

template<typename T> void __lockedIndexed(GLuint i, const T &v) {}
/**
 * \brief State stack with indexed apply function.
 *
 * This means there is an apply function that applies to all indices
 * and there is an apply that can apply to an ondividual index.
 */
template<typename ValueType> class IndexedValueStack
{
public:
  /**
   * Function to apply the value to all indices.
   */
  typedef void (*ApplyValue)(const ValueType &v);
  /**
   * Function to apply the value to a single index.
   */
  typedef void (*ApplyValueIndexed)(GLuint i, const ValueType &v);

  /**
   * @param numIndices number of indices
   * @param apply apply a stack value to all indices.
   * @param applyi apply a stack value to a single index.
   */
  IndexedValueStack(GLuint numIndices, ApplyValue apply, ApplyValueIndexed applyi)
  : numIndices_(numIndices),
    apply_(apply),
    applyi_(applyi),
    lastNode_(NULL)
  {
    stackIndex_ = new IndexedStack[numIndices];
    counter_.x = 0;
    counter_.y = 0;
    doApply_ = &applyInitStamped;
    doApplyi_ = new DoApplyValueIndexed[numIndices];
    lastNodei_ = new IndexedStackNode*[numIndices];
    for(GLuint i=0; i<numIndices_; ++i) {
      doApplyi_[i] = &applyInitStampedi;
      lastNodei_[i] = NULL;
    }
    lastStamp_.push(-1);
    lastStampi_.push(-1);
  }
  ~IndexedValueStack()
  {
    if(lastNode_) {
      delete lastNode_;
      lastNode_ = NULL;
    }
    if(stackIndex_) {
      delete []stackIndex_;
      stackIndex_ = NULL;
    }
    if(doApplyi_) {
      delete []doApplyi_;
      doApplyi_ = NULL;
    }
    if(lastNodei_) {
      for(GLuint i=0; i<numIndices_; ++i) {
        if(lastNodei_[i]) {
          delete lastNodei_[i];
          lastNodei_[i] = NULL;
        }
      }
      delete []lastNodei_;
      lastNodei_ = NULL;
    }
  }

  /**
   * @return the current state value or the value created by default constructor.
   */
  const ValueType& globalValue()
  {
    if(!stack_.isEmpty()) {
      return stack_.top().v;
    } else if(lastNode_) {
      return lastNode_->value_.v;
    } else {
      return zeroValue_;
    }
  }

  /**
   * Push a value onto the stack.
   * Applies to all indices.
   * @param v_ the value.
   */
  void push(const ValueType &v_)
  {
    StampedValue<ValueType> v(v_,counter_.x);

    if(counter_.y>0) {
      // an indexed value was pushed before
      // if the indexed value was pushed before the last global push. only
      // apply when the value of last global push is not equal
      if(lastStamp_.top()>lastStampi_.top())
      { doApply_(this,v_); }
      // else an indexed value was pushed. always call apply even if some
      // indices may already contain the right value
      else
      { apply_(v_); }
    }
    else {
      // no indexed push was done before
      doApply_(this,v_);
    }
    doApply_ = &applyFilledStamped;

    stack_.push(v);
    lastStamp_.push(v.stamp);

    // count number of pushes for value stamps
    counter_.x += 1;
  }
  /**
   * Push a value onto the stack with given index.
   * @param index the index.
   * @param v_ the value.
   */
  void push(GLuint index, const ValueType &v_)
  {
    StampedValue<ValueType> v(v_,counter_.y);
    IndexedStack &stacki = stackIndex_[index];

    if(counter_.x>counter_.y) {
      GLint lastStampi = (stacki.isEmpty() ? -1 : stacki.top().stamp);
      // an global value was pushed before
      // if the global value was pushed before the last indexed push. only
      // apply when the value of last indexed push is not equal
      if(lastStampi>lastStamp_.top())
      { doApplyi_[index](this,index,v_); }
      // else an global value was pushed. call apply if values not equal
      else if(v_ != globalValue())
      { applyi_(index,v_); }
    }
    else {
      // no global push was done before
      doApplyi_[index](this,index,v_);
    }
    doApplyi_[index] = &applyFilledStampedi;

    stacki.push(v);
    lastStampi_.push(v.stamp);

    // count number of pushes for value stamps
    counter_.x += 1;
    counter_.y += 1;
  }

  /**
   * Pop out last value that was applied to all indices.
   */
  void pop()
  {
    IndexedStackNode *top = stack_.topNode();
    IndexedStackNode *next = top->next_;
    ValueType *globalValue;

    // apply previously pushed global value
    if(next) {
      globalValue = &next->value_.v;

      if(stack_.top().v != (*globalValue)) { apply_(*globalValue); }
      stack_.pop();
    }
    else { // last value. keep the node until next push
      globalValue = &top->value_.v;

      stack_.popKeepNode();
      lastNode_ = top;
      doApply_ = &applyEmptyStamped;
    }
    lastStamp_.pop();

    // if there are indexed values pushed we may
    // have to re-enable them here...
    if(counter_.y>0) {
      // Indexed states only applied with stamp>lastEqStamp
      // Loop over all indexed stacks and compare stamps of top element.
      for(register GLuint i=0; i<numIndices_; ++i)
      {
        IndexedStack &stacki = stackIndex_[i];
        if(stacki.isEmpty()) { continue; }

        const StampedValue<ValueType>& top = stacki.top();
        if((GLint)top.stamp>lastStamp_.top() && (*globalValue)!=top.v)
        { applyi_(i,top.v); }
      }
    }

    // count number of pushes for value stamps
    counter_.x -= 1;
  }
  /**
   * Pop out last value at given index.
   * @param index the value index.
   */
  void pop(GLuint index)
  {
    IndexedStack &stack = stackIndex_[index];
    IndexedStackNode *top = stack.topNode();
    IndexedStackNode *next = top->next_;
    ValueType *indexedValue;
    GLboolean valueChanged;

    // apply previously pushed indexed value
    if(next) {
      indexedValue = &next->value_.v;

      valueChanged = (top->value_.v != (*indexedValue));
      stack.pop();
    }
    else { // last value. keep the node until next push
      indexedValue = &top->value_.v;

      valueChanged = GL_FALSE;
      stack.popKeepNode();
      lastNodei_[index] = top;
      doApplyi_[index] = &applyEmptyStampedi;
    }
    lastStampi_.pop();

    GLint lastStampi = (stack.isEmpty() ? -1 : stack.top().stamp);
    // reset to equation with latest stamp
    if(lastStamp_.top() > lastStampi) {
      const ValueType &globalValue = stack_.top().v;
      if((*indexedValue) != globalValue) applyi_(index,globalValue);
    }
    else if(valueChanged)
    { applyi_(index, *indexedValue); }

    // count number of pushes for value stamps
    counter_.x -= 1;
    counter_.y -= 1;
  }

  /**
   * Lock this stack. Until locked push/pop is ignored.
   */
  void lock()
  {
    ++lockCounter_;
    apply_ = __lockedValue;
    applyi_ = __lockedIndexed;
  }
  /**
   * Unlock previously locked stack.
   */
  void unlock()
  {
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

protected:
  typedef Stack< StampedValue<ValueType> > IndexedStack;
  typedef typename IndexedStack::Node IndexedStackNode;

  // Number of indices.
  GLuint numIndices_;
  // A stack containing stamped values applied to all indices.
  IndexedStack stack_;
  // A stack array containing stamped values applied to individual indices.
  IndexedStack *stackIndex_;
  // Counts number of pushes to any stack and number of pushes to indexed stacks only.
  Vec2ui counter_;

  ValueType zeroValue_;

  // Function to apply the value to all indices.
  ApplyValue apply_;
  // Function to apply the value to a single index.
  ApplyValueIndexed applyi_;
  // Points to actual apply function when the stack is locked.
  ApplyValue lockedApply_;
  // Points to actual apply function when the stack is locked.
  ApplyValueIndexed lockedApplyi_;

  // Counts number of locks.
  GLint lockCounter_;

  Stack<GLint> lastStamp_;
  Stack<GLint> lastStampi_;

  // keep last node for empty stacks
  IndexedStackNode *lastNode_;
  IndexedStackNode **lastNodei_;

  typedef void (*DoApplyValue)(IndexedValueStack*, const ValueType&);
  typedef void (*DoApplyValueIndexed)(IndexedValueStack*, GLuint, const ValueType&);
  // use function pointer to avoid some if statements
  DoApplyValue doApply_;
  DoApplyValueIndexed *doApplyi_;

  friend void applyInitStamped< IndexedValueStack, ValueType >
      (IndexedValueStack*, const ValueType&);
  friend void applyEmptyStamped< IndexedValueStack, ValueType >
      (IndexedValueStack*, const ValueType&);
  friend void applyFilledStamped< IndexedValueStack, ValueType >
      (IndexedValueStack*, const ValueType&);
  friend void applyInitStampedi< IndexedValueStack, ValueType >
      (IndexedValueStack*, GLuint, const ValueType&);
  friend void applyEmptyStampedi< IndexedValueStack, ValueType >
      (IndexedValueStack*, GLuint, const ValueType&);
  friend void applyFilledStampedi< IndexedValueStack, ValueType >
      (IndexedValueStack*, GLuint, const ValueType&);
};

} // namespace

#endif /* STATE_STACKS_H_ */
