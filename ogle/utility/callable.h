/*
 * callable.h
 *
 *  Created on: 21.03.2011
 *      Author: daniel
 */

#ifndef CALLABLE_H_
#define CALLABLE_H_

namespace ogle {

/**
 * Interface for callable objects.
 * This may be a callback for an event or something similar.
 */
class Callable {
public:
  virtual ~Callable() {};
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
  virtual ~Callable1() {};
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
  virtual ~Callable2() {};
  virtual void call(T *v1, void *v2) = 0;
};

} // end ogle namespace

#endif /* CALLABLE_H_ */
