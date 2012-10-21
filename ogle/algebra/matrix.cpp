/*
 * matrix.cpp
 *
 *  Created on: 20.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <string>

#include "matrix.h"

/*
template <typename T>
static int parseMatf(const string &valStr, T &val)
{
  list<string> v;
  boost::split(v, valStr, boost::is_any_of(","));
  GLfloat *data = val.x;
  int count=sizeof(val)/sizeof(GLfloat);
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==count) { break; }
    if(parseVec1f(*it,data[i])!=0) { break; }
    ++i;
  }
  return 0;
}

int parseMat3f(const string valueString, Mat3f &val)
{
  return parseMatf(valueString,val);
}
int parseMat4f(const string valueString, Mat4f &val)
{
  return parseMatf(valueString,val);
}
*/

istream& operator>>(istream& in, Mat3f &m)
{
  return in >>
      m.x[0] >> m.x[1] >> m.x[2] >>
      m.x[3] >> m.x[4] >> m.x[5] >>
      m.x[6] >> m.x[7] >> m.x[8];
}
istream& operator>>(istream& in, Mat4f &m)
{
  return in >>
      m.x[ 0] >> m.x[ 1] >> m.x[ 2] >> m.x[ 3] >>
      m.x[ 4] >> m.x[ 5] >> m.x[ 6] >> m.x[ 7] >>
      m.x[ 8] >> m.x[ 9] >> m.x[10] >> m.x[11] >>
      m.x[12] >> m.x[13] >> m.x[14] >> m.x[15];
}

ostream& operator<<(ostream& os, const Mat3f& m)
{
  return os <<
      m.x[0] << ", " << m.x[1] << ", " << m.x[2] << endl <<
      m.x[3] << ", " << m.x[4] << ", " << m.x[5] << endl <<
      m.x[6] << ", " << m.x[7] << ", " << m.x[8];
}
ostream& operator<<(ostream& os, const Mat4f& m)
{
  return os <<
      m.x[0] << ", " << m.x[1] << ", " << m.x[2] << ", " << m.x[3] << ", " <<
      m.x[4] << ", " << m.x[5] << ", " << m.x[6] << ", " << m.x[7] << ", " <<
      m.x[8] << ", " << m.x[9] << ", " << m.x[10] << ", " << m.x[11] << ", " <<
      m.x[12] << ", " << m.x[13] << ", " << m.x[14] << ", " << m.x[15];
}
