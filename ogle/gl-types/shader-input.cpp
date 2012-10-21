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
  // TODO: allow const and instanced input for << operator
}

GLboolean ShaderInput::isVertexAttribute() const
{
  return (numInstances_>1 || numVertices_>1);
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
void ShaderInput1f::operator<<(const string &valueString)
{
  GLfloat value=0.0f;
  if(parseVec1f(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a float.");
  }
}
void ShaderInput1f::enableUniform(GLint loc) const
{
  glUniform1fv(loc, elementCount_, (const GLfloat*)data_->data());
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
void ShaderInput2f::operator<<(const string &valueString)
{
  Vec2f value(0.0f);
  if(parseVec2f(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec2f.");
  }
}
void ShaderInput2f::enableUniform(GLint loc) const
{
  glUniform2fv(loc, elementCount_, (const GLfloat*)data_->data());
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
void ShaderInput3f::operator<<(const string &valueString)
{
  Vec3f value(0.0f);
  if(parseVec3f(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec3f.");
  }
}
void ShaderInput3f::enableUniform(GLint loc) const
{
  glUniform3fv(loc, elementCount_, (const GLfloat*)data_->data());
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
void ShaderInput4f::operator<<(const string &valueString)
{
  Vec4f value(0.0f);
  if(parseVec4f(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec4f.");
  }
}
void ShaderInput4f::enableUniform(GLint loc) const
{
  glUniform4fv(loc, elementCount_, (const GLfloat*)data_->data());
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
void ShaderInput1d::operator<<(const string &valueString)
{
  GLdouble value=0.0;
  if(parseVec1d(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a double.");
  }
}
void ShaderInput1d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_->data();
  GLfloat castedData[1] = { data[0] };
  glUniform1fv(loc, elementCount_, castedData);
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
void ShaderInput2d::operator<<(const string &valueString)
{
  Vec2d value(0.0);
  if(parseVec2d(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec2d.");
  }
}
void ShaderInput2d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_->data();
  GLfloat castedData[2] = { data[0], data[1] };
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
void ShaderInput3d::operator<<(const string &valueString)
{
  Vec3d value(0.0);
  if(parseVec3d(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec3d.");
  }
}
void ShaderInput3d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_->data();
  GLfloat castedData[3] = { data[0], data[1], data[2] };
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
void ShaderInput4d::operator<<(const string &valueString)
{
  Vec4d value(0.0);
  if(parseVec4d(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec4d.");
  }
}
void ShaderInput4d::enableUniform(GLint loc) const
{
  const GLdouble *data = (const GLdouble*)data_->data();
  GLfloat castedData[4] = { data[0], data[1], data[2], data[3] };
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
void ShaderInput1i::operator<<(const string &valueString)
{
  GLint value=0;
  if(parseVec1i(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a int.");
  }
}
void ShaderInput1i::enableUniform(GLint loc) const
{
  glUniform1iv(loc, elementCount_, (const GLint*)data_->data());
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
void ShaderInput2i::operator<<(const string &valueString)
{
  Vec2i value(0);
  if(parseVec2i(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec2i.");
  }
}
void ShaderInput2i::enableUniform(GLint loc) const
{
  glUniform2iv(loc, elementCount_, (const GLint*)data_->data());
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
void ShaderInput3i::operator<<(const string &valueString)
{
  Vec3i value(0);
  if(parseVec3i(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec3i.");
  }
}
void ShaderInput3i::enableUniform(GLint loc) const
{
  glUniform3iv(loc, elementCount_, (const GLint*)data_->data());
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
void ShaderInput4i::operator<<(const string &valueString)
{
  Vec4i value(0);
  if(parseVec4i(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a vec4i.");
  }
}
void ShaderInput4i::enableUniform(GLint loc) const
{
  glUniform4iv(loc, elementCount_, (const GLint*)data_->data());
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
void ShaderInput1ui::operator<<(const string &valueString)
{
  GLuint value=0u;
  if(parseVec1ui(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a uint.");
  }
}
void ShaderInput1ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_->data();
  GLint intData[1] = { data[0] };
  glUniform1iv(loc, elementCount_, intData);
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
void ShaderInput2ui::operator<<(const string &valueString)
{
  Vec2ui value(0u);
  if(parseVec2ui(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a uvec2.");
  }
}
void ShaderInput2ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_->data();
  GLint intData[2] = { data[0], data[1] };
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
void ShaderInput3ui::operator<<(const string &valueString)
{
  Vec3ui value(0u);
  if(parseVec3ui(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a uvec3.");
  }
}
void ShaderInput3ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_->data();
  GLint intData[3] = { data[0], data[1], data[2] };
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
void ShaderInput4ui::operator<<(const string &valueString)
{
  Vec4ui value(0u);
  if(parseVec4ui(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'Not a uvec4.");
  }
}
void ShaderInput4ui::enableUniform(GLint loc) const
{
  const GLuint *data = (const GLuint*)data_->data();
  GLint intData[4] = { data[0], data[1], data[2], data[3] };
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
void ShaderInputMat3::operator<<(const string &valueString)
{
  Mat3f value = identity3f();
  if(parseMat3f(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'. Not a mat3.");
  }
}
void ShaderInputMat3::enableAttribute(GLint loc) const
{
  enableMat3(loc);
}
void ShaderInputMat3::enableUniform(GLint loc) const
{
  glUniformMatrix3fv(loc, elementCount_, transpose_, (const GLfloat*)data_->data());
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
void ShaderInputMat4::operator<<(const string &valueString)
{
  Mat4f value = identity4f();
  if(parseMat4f(valueString,value)==0) {
    setUniformData(value);
  } else {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'. Not a mat4.");
  }
}
void ShaderInputMat4::enableAttribute(GLint loc) const
{
  enableMat4(loc);
}
void ShaderInputMat4::enableUniform(GLint loc) const
{
  glUniformMatrix4fv(loc, elementCount_, transpose_, (const GLfloat*)data_->data());
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
void TexcoShaderInput::operator<<(const string &valueString)
{
  int result=0;
  if(valsPerElement_==1) {
    GLfloat val;
    if(parseVec1f(valueString, val)==0) {
      setInstanceData(1, 1, (byte*)&val);
    } else {
      result=1;
    }
  } else if(valsPerElement_==2) {
    Vec2f val;
    if(parseVec2f(valueString, val)==0) {
      setInstanceData(1, 1, (byte*)&val.x);
    } else {
      result=1;
    }
  } else if(valsPerElement_==3) {
    Vec3f val;
    if(parseVec3f(valueString, val)==0) {
      setInstanceData(1, 1, (byte*)&val.x);
    } else {
      result=1;
    }
  } else if(valsPerElement_==4) {
    Vec4f val;
    if(parseVec4f(valueString, val)==0) {
      setInstanceData(1, 1, (byte*)&val.x);
    } else {
      result=1;
    }
  }
  if(result!=0) {
    WARN_LOG("Failed to parse '" << valueString <<
        "'. For '"<< name_ <<"'. Not a texco attribute value.");
  }
}
GLuint TexcoShaderInput::channel() const {
  return channel_;
}
void TexcoShaderInput::enableUniform(GLint loc) const
{
  if(valsPerElement_==1) {
    glUniform1fv(loc, elementCount_, (const GLfloat*)data_->data());
  } else if(valsPerElement_==2) {
    glUniform2fv(loc, elementCount_, (const GLfloat*)data_->data());
  } else if(valsPerElement_==3) {
    glUniform3fv(loc, elementCount_, (const GLfloat*)data_->data());
  } else if(valsPerElement_==4) {
    glUniform4fv(loc, elementCount_, (const GLfloat*)data_->data());
  }
}
