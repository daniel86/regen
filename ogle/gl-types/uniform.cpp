/*
 * uniform.cpp
 *
 *  Created on: 02.04.2011
 *      Author: daniel
 */

#include "uniform.h"

Uniform::Uniform(
    const string &name,
    GLenum dataType,
    GLuint dataTypeBytes,
    GLuint valsPerElement,
    GLboolean normalize,
    GLuint elementCount,
    GLboolean isIntegerType)
: name_(name),
  dataType_(dataType),
  dataTypeBytes_(dataTypeBytes),
  valsPerElement_(valsPerElement),
  normalize_(normalize),
  count_(elementCount),
  numInstances_(1),
  isIntegerType_(isIntegerType)
{
  pushNewAttribute();
}
Uniform::~Uniform()
{
}

GLuint Uniform::numInstances() const
{
  return numInstances_;
}
GLuint Uniform::count() const
{
  return count_;
}

ref_ptr<VertexAttribute>& Uniform::attribute()
{
  return valueStack_.topPtr();
}

void Uniform::set_name(const string &s)
{
  name_ = s;
  UniformStack::Stack::Node *n = valueStack_.topNode();
  for(; n!=NULL; n=n->next_) {
    n->value_->set_name(s);
  }
}

void Uniform::pushNewAttribute()
{
  ref_ptr<VertexAttribute> att_;
  if(isIntegerType_) {
    att_ = ref_ptr<VertexAttribute>::manage( new VertexAttributeI(
        name_, dataType_, dataTypeBytes_, valsPerElement_, count_) );
  } else {
    att_ = ref_ptr<VertexAttribute>::manage( new VertexAttribute(
        name_, dataType_, dataTypeBytes_, valsPerElement_, normalize_, count_) );
  }
  valueStack_.push(att_);
}

UniformInt::UniformInt(const string &name,
    const GLuint count)
: ValueUniform<GLint>(name, GL_INT,
    sizeof(GLint), 1, false, count, true)
{
}
UniformInt::UniformInt(const string &name,
    const GLuint count, const GLint &val)
: ValueUniform<GLint>(name, GL_INT,
    sizeof(GLint), 1, false, count, true)
{
  set_value(val);
}
void UniformInt::apply(GLint loc) const
{
  glUniform1iv(loc, count_, &value());
}

UniformIntV2::UniformIntV2(const string &name,
    const GLuint count)
: ValueUniform<Vec2i>(name, GL_INT,
    sizeof(GLint), 2, false, count, true)
{
}
UniformIntV2::UniformIntV2(const string &name,
    const GLuint count, const Vec2i &val)
: ValueUniform<Vec2i>(name, GL_INT,
    sizeof(GLint), 2, false, count, true)
{
  set_value(val);
}
void UniformIntV2::apply(GLint loc) const
{
  glUniform4iv(loc, count_, &value().x);
}

UniformIntV3::UniformIntV3(const string &name,
    const GLuint count)
: ValueUniform<Vec3i>(name, GL_INT,
    sizeof(GLint), 3, false, count, true)
{
}
UniformIntV3::UniformIntV3(const string &name,
    const GLuint count, const Vec3i &val)
: ValueUniform<Vec3i>(name, GL_INT,
    sizeof(GLint), 3, false, count, true)
{
  set_value(val);
}
void UniformIntV3::apply(GLint loc) const
{
  glUniform4iv(loc, count_, &value().x);
}

UniformIntV4::UniformIntV4(const string &name,
    const GLuint count)
: ValueUniform<Vec4i>(name, GL_INT,
    sizeof(GLint), 4, false, count, true)
{
}
UniformIntV4::UniformIntV4(const string &name,
    const GLuint count, const Vec4i &val)
: ValueUniform<Vec4i>(name, GL_INT,
    sizeof(GLint), 4, false, count, true)
{
  set_value(val);
}
void UniformIntV4::apply(GLint loc) const
{
  glUniform4iv(loc, count_, &value().x);
}

UniformFloat::UniformFloat(const string &name,
    const GLuint count)
: ValueUniform<GLfloat>(name, GL_FLOAT,
    sizeof(GLfloat), 1, false, count, false)
{
}
UniformFloat::UniformFloat(const string &name,
    const GLuint count, const GLfloat &val)
: ValueUniform<GLfloat>(name, GL_FLOAT,
    sizeof(GLfloat), 1, false, count, false)
{
  set_value(val);
}
void UniformFloat::apply(GLint loc) const
{
  glUniform1fv(loc, count_, &value());
}

UniformVec2::UniformVec2(const string &name,
    const GLuint count)
: ValueUniform<Vec2f>(name, GL_FLOAT,
    sizeof(GLfloat), 2, false, count, false)
{
}
UniformVec2::UniformVec2(const string &name,
    const GLuint count, const Vec2f &val)
: ValueUniform<Vec2f>(name, GL_FLOAT,
    sizeof(GLfloat), 2, false, count, false)
{
  set_value(val);
}
void UniformVec2::apply(GLint loc) const
{
  glUniform2fv(loc, count_, &value().x);
}

UniformVec3::UniformVec3(const string &name,
    const GLuint count)
: ValueUniform<Vec3f>(name, GL_FLOAT,
    sizeof(GLfloat), 3, false, count, false)
{
}
UniformVec3::UniformVec3(const string &name,
    const GLuint count, const Vec3f &val)
: ValueUniform<Vec3f>(name, GL_FLOAT,
    sizeof(GLfloat), 3, false, count, false)
{
  set_value(val);
}
void UniformVec3::apply(GLint loc) const
{
  glUniform3fv(loc, count_, &value().x);
}

UniformVec4::UniformVec4(const string &name,
    const GLuint count)
: ValueUniform<Vec4f>(name, GL_FLOAT,
    sizeof(GLfloat), 4, false, count, false)
{
}
UniformVec4::UniformVec4(const string &name,
    const GLuint count, const Vec4f &val)
: ValueUniform<Vec4f>(name, GL_FLOAT,
    sizeof(GLfloat), 4, false, count, false)
{
  set_value(val);
}
void UniformVec4::apply(GLint loc) const
{
  glUniform4fv(loc, count_, &value().x);
}

UniformMat3::UniformMat3(const string &name,
    const GLuint count)
: ValueUniform<Mat3f>(name, GL_FLOAT,
    sizeof(GLfloat), 9, false, count, false)
{
}
UniformMat3::UniformMat3(const string &name,
    const GLuint count, const Mat3f &val)
: ValueUniform<Mat3f>(name, GL_FLOAT,
    sizeof(GLfloat), 9, false, count, false)
{
  set_value(val);
}
void UniformMat3::apply(GLint loc) const
{
  glUniformMatrix3fv(loc, count_, false, value().x);
}

UniformMat4::UniformMat4(const string &name,
    const GLuint count)
: ValueUniform<Mat4f>(name, GL_FLOAT,
    sizeof(GLfloat), 16, false, count, false)
{
}
UniformMat4::UniformMat4(const string &name,
    const GLuint count, const Mat4f &val)
: ValueUniform<Mat4f>(name, GL_FLOAT,
    sizeof(GLfloat), 16, false, count, false)
{
  set_value(val);
}
void UniformMat4::apply(GLint loc) const
{
  glUniformMatrix4fv(loc, count_, false, value().x);
}


