/*
 * stack.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef __STACK_H_
#define __STACK_H_

/**
 * A simple stack implementation.
 */
template<class T> class Stack
{
public:
  /**
   * A Node in the stack.
   */
  class Node {
  public:
      Node(const T &value, Node *next)
      : value_(value), next_(next) {}
      T value_;
      Node *next_;
  };

  Stack()
  : top_(NULL)
  {
  }
  /**
   * Sets top value.
   */
  void set_value(const T& value)
  {
    top_->value_ = value;
  }
  /**
   * Push value onto the stack.
   */
  void push(const T& value)
  {
    top_ = new Node(value, top_);
  }
  /**
   * Pop the top value.
   */
  void pop()
  {
    Node *buf = top_;
    top_ = top_->next_;
    delete buf;
  }
  /**
   * Top value.
   */
  const T& top() const
  {
    return top_->value_;
  }
  /**
   * Top value.
   */
  T& topPtr()
  {
    return top_->value_;
  }
  /**
   * Top value node.
   * You can use the node to iterate through the stack.
   */
  Node* topNode()
  {
    return top_;
  }
  /**
   * Returns if the stack is empty.
   */
  bool isEmpty()
  {
    return top_==NULL;
  }
private:
  Node *top_;
};

#endif /* __STACK_H_ */
