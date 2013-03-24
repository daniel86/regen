/*
 * shader-input.cpp
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>

#include "shader-input.h"
using namespace ogle;

/////////////
/////////////
/////////////

void ShaderInput::enableUniform1f(GLint loc) const
{
  glUniform1fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniform2f(GLint loc) const
{
  glUniform2fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniform3f(GLint loc) const
{
  glUniform3fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniform4f(GLint loc) const
{
  glUniform4fv(loc, elementCount_, (const GLfloat*)data());
}
void ShaderInput::enableUniform1i(GLint loc) const
{
  glUniform1iv(loc, elementCount_, (const GLint*)data());
}
void ShaderInput::enableUniform2i(GLint loc) const
{
  glUniform2iv(loc, elementCount_, (const GLint*)data());
}
void ShaderInput::enableUniform3i(GLint loc) const
{
  glUniform3iv(loc, elementCount_, (const GLint*)data());
}
void ShaderInput::enableUniform4i(GLint loc) const
{
  glUniform4iv(loc, elementCount_, (const GLint*)data());
}
void ShaderInput::enableUniformMat3(GLint loc) const
{
  glUniformMatrix3fv(loc, elementCount_, transpose(), (const GLfloat*)data());
}
void ShaderInput::enableUniformMat4(GLint loc) const
{
  glUniformMatrix4fv(loc, elementCount_, transpose(), (const GLfloat*)data());
}

void ShaderInput::enableUniform1d(GLint loc) const
{
  const GLdouble *v = (const GLdouble*)data();
  GLfloat castedData = v[0];
  glUniform1fv(loc, elementCount_, &castedData);
}
void ShaderInput::enableUniform2d(GLint loc) const
{
  const GLdouble *v = (const GLdouble*)data();
  GLfloat castedData[2];
  castedData[0] = v[0];
  castedData[1] = v[1];
  glUniform2fv(loc, elementCount_, castedData);
}
void ShaderInput::enableUniform3d(GLint loc) const
{
  const GLdouble *v = (const GLdouble*)data();
  GLfloat castedData[3];
  castedData[0] = v[0];
  castedData[1] = v[1];
  castedData[2] = v[2];
  glUniform3fv(loc, elementCount_, castedData);
}
void ShaderInput::enableUniform4d(GLint loc) const
{
  const GLdouble *v = (const GLdouble*)data();
  GLfloat castedData[4];
  castedData[0] = v[0];
  castedData[1] = v[1];
  castedData[2] = v[2];
  castedData[3] = v[3];
  glUniform4fv(loc, elementCount_, castedData);
}
void ShaderInput::enableUniform1ui(GLint loc) const
{
  const GLuint *v = (const GLuint*)data();
  GLint intData = v[0];
  glUniform1iv(loc, elementCount_, &intData);
}
void ShaderInput::enableUniform2ui(GLint loc) const
{
  const GLuint *v = (const GLuint*)data();
  GLint intData[2];
  intData[0] = v[0];
  intData[1] = v[1];
  glUniform2iv(loc, elementCount_, intData);
}
void ShaderInput::enableUniform3ui(GLint loc) const
{
  const GLuint *v = (const GLuint*)data();
  GLint intData[3];
  intData[0] = v[0];
  intData[1] = v[1];
  intData[2] = v[2];
  glUniform3iv(loc, elementCount_, intData);
}
void ShaderInput::enableUniform4ui(GLint loc) const
{
  const GLuint *v = (const GLuint*)data();
  GLint intData[4];
  intData[0] = v[0];
  intData[1] = v[1];
  intData[2] = v[2];
  intData[3] = v[3];
  glUniform4iv(loc, elementCount_, intData);
}

/////////////
/////////////
////////////

ref_ptr<ShaderInput> ShaderInput::create(
    const string &name, GLenum dataType, GLuint valsPerElement)
{
  switch(dataType) {
  case GL_FLOAT:
    switch(valsPerElement) {
    case 16:
      return ref_ptr<ShaderInput>::manage(new ShaderInputMat4(name));
    case 9:
      return ref_ptr<ShaderInput>::manage(new ShaderInputMat3(name));
    case 4:
      return ref_ptr<ShaderInput>::manage(new ShaderInput4f(name));
    case 3:
      return ref_ptr<ShaderInput>::manage(new ShaderInput3f(name));
    case 2:
      return ref_ptr<ShaderInput>::manage(new ShaderInput2f(name));
    default:
      return ref_ptr<ShaderInput>::manage(new ShaderInput1f(name));
    }
    break;
  case GL_DOUBLE:
    switch(valsPerElement) {
    case 4:
      return ref_ptr<ShaderInput>::manage(new ShaderInput4d(name));
    case 3:
      return ref_ptr<ShaderInput>::manage(new ShaderInput3d(name));
    case 2:
      return ref_ptr<ShaderInput>::manage(new ShaderInput2d(name));
    default:
      return ref_ptr<ShaderInput>::manage(new ShaderInput1d(name));
    }
    break;
  case GL_BOOL:
  case GL_INT:
    switch(valsPerElement) {
    case 4:
      return ref_ptr<ShaderInput>::manage(new ShaderInput4i(name));
    case 3:
      return ref_ptr<ShaderInput>::manage(new ShaderInput3i(name));
    case 2:
      return ref_ptr<ShaderInput>::manage(new ShaderInput2i(name));
    default:
      return ref_ptr<ShaderInput>::manage(new ShaderInput1i(name));
    }
    break;
  case GL_UNSIGNED_INT:
    switch(valsPerElement) {
    case 4:
      return ref_ptr<ShaderInput>::manage(new ShaderInput4ui(name));
    case 3:
      return ref_ptr<ShaderInput>::manage(new ShaderInput3ui(name));
    case 2:
      return ref_ptr<ShaderInput>::manage(new ShaderInput2ui(name));
    default:
      return ref_ptr<ShaderInput>::manage(new ShaderInput1ui(name));
    }
    break;
  default:
    return ref_ptr<ShaderInput>();
  }
}

ShaderInput::ShaderInput(
    const string &name,
    GLenum dataType,
    GLuint dataTypeBytes,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: VertexAttribute(name,dataType,dataTypeBytes,valsPerElement,elementCount,normalize),
  isConstant_(GL_FALSE),
  forceArray_(GL_FALSE)
{
  enableAttribute_ = &VertexAttribute::enable;
}

GLboolean ShaderInput::isVertexAttribute() const
{
  return isVertexAttribute_;
}

void ShaderInput::set_isConstant(GLboolean isConstant)
{
  isConstant_ = isConstant;
}
GLboolean ShaderInput::isConstant() const
{
  return isConstant_;
}

void ShaderInput::set_forceArray(GLboolean forceArray)
{
  forceArray_ = forceArray;
}
GLboolean ShaderInput::forceArray() const
{
  return forceArray_;
}

void ShaderInput::setUniformDataUntyped(byte *data)
{
  setInstanceData(1,1,data);
  isVertexAttribute_ = GL_FALSE;
}

void ShaderInput::enableAttribute(GLint loc) const
{
  (this->*(this->enableAttribute_))(loc);
}

void ShaderInput::enableUniform(GLint loc) const
{
  (this->*(this->enableUniform_))(loc);
}

/////////////

ShaderInputf::ShaderInputf(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInput(name, GL_FLOAT, sizeof(GLfloat), valsPerElement, elementCount, normalize)
{
}

ShaderInput1f::ShaderInput1f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, 1, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform1f;
}
istream& ShaderInput1f::operator<<(istream &in)
{
  GLfloat value=0.0f;
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput1f::operator>>(ostream &out) const
{
  return out << *((GLfloat*)data_);
}
void ShaderInput1f::setUniformData(const GLfloat &data)
{
  setUniformDataUntyped((byte*) &data);
}

ShaderInput2f::ShaderInput2f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, 2, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform2f;
}
istream& ShaderInput2f::operator<<(istream &in)
{
  Vec2f value(0.0f);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput2f::operator>>(ostream &out) const
{
  return out << *((Vec2f*)data_);
}
void ShaderInput2f::setUniformData(const Vec2f &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInput3f::ShaderInput3f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, 3, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform3f;
}
istream& ShaderInput3f::operator<<(istream &in)
{
  Vec3f value(0.0f);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput3f::operator>>(ostream &out) const
{
  return out << *((Vec3f*)data_);
}
void ShaderInput3f::setUniformData(const Vec3f &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInput4f::ShaderInput4f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, 4, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform4f;
}
istream& ShaderInput4f::operator<<(istream &in)
{
  Vec4f value(0.0f);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput4f::operator>>(ostream &out) const
{
  return out << *((Vec4f*)data_);
}
void ShaderInput4f::setUniformData(const Vec4f &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInputd::ShaderInputd(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInput(name, GL_DOUBLE, sizeof(GLdouble), valsPerElement, elementCount, normalize)
{
}

ShaderInput1d::ShaderInput1d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputd(name, 1, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform1d;
}
istream& ShaderInput1d::operator<<(istream &in)
{
  GLdouble value=0.0;
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput1d::operator>>(ostream &out) const
{
  return out << *((GLdouble*)data_);
}
void ShaderInput1d::setUniformData(const GLdouble &data)
{
  setUniformDataUntyped((byte*) &data);
}

ShaderInput2d::ShaderInput2d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputd(name, 2, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform2d;
}
istream& ShaderInput2d::operator<<(istream &in)
{
  Vec2d value(0.0);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput2d::operator>>(ostream &out) const
{
  return out << *((Vec2d*)data_);
}
void ShaderInput2d::setUniformData(const Vec2d &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInput3d::ShaderInput3d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputd(name, 3, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform3d;
}
istream& ShaderInput3d::operator<<(istream &in)
{
  Vec3d value(0.0);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput3d::operator>>(ostream &out) const
{
  return out << *((Vec3d*)data_);
}
void ShaderInput3d::setUniformData(const Vec3d &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInput4d::ShaderInput4d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputd(name, 4, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform4d;
}
istream& ShaderInput4d::operator<<(istream &in)
{
  Vec4d value(0.0);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput4d::operator>>(ostream &out) const
{
  return out << *((Vec4d*)data_);
}
void ShaderInput4d::setUniformData(const Vec4d &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInputi::ShaderInputi(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInput(name, GL_INT, sizeof(GLint), valsPerElement, elementCount, normalize)
{
  enableAttribute_ = &VertexAttribute::enablei;
}

ShaderInput1i::ShaderInput1i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 1, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform1i;
}
istream& ShaderInput1i::operator<<(istream &in)
{
  GLint value=0;
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput1i::operator>>(ostream &out) const
{
  return out << *((GLint*)data_);
}
void ShaderInput1i::setUniformData(const GLint &data)
{
  setUniformDataUntyped((byte*) &data);
}

ShaderInput2i::ShaderInput2i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 2, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform2i;
}
istream& ShaderInput2i::operator<<(istream &in)
{
  Vec2i value(0);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput2i::operator>>(ostream &out) const
{
  return out << *((Vec4d*)data_);
}
void ShaderInput2i::setUniformData(const Vec2i &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInput3i::ShaderInput3i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 3, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform3i;
}
istream& ShaderInput3i::operator<<(istream &in)
{
  Vec3i value(0);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput3i::operator>>(ostream &out) const
{
  return out << *((Vec3i*)data_);
}
void ShaderInput3i::setUniformData(const Vec3i &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInput4i::ShaderInput4i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 4, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform4i;
}
istream& ShaderInput4i::operator<<(istream &in)
{
  Vec4i value(0);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput4i::operator>>(ostream &out) const
{
  return out << *((Vec4i*)data_);
}
void ShaderInput4i::setUniformData(const Vec4i &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInputui::ShaderInputui(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInput(name, GL_UNSIGNED_INT, sizeof(GLuint), valsPerElement, elementCount, normalize)
{
  enableAttribute_ = &VertexAttribute::enablei;
}

ShaderInput1ui::ShaderInput1ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 1, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform1ui;
}
istream& ShaderInput1ui::operator<<(istream &in)
{
  GLuint value=0u;
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput1ui::operator>>(ostream &out) const
{
  return out << *((GLuint*)data_);
}
void ShaderInput1ui::setUniformData(const GLuint &data)
{
  setUniformDataUntyped((byte*) &data);
}

ShaderInput2ui::ShaderInput2ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 2, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform2ui;
}
istream& ShaderInput2ui::operator<<(istream &in)
{
  Vec2ui value(0u);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput2ui::operator>>(ostream &out) const
{
  return out << *((Vec2ui*)data_);
}
void ShaderInput2ui::setUniformData(const Vec2ui &data)
{
  setUniformDataUntyped( (byte*) &data.x);
}

ShaderInput3ui::ShaderInput3ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 3, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform3ui;
}
istream& ShaderInput3ui::operator<<(istream &in)
{
  Vec3ui value(0u);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput3ui::operator>>(ostream &out) const
{
  return out << *((Vec3ui*)data_);
}
void ShaderInput3ui::setUniformData(const Vec3ui &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInput4ui::ShaderInput4ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 4, elementCount, normalize)
{
  enableUniform_ = &ShaderInput::enableUniform4ui;
}
istream& ShaderInput4ui::operator<<(istream &in)
{
  Vec4ui value(0u);
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInput4ui::operator>>(ostream &out) const
{
  return out << *((Vec4ui*)data_);
}
void ShaderInput4ui::setUniformData(const Vec4ui &data)
{
  setUniformDataUntyped((byte*) &data.x);
}

ShaderInputMat::ShaderInputMat(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, valsPerElement, elementCount, normalize)
{
  transpose_ = GL_FALSE;
}

ShaderInputMat3::ShaderInputMat3(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputMat(name, 9, elementCount, normalize)
{
  enableAttribute_ = &VertexAttribute::enableMat3;
  enableUniform_ = &ShaderInput::enableUniformMat3;
}
istream& ShaderInputMat3::operator<<(istream &in)
{
  Mat3f value = Mat3f::identity();
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInputMat3::operator>>(ostream &out) const
{
  return out << *((Mat3f*)data_);
}
void ShaderInputMat3::setUniformData(const Mat3f &data)
{
  setUniformDataUntyped((byte*) data.x);
}

ShaderInputMat4::ShaderInputMat4(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputMat(name, 16, elementCount, normalize)
{
  enableAttribute_ = &VertexAttribute::enableMat4;
  enableUniform_ = &ShaderInput::enableUniformMat4;
}
istream& ShaderInputMat4::operator<<(istream &in)
{
  Mat4f value = Mat4f::identity();
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInputMat4::operator>>(ostream &out) const
{
  return out << *((Mat4f*)data_);
}
void ShaderInputMat4::setUniformData(const Mat4f &data)
{
  setUniformDataUntyped((byte*) data.x);
}
