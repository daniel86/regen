/*
 * callable.h
 *
 *  Created on: 21.03.2011
 *      Author: daniel
 */

#ifndef CALLABLE_H_
#define CALLABLE_H_

/**
 * Interface for callable objects.
 * This may be a callback for an event or something similar.
 */
class Callable {
public:
  virtual void call() = 0;
};

/**
 * Interface for callable objects.
 * This may be a callback for an event or something similar.
 * Callback has one argument.
 */
template<class T>
class Callable1 {
public:
  virtual void call(T *val) = 0;
};

/**
 * Interface for callable objects.
 * This may be a callback for an event or something similar.
 * Callback has two arguments.
 */
template<class T>
class Callable2 {
public:
  virtual void call(T *v1, void *v2) = 0;
};

#endif /* CALLABLE_H_ */
