/*
 * string-util.h
 *
 *  Created on: 06.08.2012
 *      Author: daniel
 */

#ifndef STRING_UTIL_H_
#define STRING_UTIL_H_

#include <sstream>
#include <string>
using namespace std;

namespace regen {
/**
 * Formats a string using the << operator.
 */
#define FORMAT_STRING(...)\
  ( ( dynamic_cast<ostringstream &> (\
         ostringstream() . seekp( 0, ios_base::cur ) << __VA_ARGS__ )\
    ) . str() )

/**
 * True if string starts with given prefix.
 */
bool hasPrefix(const string &s, const string &prefix);
/**
 * Removes given prefix from string.
 */
string truncPrefix(const string &s, const string &prefix);

/**
 * @param s input string
 * @return true if given string is a int number
 */
bool isInteger(const string & s);
/**
 * @param s
 * @return true if given string is a float number
 */
bool isFloat(const string & s);
/**
 * @param s input string
 * @return true if given string is a number
 */
bool isNumber(const string & s);

} // namespace

#endif /* STRING_UTIL_H_ */
