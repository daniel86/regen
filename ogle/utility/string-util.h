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

/**
 * Formats a string using the << operator.
 */
#define FORMAT_STRING(...)\
  ( ( dynamic_cast<ostringstream &> (\
         ostringstream() . seekp( 0, ios_base::cur ) << __VA_ARGS__ )\
    ) . str() )

bool hasPrefix(const string &s, const string &prefix);
string truncPrefix(const string &s, const string &prefix);

bool isInteger(const string & s);
bool isFloat(const string & s);
bool isNumber(const string & s);

unsigned int getNumLines(const string &s);
unsigned int getFirstLine(const string &s);

void replaceVariable(
    const string &fromName,
    const string &toName,
    string *code);

/**
 * Removes undefined code.
 * No brackets supported yet.
 */
string evaluateMacros(const string &code);

#endif /* STRING_UTIL_H_ */
