/*
 * state-stacks.h
 *
 *  Created on: 26.02.2013
 *      Author: daniel
 */

#ifndef STATE_STACKS_H_
#define STATE_STACKS_H_

#include <GL/glew.h>

#include <regen/config.h>
#include <regen/utility/stack.h>
#include <regen/math/vector.h>

namespace regen {
  template<typename StackType, typename ValueType>
  static void applyFilled(StackType *s, const ValueType &v)
  {
    if(v!=s->value()) { s->apply(v); }
  }
  template<typename StackType, typename ValueType>
  static void applyInit(StackType *s, const ValueType &v)
  {
    s->apply(v);
    s->doApply_ = &applyFilled;
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
    : head_(new Node(zeroValue_)),
      isEmpty_(GL_TRUE),
      apply_(apply),
      applyPtr_(apply),
      lockedApplyPtr_(lockedApply),
      lockCounter_(0)
    {
      self_ = (StackType*)this;
      doApply_ = &applyInit;
    }
    ~StateStack()
    {
      Stack< Node* > s;
      Node *n=head_;
      while(n->prev) n = n->prev;
      for(; n!=NULL; n=n->next)
      { s.push(n); }
      while(!s.isEmpty()) {
        Node *x = s.top();
        delete x;
        s.pop();
      }
    }

    /**
     * @return the current state value or the value created by default constructor.
     */
    const ValueType& value()
    { return head_->v; }

    /**
     * Push a value onto the stack.
     * @param v the value.
     */
    void push(const ValueType &v)
    {
      doApply_(self_,v);

      if(isEmpty_) {
        // initial push to the stack or first push after everything
        // was popped out.
        head_->v = v;
        isEmpty_ = GL_FALSE;
      }
      else if(head_->next) {
        // use node created earlier
        head_ = head_->next;
        head_->v = v;
      }
      else {
        head_->next = new Node(v, head_);
        head_ = head_->next;
      }
    }

    /**
     * Pop out last value.
     */
    void pop()
    {
      if(head_->prev) {
        if(head_->v != head_->prev->v) {
          self_->apply(head_->prev->v);
        }
        head_ = head_->prev;
      } else {
        isEmpty_ = GL_TRUE;
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
    struct Node {
      Node(const ValueType &_v) : prev(NULL), next(NULL), v(_v) {}
      Node(const ValueType &_v, Node *_prev)
      : prev(_prev), next(NULL), v(_v)
      {
        _prev->next = this;
      }
      Node *prev;
      Node *next;
      ValueType v;
    };
    // cast to parent class
    StackType *self_;
    // just in case nothing was pushed
    ValueType zeroValue_;
    // The actual value stack.
    Node *head_;
    GLboolean isEmpty_;
    // Function to apply the value.
    ApplyFunc apply_;
    // Points to actual apply function when the stack is locked.
    ApplyFunc applyPtr_;
    // Points to locked apply function.
    ApplyFunc lockedApplyPtr_;
    // Counts number of locks.
    GLint lockCounter_;
    // use function pointer to avoid some if statements
    void (*doApply_)(StackType *s, const ValueType &v);

    friend void applyFilled<StackType,ValueType>(StackType*, const ValueType&);
    friend void applyInit<StackType,ValueType>(StackType*, const ValueType&);
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
     * Apply a state value.
     * @param v the value.
     */
    typedef void (*AtomicStateApply)(T v);
    /**
     * @param apply apply a stack value.
     */
    ValueStackAtomic(AtomicStateApply apply)
    : StateStack<ValueStackAtomic,T,AtomicStateApply>(apply, __lockedAtomicValue) {}
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

  template<typename T> void __lockedAtomicParameter(GLenum key, T v) {}
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
    : StateStack<ParameterStackAtomic,T,void (*)(GLenum,T)>(
        apply, __lockedAtomicParameter), key_(key) {}
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
  static void applyFilledStamped(StackType *s, const ValueType &v)
  {
    if(v!=s->head_->v) { s->apply_(v); }
  }
  template<typename StackType, typename ValueType>
  static void applyFilledStampedi(StackType *s, GLuint i, const ValueType &v)
  {
    if(v!=s->headi_[i]->v) { s->applyi_(i,v); }
  }
  template<typename StackType, typename ValueType>
  static void applyInitStamped(StackType *s, const ValueType &v)
  {
    s->apply_(v);
    s->doApply_ = &applyFilledStamped;
  }
  template<typename StackType, typename ValueType>
  static void applyInitStampedi(StackType *s, GLuint i, const ValueType &v)
  {
    s->applyi_(i,v);
    s->doApplyi_[i] = &applyFilledStampedi;
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
      head_(new Node(zeroValue_)),
      apply_(apply),
      applyi_(applyi),
      lockCounter_(0),
      lastStampi_(-1),
      isEmpty_(GL_TRUE)
    {
      head_->stamp = -1;
      counter_.x = 0;
      counter_.y = 0;
      doApply_ = &applyInitStamped;

      headi_ = new Node*[numIndices];
      doApplyi_ = new DoApplyValueIndexed[numIndices];
      isEmptyi_ = new GLboolean[numIndices];
      for(GLuint i=0; i<numIndices_; ++i) {
        doApplyi_[i] = &applyInitStampedi;
        headi_[i] = new Node(zeroValue_);
        isEmptyi_[i] = GL_TRUE;
      }
    }
    ~IndexedValueStack()
    {
      if(head_) {
        deleteNodes(head_);
      }
      if(isEmptyi_) {
        delete []isEmptyi_;
        isEmptyi_ = NULL;
      }
      if(headi_) {
        for(GLuint i=0; i<numIndices_; ++i)
        { deleteNodes(headi_[i]); }
        delete []headi_;
        headi_ = NULL;
      }
      if(doApplyi_) {
        delete []doApplyi_;
        doApplyi_ = NULL;
      }
    }

    /**
     * @return the current state value or the value created by default constructor.
     */
    const ValueType& globalValue()
    { return head_->v; }

    /**
     * Push a value onto the stack.
     * Applies to all indices.
     * @param v the value.
     */
    void push(const ValueType &v)
    {
      if(counter_.y>0) {
        // an indexed value was pushed before
        // if the indexed value was pushed before the last global push. only
        // apply when the value of last global push is not equal
        if(head_->stamp>lastStampi_)
        { doApply_(this,v); }
        // else an indexed value was pushed. always call apply even if some
        // indices may already contain the right value
        else
        { apply_(v); }
      }
      else {
        // no indexed push was done before
        doApply_(this,v);
      }

      if(isEmpty_) {
        // initial push to the stack or first push after everything
        // was popped out.
        head_->v = v;
        isEmpty_ = GL_FALSE;
      }
      else if(head_->next) {
        // use node created earlier
        head_ = head_->next;
        head_->v = v;
      }
      else {
        // first time someone pushed so deep
        head_->next = new Node(v, head_);
        head_ = head_->next;
      }
      head_->stamp = counter_.x;

      // count number of pushes for value stamps
      counter_.x += 1;
    }
    /**
     * Push a value onto the stack with given index.
     * @param index the index.
     * @param v the value.
     */
    void push(GLuint index, const ValueType &v)
    {
      Node *headi = headi_[index];

      if(counter_.x>counter_.y) {
        GLint lastStampi = (isEmptyi_[index] ? -1 : headi->stamp);
        // an global value was pushed before
        // if the global value was pushed before the last indexed push. only
        // apply when the value of last indexed push is not equal
        if(lastStampi>head_->stamp)
        { doApplyi_[index](this,index,v); }
        // else an global value was pushed. call apply if values not equal
        else if(v != head_->v)
        { applyi_(index,v); }
      }
      else {
        // no global push was done before
        doApplyi_[index](this,index,v);
      }

      if(isEmptyi_[index]) {
        // initial push to the stack or first push after everything
        // was popped out.
        headi->v = v;
        isEmptyi_[index] = GL_FALSE;
      }
      else if(headi->next) {
        // use node created earlier
        headi = headi->next;
        headi->v = v;
        headi_[index] = headi;
      }
      else {
        // first time someone pushed so deep
        headi->next = new Node(v, headi);
        headi = headi->next;
        headi_[index] = headi;
      }
      headi->stamp = counter_.y;
      lastStampi_ = counter_.y;

      // count number of pushes for value stamps
      counter_.x += 1;
      counter_.y += 1;
    }

    /**
     * Pop out last value that was applied to all indices.
     */
    void pop()
    {
      // apply previously pushed global value
      if(head_->prev) {
        if(head_->v != head_->prev->v) {
          apply_(head_->prev->v);
        }
        head_ = head_->prev;
      } else {
        isEmpty_ = GL_TRUE;
      }

      // if there are indexed values pushed we may
      // have to re-enable them here...
      if(counter_.y>0) {
        // Indexed states only applied with stamp>lastEqStamp
        // Loop over all indexed stacks and compare stamps of top element.
        for(GLuint i=0; i<numIndices_; ++i)
        {
          if(isEmptyi_[i]) { continue; }
          Node *headi = headi_[i];

          if(headi->stamp>head_->stamp && head_->v!=headi->v)
          { applyi_(i,headi->v); }
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
      Node *headi = headi_[index];
      GLboolean valueChanged;

      // apply previously pushed indexed value
      if(headi->prev) {
        valueChanged = (headi->v != headi->prev->v);
        headi = headi->prev;
        headi_[index] = headi;
        lastStampi_ = headi->stamp;
      } else {
        valueChanged = GL_FALSE;
        isEmptyi_[index] = GL_TRUE;
        lastStampi_ = -1;
      }

      GLint lastStampi = (isEmptyi_[index] ? -1 : headi->stamp);
      // reset to equation with latest stamp
      if(head_->stamp>lastStampi) {
        if(headi->v != head_->v) applyi_(index,head_->v);
      }
      else if(valueChanged)
      { applyi_(index, headi->v); }

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
    struct Node {
      Node()
      : prev(NULL), next(NULL), stamp(-1) {}
      Node(const ValueType &_v)
      : prev(NULL), next(NULL), v(_v), stamp(-1) {}
      Node(const ValueType &_v, Node *_prev)
      : prev(_prev), next(NULL), v(_v), stamp(-1)
      {
        _prev->next = this;
      }
      Node *prev;
      Node *next;
      ValueType v;
      GLint stamp;
    };

    void deleteNodes(Node *n_)
    {
      Stack< Node* > s;
      Node *n=n_;
      while(n->prev) n=n->prev;
      for(; n!=NULL; n=n->next)
      { s.push(n); }
      while(!s.isEmpty()) {
        Node *x = s.top();
        delete x;
        s.pop();
      }
    }

    // Number of indices.
    GLuint numIndices_;
    // A stack containing stamped values applied to all indices.
    Node *head_;
    // A stack array containing stamped values applied to individual indices.
    Node **headi_;
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

    GLint lastStampi_;

    GLboolean isEmpty_;
    GLboolean *isEmptyi_;

    typedef void (*DoApplyValue)(IndexedValueStack*, const ValueType&);
    typedef void (*DoApplyValueIndexed)(IndexedValueStack*, GLuint, const ValueType&);
    // use function pointer to avoid some if statements
    DoApplyValue doApply_;
    DoApplyValueIndexed *doApplyi_;

    friend void applyInitStamped< IndexedValueStack, ValueType >
        (IndexedValueStack*, const ValueType&);
    friend void applyFilledStamped< IndexedValueStack, ValueType >
        (IndexedValueStack*, const ValueType&);
    friend void applyInitStampedi< IndexedValueStack, ValueType >
        (IndexedValueStack*, GLuint, const ValueType&);
    friend void applyFilledStampedi< IndexedValueStack, ValueType >
        (IndexedValueStack*, GLuint, const ValueType&);
  };
} // namespace

#endif /* STATE_STACKS_H_ */
