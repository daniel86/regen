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

  Stack() : top_(NULL) {}
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
   * Push value to the stack bottom.
   */
  void pushBottom(const T& value)
  {
    Node *root = bottomNode();
    if(root==NULL) {
      top_ = new Node(value, NULL);
    } else {
      root->next_ = new Node(value, NULL);
    }
  }
  /**
   * Pop the top value.
   */
  void pop()
  {
    if(top_==NULL) { return; }
    Node *buf = top_;
    top_ = top_->next_;
    delete buf;
  }
  /**
   * Pops the bottom value.
   */
  void popBottom()
  {
    if(!top_ || !top_->next_) {
      // empty or single element
      pop();
      return;
    }
    for(Node *n=top_; n!=NULL; n=n->next_)
    {
      Node *root = n->next_;
      if(!root->next_) {
        n->next_ = NULL;
        delete root;
        break;
      }
    }
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
   * Bottom value node.
   */
  Node* bottomNode()
  {
    if(!top_) return NULL;
    if(!top_->next_) return top_;
    for(Node *n=top_; n!=NULL; n=n->next_)
    {
      if(!n->next_->next_) return n->next_;
    }
    return NULL;
  }
  /**
   * Returns if the stack is empty.
   */
  bool isEmpty() const
  {
    return top_==NULL;
  }
private:
  Node *top_;
};

#endif /* __STACK_H_ */
