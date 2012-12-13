/*
 * shader-input.cpp
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#include "shader-input.h"

#include <ogle/utility/string-util.h>

ShaderInput::ShaderInput(
    const string &name,
    GLenum dataType,
    GLuint dataTypeBytes,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: VertexAttribute(name,dataType,dataTypeBytes,valsPerElement,elementCount,normalize),
  isConstant_(GL_FALSE),
  forceArray_(GL_FALSE),
  fragmentInterpolation_(FRAGMENT_INTERPOLATION_DEFAULT)
{
}

GLboolean ShaderInput::isVertexAttribute() const
{
  return (numInstances_>1 || numVertices_>1 || buffer_>0);
}

void ShaderInput::set_isConstant(GLboolean isConstant)
{
  isConstant_ = isConstant;
}
GLboolean ShaderInput::isConstant() const
{
  return isConstant_;
}

void ShaderInput::set_interpolationMode(FragmentInterpolation fragmentInterpolation)
{
  fragmentInterpolation_ = fragmentInterpolation;
}
FragmentInterpolation ShaderInput::interpolationMode()
{
  if(numInstances_>1) {
    // no interpolation needed for instanced attributes because
    // they do not change for faces.
    return FRAGMENT_INTERPOLATION_FLAT;
  } else {
    return fragmentInterpolation_;
  }
}

void ShaderInput::set_forceArray(GLboolean forceArray)
{
  forceArray_ = forceArray;
}
GLboolean ShaderInput::forceArray()
{
  return forceArray_;
}

/**
 * Binds vertex attribute for active buffer to the
 * given shader location.
 */
