/*
 * vector.cpp
 *
 *  Created on: 30.01.2011
 *      Author: daniel
 */

#include <boost/algorithm/string.hpp>
#include <iostream>
#include <fstream>
#include <string>

#include "vector.h"

const Vec3f UP_VECTOR = (Vec3f) { 0.0, 1.0, 0.0 };

int parseVec1b(const string valueString, GLboolean &val)
{
  val = (
      valueString == "1" ||
      valueString == "true" ||
      valueString == "TRUE" ||
      valueString == "y" ||
      valueString == "yes") ? GL_TRUE : GL_FALSE;
  return 0;
}
int parseVec1f(const string valueString, GLfloat &val)
{
  char *pEnd;
  val = strtof(valueString.c_str(), &pEnd);
  return 0;
}
int parseVec1d(const string valueString, GLdouble &val)
{
  char *pEnd;
  val = strtod(valueString.c_str(), &pEnd);
  return 0;
}
int parseVec1i(const string valueString, GLint &val)
{
  char *pEnd;
  val = strtoul(valueString.c_str(), &pEnd, 0);
  return 0;
}
int parseVec1ui(const string valueString, GLuint &val)
{
  char *pEnd;
  val = strtoul(valueString.c_str(), &pEnd, 0);
  return 0;
}

template <typename T>
int parseVecf(const string &valStr, T &val)
{
  list<string> v;
  boost::split(v, valStr, boost::is_any_of(","));
  GLfloat *data = &val.x;
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
template <typename T>
int parseVecd(const string &valStr, T &val)
{
  list<string> v;
  boost::split(v, valStr, boost::is_any_of(","));
  GLdouble *data = &val.x;
  int count=sizeof(val)/sizeof(GLdouble);
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==count) { break; }
    if(parseVec1d(*it,data[i])!=0) { break; }
    ++i;
  }
  return 0;
}
template <typename T>
int parseVeci(const string &valStr, T &val)
{
  list<string> v;
  boost::split(v, valStr, boost::is_any_of(","));
  GLint *data = &val.x;
  int count=sizeof(val)/sizeof(GLint);
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==count) { break; }
    if(parseVec1i(*it,data[i])!=0) { break; }
    ++i;
  }
  return 0;
}
template <typename T>
int parseVecb(const string &valStr, T &val)
{
  list<string> v;
  boost::split(v, valStr, boost::is_any_of(","));
  GLboolean *data = &val.x;
  int count=sizeof(val)/sizeof(GLboolean);
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==count) { break; }
    if(parseVec1b(*it,data[i])!=0) { break; }
    ++i;
  }
  return 0;
}
template <typename T>
int parseVecui(const string &valStr, T &val)
{
  list<string> v;
  boost::split(v, valStr, boost::is_any_of(","));
  GLuint *data = &val.x;
  int count=sizeof(val)/sizeof(GLuint);
  int i=0;
  for(list<string>::iterator it=v.begin(); it!=v.end(); ++it)
  {
    if(i==count) { break; }
    if(parseVec1ui(*it,data[i])!=0) { break; }
    ++i;
  }
  return 0;
}

int parseVec2f(const string valueString, Vec2f &val) {
  return parseVecf(valueString,val);
}
int parseVec3f(const string valueString, Vec3f &val) {
  return parseVecf(valueString,val);
}
int parseVec4f(const string valueString, Vec4f &val) {
  return parseVecf(valueString,val);
}

int parseVec2d(const string valueString, Vec2d &val) {
  return parseVecd(valueString,val);
}
int parseVec3d(const string valueString, Vec3d &val) {
  return parseVecd(valueString,val);
}
int parseVec4d(const string valueString, Vec4d &val) {
  return parseVecd(valueString,val);
}

int parseVec2i(const string valueString, Vec2i &val) {
  return parseVeci(valueString,val);
}
int parseVec3i(const string valueString, Vec3i &val) {
  return parseVeci(valueString,val);
}
int parseVec4i(const string valueString, Vec4i &val) {
  return parseVeci(valueString,val);
}

int parseVec2b(const string valueString, Vec2b &val) {
  return parseVecb(valueString,val);
}
int parseVec3b(const string valueString, Vec3b &val) {
  return parseVecb(valueString,val);
}
int parseVec4b(const string valueString, Vec4b &val) {
  return parseVecb(valueString,val);
}

int parseVec2ui(const string valueString, Vec2ui &val) {
  return parseVecui(valueString,val);
}
int parseVec3ui(const string valueString, Vec3ui &val) {
  return parseVecui(valueString,val);
}
int parseVec4ui(const string valueString, Vec4ui &val) {
  return parseVecui(valueString,val);
}


ostream& operator<<(ostream& os, const Vec2f& v)
{
  return os << v.x << ", " << v.y;
}
ostream& operator<<(ostream& os, const Vec3f& v)
{
  return os << v.x << ", " << v.y << ", " << v.z;
}
ostream& operator<<(ostream& os, const Vec4f& v)
{
  return os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
}

ostream& operator<<(ostream& os, const Vec2d& v)
{
  return os << v.x << ", " << v.y;
}
ostream& operator<<(ostream& os, const Vec3d& v)
{
  return os << v.x << ", " << v.y << ", " << v.z;
}
ostream& operator<<(ostream& os, const Vec4d& v)
{
  return os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
}

ostream& operator<<(ostream& os, const Vec2i& v)
{
  return os << v.x << ", " << v.y;
}
ostream& operator<<(ostream& os, const Vec3i& v)
{
  return os << v.x << ", " << v.y << ", " << v.z;
}
ostream& operator<<(ostream& os, const Vec4i& v)
{
  return os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
}

ostream& operator<<(ostream& os, const Vec2ui& v)
{
  return os << v.x << ", " << v.y;
}
ostream& operator<<(ostream& os, const Vec3ui& v)
{
  return os << v.x << ", " << v.y << ", " << v.z;
}
ostream& operator<<(ostream& os, const Vec4ui& v)
{
  return os << v.x << ", " << v.y << ", " << v.z << ", " << v.w;
}

