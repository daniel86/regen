/*
 * string-util.h
 *
 *  Created on: 06.08.2012
 *      Author: daniel
 */

#ifndef STRING_UTIL_H_
#define STRING_UTIL_H_

#include <boost/algorithm/string.hpp>
#include <sstream>
#include <string>
using namespace std;

namespace regen {
  /**
   * Formats a string using the << operator.
   */
  #define REGEN_STRING(...)\
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

  /**
   * Reads a typed value from the input stream.
   * The value type must implement >> operator.
   * @param in the input stream
   * @param v the value output
   */
  template<typename T> void readValue(istream& in, T &v, const char separator=',')
  {
    if(!in.good()) return;
    string val;
    std::getline(in, val, separator);
    boost::algorithm::trim(val);
    stringstream ss(val);
    ss >> v;
  }
} // namespace

#endif /* STRING_UTIL_H_ */