void ShaderInput::enableAttribute(GLint loc) const
{
  enable(loc);
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
void ShaderInput1f::enableUniform(GLint loc) const
{
  glUniform1fv(loc, elementCount_, (const GLfloat*)data_);
}
void ShaderInput1f::setUniformData(const GLfloat &data)
{
  setInstanceData(1, 1, (byte*) &data);
}

ShaderInput2f::ShaderInput2f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, 2, elementCount, normalize)
{
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
void ShaderInput2f::enableUniform(GLint loc) const
{
  glUniform2fv(loc, elementCount_, (const GLfloat*)data_);
}
void ShaderInput2f::setUniformData(const Vec2f &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput3f::ShaderInput3f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, 3, elementCount, normalize)
{
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
void ShaderInput3f::enableUniform(GLint loc) const
{
  glUniform3fv(loc, elementCount_, (const GLfloat*)data_);
}
void ShaderInput3f::setUniformData(const Vec3f &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput4f::ShaderInput4f(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, 4, elementCount, normalize)
{
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
void ShaderInput4f::enableUniform(GLint loc) const
{
  glUniform4fv(loc, elementCount_, (const GLfloat*)data_);
}
void ShaderInput4f::setUniformData(const Vec4f &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
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
void ShaderInput1d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_;
  GLfloat castedData = data[0];
  glUniform1fv(loc, elementCount_, &castedData);
}
void ShaderInput1d::setUniformData(const GLdouble &data)
{
  setInstanceData(1, 1, (byte*) &data);
}

ShaderInput2d::ShaderInput2d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputd(name, 2, elementCount, normalize)
{
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
void ShaderInput2d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_;
  GLfloat castedData[2];
  castedData[0] = data[0];
  castedData[1] = data[1];
  glUniform2fv(loc, elementCount_, castedData);
}
void ShaderInput2d::setUniformData(const Vec2d &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput3d::ShaderInput3d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputd(name, 3, elementCount, normalize)
{
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
void ShaderInput3d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_;
  GLfloat castedData[3];
  castedData[0] = data[0];
  castedData[1] = data[1];
  castedData[2] = data[2];
  glUniform3fv(loc, elementCount_, castedData);
}
void ShaderInput3d::setUniformData(const Vec3d &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput4d::ShaderInput4d(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputd(name, 4, elementCount, normalize)
{
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
void ShaderInput4d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_;
  GLfloat castedData[4];
  castedData[0] = data[0];
  castedData[1] = data[1];
  castedData[2] = data[2];
  castedData[3] = data[3];
  glUniform4fv(loc, elementCount_, castedData);
}
void ShaderInput4d::setUniformData(const Vec4d &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInputi::ShaderInputi(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInput(name, GL_INT, sizeof(GLint), valsPerElement, elementCount, normalize)
{
}
void ShaderInputi::enableAttribute(GLint loc) const
{
  enablei(loc);
}

ShaderInput1i::ShaderInput1i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 1, elementCount, normalize)
{
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
void ShaderInput1i::enableUniform(GLint loc) const
{
  glUniform1iv(loc, elementCount_, (const GLint*)data_);
}
void ShaderInput1i::setUniformData(const GLint &data)
{
  setInstanceData(1, 1, (byte*) &data);
}

ShaderInput2i::ShaderInput2i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 2, elementCount, normalize)
{
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
void ShaderInput2i::enableUniform(GLint loc) const
{
  glUniform2iv(loc, elementCount_, (const GLint*)data_);
}
void ShaderInput2i::setUniformData(const Vec2i &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput3i::ShaderInput3i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 3, elementCount, normalize)
{
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
void ShaderInput3i::enableUniform(GLint loc) const
{
  glUniform3iv(loc, elementCount_, (const GLint*)data_);
}
void ShaderInput3i::setUniformData(const Vec3i &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput4i::ShaderInput4i(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputi(name, 4, elementCount, normalize)
{
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
void ShaderInput4i::enableUniform(GLint loc) const
{
  glUniform4iv(loc, elementCount_, (const GLint*)data_);
}
void ShaderInput4i::setUniformData(const Vec4i &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInputui::ShaderInputui(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInput(name, GL_UNSIGNED_INT, sizeof(GLuint), valsPerElement, elementCount, normalize)
{
}
void ShaderInputui::enableAttribute(GLint loc) const
{
  enablei(loc);
}

ShaderInput1ui::ShaderInput1ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 1, elementCount, normalize)
{
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
void ShaderInput1ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_;
  GLint intData = data[0];
  glUniform1iv(loc, elementCount_, &intData);
}
void ShaderInput1ui::setUniformData(const GLuint &data)
{
  setInstanceData(1, 1, (byte*) &data);
}

ShaderInput2ui::ShaderInput2ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 2, elementCount, normalize)
{
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
void ShaderInput2ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_;
  GLint intData[2];
  intData[0] = data[0];
  intData[1] = data[1];
  glUniform2iv(loc, elementCount_, intData);
}
void ShaderInput2ui::setUniformData(const Vec2ui &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput3ui::ShaderInput3ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 3, elementCount, normalize)
{
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
void ShaderInput3ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_;
  GLint intData[3];
  intData[0] = data[0];
  intData[1] = data[1];
  intData[2] = data[2];
  glUniform3iv(loc, elementCount_, intData);
}
void ShaderInput3ui::setUniformData(const Vec3ui &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInput4ui::ShaderInput4ui(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputui(name, 4, elementCount, normalize)
{
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
void ShaderInput4ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_;
  GLint intData[4];
  intData[0] = data[0];
  intData[1] = data[1];
  intData[2] = data[2];
  intData[3] = data[3];
  glUniform4iv(loc, elementCount_, intData);
}
void ShaderInput4ui::setUniformData(const Vec4ui &data)
{
  setInstanceData(1, 1, (byte*) &data.x);
}

ShaderInputMat::ShaderInputMat(
    const string &name,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(name, valsPerElement, elementCount, normalize),
  transpose_(GL_FALSE)
{
}
void ShaderInputMat::set_transpose(GLboolean transpose) {
  transpose_ = transpose;
}
GLboolean ShaderInputMat::transpose() const {
  return transpose_;
}

ShaderInputMat3::ShaderInputMat3(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputMat(name, 9, elementCount, normalize)
{
}
istream& ShaderInputMat3::operator<<(istream &in)
{
  Mat3f value = identity3f();
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInputMat3::operator>>(ostream &out) const
{
  return out << *((Mat3f*)data_);
}
void ShaderInputMat3::enableAttribute(GLint loc) const
{
  enableMat3(loc);
}
void ShaderInputMat3::enableUniform(GLint loc) const
{
  glUniformMatrix3fv(loc, elementCount_, transpose_, (const GLfloat*)data_);
}
void ShaderInputMat3::setUniformData(const Mat3f &data)
{
  setInstanceData(1, 1, (byte*) data.x);
}

ShaderInputMat4::ShaderInputMat4(
    const string &name,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputMat(name, 16, elementCount, normalize)
{
}
istream& ShaderInputMat4::operator<<(istream &in)
{
  Mat4f value = identity4f();
  in >> value;
  setUniformData(value);
  return in;
}
ostream& ShaderInputMat4::operator>>(ostream &out) const
{
  return out << *((Mat4f*)data_);
}
void ShaderInputMat4::enableAttribute(GLint loc) const
{
  enableMat4(loc);
}
void ShaderInputMat4::enableUniform(GLint loc) const
{
  glUniformMatrix4fv(loc, elementCount_, transpose_, (const GLfloat*)data_);
}
void ShaderInputMat4::setUniformData(const Mat4f &data)
{
  setInstanceData(1, 1, (byte*) data.x);
}

///////////

PositionShaderInput::PositionShaderInput(GLboolean normalize)
: ShaderInput3f(ATTRIBUTE_NAME_POS, 1, normalize)
{
}

NormalShaderInput::NormalShaderInput(GLboolean normalize)
: ShaderInput3f(ATTRIBUTE_NAME_NOR, 1, normalize)
{
}

TangentShaderInput::TangentShaderInput(GLboolean normalize)
: ShaderInput4f(ATTRIBUTE_NAME_TAN, 1, normalize)
{
}

TexcoShaderInput::TexcoShaderInput(
    GLuint channel,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: ShaderInputf(FORMAT_STRING("texco"<<channel), valsPerElement, elementCount, normalize)
{
}
istream& TexcoShaderInput::operator<<(istream &in)
{
  if(valsPerElement_==1) {
    GLfloat val;
    in >> val;
    setInstanceData(1, 1, (byte*)&val);
  } else if(valsPerElement_==2) {
    Vec2f val;
    in >> val;
    setInstanceData(1, 1, (byte*)&val.x);
  } else if(valsPerElement_==3) {
    Vec3f val;
    in >> val;
    setInstanceData(1, 1, (byte*)&val.x);
  } else if(valsPerElement_==4) {
    Vec4f val;
    in >> val;
    setInstanceData(1, 1, (byte*)&val.x);
  }
  return in;
}
ostream& TexcoShaderInput::operator>>(ostream &out) const
{
  if(valsPerElement_==1) {
    return out << *((GLfloat*)data_);
  } else if(valsPerElement_==2) {
    return out << *((Vec2f*)data_);
  } else if(valsPerElement_==3) {
    return out << *((Vec3f*)data_);
  } else if(valsPerElement_==4) {
    return out << *((Vec4f*)data_);
  } else {
    return out;
  }
}
GLuint TexcoShaderInput::channel() const {
  return channel_;
}
void TexcoShaderInput::enableUniform(GLint loc) const
{
  if(valsPerElement_==1) {
    glUniform1fv(loc, elementCount_, (const GLfloat*)data_);
  } else if(valsPerElement_==2) {
    glUniform2fv(loc, elementCount_, (const GLfloat*)data_);
  } else if(valsPerElement_==3) {
    glUniform3fv(loc, elementCount_, (const GLfloat*)data_);
  } else if(valsPerElement_==4) {
    glUniform4fv(loc, elementCount_, (const GLfloat*)data_);
  }
}
