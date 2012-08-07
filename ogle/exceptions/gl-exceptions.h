/*
 * gl-exceptions.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef GL_EXCEPTIONS_H_
#define GL_EXCEPTIONS_H_

#include <string>
#include <stdexcept>
using namespace std;

class ExtensionUnsupported
: public std::runtime_error
{
public:
  ExtensionUnsupported(const string &ext)
  : std::runtime_error(ext)
  {
  }
};

#endif /* GL_EXCEPTIONS_H_ */
