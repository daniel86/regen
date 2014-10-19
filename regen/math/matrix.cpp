/*
 * matrix.cpp
 *
 *  Created on: 20.10.2012
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "matrix.h"
namespace regen {

#define READ_VEC(v) if(in.good()){\
    std::string val;\
    std::getline(in, val, ',');\
    boost::algorithm::trim(val);\
    std::stringstream ss(val);\
    ss >> v;\
}

std::istream& operator>>(std::istream& in, Mat3f &m)
{
  READ_VEC(m.x[ 0]); READ_VEC(m.x[ 1]); READ_VEC(m.x[ 2]);
  READ_VEC(m.x[ 3]); READ_VEC(m.x[ 4]); READ_VEC(m.x[ 5]);
  READ_VEC(m.x[ 6]); READ_VEC(m.x[ 7]); READ_VEC(m.x[ 8]);
  return in;
}
std::istream& operator>>(std::istream& in, Mat4f &m)
{
  READ_VEC(m.x[ 0]); READ_VEC(m.x[ 1]); READ_VEC(m.x[ 2]); READ_VEC(m.x[ 3]);
  READ_VEC(m.x[ 4]); READ_VEC(m.x[ 5]); READ_VEC(m.x[ 6]); READ_VEC(m.x[ 7]);
  READ_VEC(m.x[ 8]); READ_VEC(m.x[ 9]); READ_VEC(m.x[10]); READ_VEC(m.x[11]);
  READ_VEC(m.x[12]); READ_VEC(m.x[13]); READ_VEC(m.x[14]); READ_VEC(m.x[15]);
  return in;
}

#undef READ_VEC

std::ostream& operator<<(std::ostream& os, const Mat3f& m)
{
  return os <<
      m.x[0] << ", " << m.x[1] << ", " << m.x[2] << std::endl <<
      m.x[3] << ", " << m.x[4] << ", " << m.x[5] << std::endl <<
      m.x[6] << ", " << m.x[7] << ", " << m.x[8];
}
std::ostream& operator<<(std::ostream& os, const Mat4f& m)
{
  return os <<
      m.x[0] << ", " << m.x[1] << ", " << m.x[2] << ", " << m.x[3] << ", " <<
      m.x[4] << ", " << m.x[5] << ", " << m.x[6] << ", " << m.x[7] << ", " <<
      m.x[8] << ", " << m.x[9] << ", " << m.x[10] << ", " << m.x[11] << ", " <<
      m.x[12] << ", " << m.x[13] << ", " << m.x[14] << ", " << m.x[15];
}
}
