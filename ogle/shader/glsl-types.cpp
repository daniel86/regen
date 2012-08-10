/*
 * glsl-types.cpp
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#include "glsl-types.h"

ostream& operator<<(ostream& os, const GLSLExport& a) {
  if(a.value.size() == 0) {
    return os << a.name;
  } else {
    return os << a.name << " = " << a.value;
  }
}
bool operator<(const GLSLExport& a, const GLSLExport& b) {
  return a.name < b.name;
}

ostream& operator<<(ostream& os, const GLSLTransfer& a) {
  if(a.numElems > 1 || a.forceArray) {
    return os << a.type << " " << a.name << "[" << a.numElems << "]";
  } else {
    return os << a.type << " " << a.name;
  }
}
bool operator<(const GLSLTransfer& a, const GLSLTransfer& b) {
  return a.name < b.name;
}

ostream& operator<<(ostream& os, const GLSLUniform& a) {
  if(a.numElems > 1 || a.forceArray) {
    return os << a.type << " " << a.name << "[" << a.numElems << "]";
  } else {
    return os << a.type << " " << a.name;
  }
}
bool operator<(const GLSLUniform& a, const GLSLUniform& b) {
  return a.name < b.name;
}

ostream& operator<<(ostream& os, const GLSLVariable& a) {
  if(a.value.size() == 0) {
    return os << a.type << " " << a.name;
  } else {
    return os << a.type << " " << a.name << " = " << a.value;
  }
}
bool operator<(const GLSLVariable& a, const GLSLVariable& b) {
  return a.name < b.name;
}
