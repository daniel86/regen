/*
 * glsl-types.h
 *
 *  Created on: 04.08.2012
 *      Author: daniel
 */

#ifndef GLSL_TYPES_H_
#define GLSL_TYPES_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <string>
#include <iostream>
using namespace std;

struct GLSLUniform {
  string type;
  string name;
  unsigned int numElems;
  bool forceArray;
  GLSLUniform(const string &_type,
      const string &_name,
      unsigned int _numElems=1,
      bool _forceArray=false)
  : type(_type),
    name(_name),
    numElems(_numElems),
    forceArray(_forceArray)
  {
  }
};

/**
 * Transfer between shaders.
 */
struct GLSLTransfer {
  string type;
  string name;
  unsigned int numElems;
  bool forceArray;
  string interpolation;
  GLSLTransfer(
      const string &_type="",
      const string &_name="",
      unsigned int _numElems=1,
      bool _forceArray=false,
      const string &_interpolation="")
  : type(_type),
    name(_name),
    numElems(_numElems),
    forceArray(_forceArray),
    interpolation(_interpolation)
  {
  }
};

struct GLSLStatement {
  string statement;
  GLSLStatement(const string &statement_)
  : statement(statement_)
  {
  }
};
inline bool operator==(const GLSLStatement &a, const GLSLStatement &b) {
  return a.statement.compare(b.statement)==0;
}

struct GLSLExport {
  string name;
  string value;
  GLSLExport(const string &_name, const string &_value)
  : name(_name),
    value(_value)
  {
  }
};
typedef GLSLExport GLSLEquation;
inline bool operator==(const GLSLExport &a, const GLSLExport &b) {
  return a.name.compare(b.name)==0;
}

struct GLSLFragmentOutput {
  string type;
  string name;
  GLuint colorAttachment;
  GLSLFragmentOutput(
      const string &_type, const string &_name, GLuint _colorAttachment)
  : type(_type),
    name(_name),
    colorAttachment(_colorAttachment)
  {
  }
};

struct GLSLVariable {
  string type;
  string name;
  string value;
  GLSLVariable(
      const string &_type,
      const string &_name,
      const string &_value="")
  : type(_type),
    name(_name),
    value(_value)
  {
  }
};
typedef GLSLVariable GLSLConstant;
inline bool operator==(const GLSLVariable &a, const GLSLVariable &b) {
  return a.name.compare(b.name)==0;
}

ostream& operator<<(ostream& os, const GLSLTransfer& a);
ostream& operator<<(ostream& os, const GLSLUniform& a);
ostream& operator<<(ostream& os, const GLSLExport& a);
ostream& operator<<(ostream& os, const GLSLVariable& a);

bool operator<(const GLSLTransfer& a, const GLSLTransfer& b);
bool operator<(const GLSLUniform& a, const GLSLUniform& b);
bool operator<(const GLSLExport& a, const GLSLExport& b);
bool operator<(const GLSLVariable& a, const GLSLVariable& b);

#endif /* GLSL_TYPES_H_ */
