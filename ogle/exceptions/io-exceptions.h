/*
 * file-not-found-exception.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef IO_EXCEPTIONS_H_
#define IO_EXCEPTIONS_H_

#include <string>
#include <stdexcept>
using namespace std;

class FileNotFoundException : public std::runtime_error {
public:
  FileNotFoundException(const string &message)
  : std::runtime_error(message)
  {
  }
};

#endif /* FILE_NOT_FOUND_EXCEPTION_H_ */
