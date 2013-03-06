/*
 * ordered-stack.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef ORDERED_STACK_H_
#define ORDERED_STACK_H_

namespace ogle {

/**
 * A stack that allows ordering of elements.
 */
template<class T> class OrderedStack {
public:
  struct Node {
    T value;
    Node *prev;
    Node *next;
  };

  /**
   * Get the position of an element in the stack.
   */
  typedef void (*GetPosition) (
      T value,
      Node *top,
      Node **left,
      Node **right);

  OrderedStack()
  : top_(NULL), getPosition_(NULL)
  {
  }
  /**
   * Obtain position for ordered insertion.
   */
  void set_getPosition(GetPosition getPosition)
  {
    getPosition_ = getPosition;
  }
  /**
   * Sets the value for empty stacks.
   */
  void set_emptyValue(T emptyValue, bool apply=true)
  {
    emptyValue_ = emptyValue;
    if(apply) topValue_ = emptyValue_;
  }
  /**
   * Top value.
   */
  T top() {
    return topValue_;
  }
  /**
   * Top value.
   */
  const T& topConst() const {
    return topValue_;
  }
  /**
   * The top node.
   * You can iterate through the stack using this node.
   */
  Node* topNode() {
    return top_;
  }
  /**
   * Pops top value.
   * Note: be careful! references to the top node will be invalid afterwards!
   */
  void pop() {
    Node *buf = top_;
    top_ = buf->next;
    if(top_!=NULL) {
      top_->prev = NULL;
      topValue_ = top_->value;
    }
    delete buf;
  }
  /**
   * Pushes value onto the stack.
   */
  Node* push(T value) {
    Node *left = NULL, *right = NULL, *newNode;

    getPosition_(value, top_, &left, &right);

    newNode = new Node;
    newNode->value = value;
    newNode->prev = left;
    newNode->next = right;
    if(left != NULL) left->next = newNode;
    if(right != NULL) right->prev = newNode;

    if(right == top_) {
      top_ = newNode;
      topValue_ = value;
    }
    return newNode;
  }
  /**
   * Removes first occurrence of value in the stack.
   * For equality != is used.
   */
  void remove(T v) {
    Node *n = top_;
    while(n->value != v) n = n->next;

    if(n->prev != NULL) {
      n->prev->next = n->next;
      if(n->next != NULL) {
        n->next->prev = n->prev;
      }
    } else {
      top_ = n->next;
      if(top_) {
        topValue_ = top_->value;
      } else {
        topValue_ = emptyValue_;
      }
    }
    delete n;
  }
protected:
  Node *top_;
  T topValue_;
  T emptyValue_;
  GetPosition getPosition_;
private:
  OrderedStack(const OrderedStack&);
  OrderedStack& operator=(const OrderedStack&);
};

} // end ogle namespace

#endif /* ORDERED_STACK_H_ */
