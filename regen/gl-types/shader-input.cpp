/*
 * shader-input.cpp
 *
 *  Created on: 15.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>

#include "shader-input.h"
using namespace regen;

ShaderInput::ShaderInput(
    const string &name,
    GLenum dataType,
    GLuint dataTypeBytes,
    GLuint valsPerElement,
    GLuint elementCount,
    GLboolean normalize)
: name_(name),
  dataType_(dataType),
  dataTypeBytes_(dataTypeBytes),
  stride_(0),
  offset_(0),
  inputSize_(0),
  elementSize_(0),
  elementCount_(elementCount),
  numVertices_(0u),
  numInstances_(0u),
  valsPerElement_(valsPerElement),
  divisor_(0),
  buffer_(0),
  bufferStamp_(0),
  normalize_(normalize),
  isVertexAttribute_(GL_TRUE),
  transpose_(GL_FALSE),
  data_(NULL),
  stamp_(1u),
  isConstant_(GL_FALSE),
  forceArray_(GL_FALSE),
  active_(GL_TRUE)
{
  elementSize_ = dataTypeBytes_*valsPerElement_*elementCount_;
  // make data_ stack root
  dataStack_.push(data_);
  enableAttribute_ = &ShaderInput::enableAttributef;
}
ShaderInput::~ShaderInput()
{
  deallocateData();
  if(bufferIterator_.get()) {
    VertexBufferObject::free(bufferIterator_.get());
  }
}

GLenum ShaderInput::dataType() const
{ return dataType_; }
GLuint ShaderInput::dataTypeBytes() const
{ return dataTypeBytes_; }

const string& ShaderInput::name() const
{ return name_; }
void ShaderInput::set_name(const string &s)
{ name_ = s; }

GLboolean ShaderInput::active() const
{ return active_; }
void ShaderInput::set_active(GLboolean v)
{ active_ = v; }

GLuint ShaderInput::numInstances() const
{ return numInstances_; }

GLuint ShaderInput::numVertices() const
{ return numVertices_; }
void ShaderInput::set_numVertices(GLuint numVertices)
{ numVertices_ = numVertices; }

GLboolean ShaderInput::isVertexAttribute() const
{ return isVertexAttribute_; }

void ShaderInput::set_isConstant(GLboolean isConstant)
{ isConstant_ = isConstant; }
GLboolean ShaderInput::isConstant() const
{ return isConstant_; }

void ShaderInput::set_forceArray(GLboolean forceArray)
{ forceArray_ = forceArray; }
GLboolean ShaderInput::forceArray() const
{ return forceArray_; }

GLuint ShaderInput::stamp() const
{ return stamp_; }
void ShaderInput::nextStamp()
{ stamp_ += 1; }

void ShaderInput::set_stride(GLuint stride)
{ stride_ = stride; }
GLuint ShaderInput::stride() const
{ return stride_; }

void ShaderInput::set_offset(GLuint offset)
{ offset_ = offset; }
GLuint ShaderInput::offset() const
{ return offset_; }

GLuint ShaderInput::divisor() const
{ return divisor_; }

GLuint ShaderInput::inputSize() const
{ return inputSize_; }
void ShaderInput::set_inputSize(GLuint size)
{ inputSize_ = size; }

GLuint ShaderInput::elementSize() const
{ return elementSize_; }

void ShaderInput::set_elementCount(GLuint v)
{
  elementCount_ = v;
  elementSize_ = dataTypeBytes_*valsPerElement_*elementCount_;
}
GLuint ShaderInput::elementCount() const
{ return elementCount_; }

GLuint ShaderInput::valsPerElement() const
{ return valsPerElement_; }

GLboolean ShaderInput::normalize() const
{ return normalize_; }

void ShaderInput::set_transpose(GLboolean transpose)
{ transpose_ = transpose; }
GLboolean ShaderInput::transpose() const
{ return transpose_; }

void ShaderInput::set_buffer(GLuint buffer, const VBOReference &it)
{
  buffer_ = buffer;
  bufferIterator_ = it;
  bufferStamp_ = stamp_;
}
GLuint ShaderInput::buffer() const
{ return buffer_; }
GLuint ShaderInput::bufferStamp() const
{ return bufferStamp_; }
ref_ptr<VertexBufferObject::Reference> ShaderInput::bufferIterator()
{ return bufferIterator_; }

/////////////
/////////////
/////////////

void ShaderInput::enableAttributef(GLint location) const
{
  for(register GLuint i=0; i<elementCount_; ++i) {
    GLint loc = location+i;
    glEnableVertexAttribArray( loc );
    glVertexAttribPointer(
        loc,
        valsPerElement_,
        dataType_,
        normalize_,
        stride_,
        BUFFER_OFFSET(offset_));
    if(divisor_!=0) {
      glVertexAttribDivisor(loc, divisor_);
    }
  }
}
void ShaderInput::enableAttributei(GLint location) const
{
  for(register GLuint i=0; i<elementCount_; ++i) {
    GLint loc = location+i;
    glEnableVertexAttribArray( loc );
    // use glVertexAttribIPointer, otherwise OpenGL
    // would convert integers to float
    glVertexAttribIPointer(
        loc,
        valsPerElement_,
        dataType_,
        stride_,
        BUFFER_OFFSET(offset_));
    if(divisor_!=0) {
      glVertexAttribDivisor(loc, divisor_);
    }
  }
}
void ShaderInput::enableAttributeMat4(GLint location) const
{
  for(register GLuint i=0; i<elementCount_*4; i+=4) {
    GLint loc0 = location+i;
    GLint loc1 = location+i+1;
    GLint loc2 = location+i+2;
    GLint loc3 = location+i+3;

    glEnableVertexAttribArray( loc0 );
    glEnableVertexAttribArray( loc1 );
    glEnableVertexAttribArray( loc2 );
    glEnableVertexAttribArray( loc3 );

    glVertexAttribPointer(loc0,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_));
    glVertexAttribPointer(loc1,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*4));
    glVertexAttribPointer(loc2,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*8));
    glVertexAttribPointer(loc3,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*12));

    if(divisor_!=0) {
      glVertexAttribDivisor(loc0, divisor_);
      glVertexAttribDivisor(loc1, divisor_);
      glVertexAttribDivisor(loc2, divisor_);
      glVertexAttribDivisor(loc3, divisor_);
    }
  }
}
void ShaderInput::enableAttributeMat3(GLint location) const
{
  for(register GLuint i=0; i<elementCount_*3; i+=4) {
    GLint loc0 = location+i;
    GLint loc1 = location+i+1;
    GLint loc2 = location+i+2;

    glEnableVertexAttribArray( loc0 );
    glEnableVertexAttribArray( loc1 );
    glEnableVertexAttribArray( loc2 );

    glVertexAttribPointer(loc0,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_));
    glVertexAttribPointer(loc1,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*4));
    glVertexAttribPointer(loc2,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*8));

    if(divisor_!=0) {
      glVertexAttribDivisor(loc0, divisor_);
      glVertexAttribDivisor(loc1, divisor_);
      glVertexAttribDivisor(loc2, divisor_);
    }
  }
}
void ShaderInput::enableAttributeMat2(GLint location) const
{
  for(register GLuint i=0; i<elementCount_*2; i+=4) {
    GLint loc0 = location+i;
    GLint loc1 = location+i+1;

    glEnableVertexAttribArray( loc0 );
    glEnableVertexAttribArray( loc1 );

    glVertexAttribPointer(loc0,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_));
    glVertexAttribPointer(loc1,
        4, dataType_, normalize_, stride_,
        BUFFER_OFFSET(offset_ + sizeof(float)*4));

    if(divisor_!=0) {
      glVertexAttribDivisor(loc0, divisor_);
      glVertexAttribDivisor(loc1, divisor_);
    }
  }
}
void ShaderInput::enableAttribute(GLint loc) const
{
  (this->*(this->enableAttribute_))(loc);
}

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
void ShaderInput::enableUniform(GLint loc) const
{
  (this->*(this->enableUniform_))(loc);
}

/////////////
/////////////
////////////

#define __ATTRIBUTE_VALUE__(vertexIndex, Type) \
    (((Type*) dataStack_.top()) + (vertexIndex*valsPerElement()) )

void ShaderInput::setVertex1f(GLuint vertexIndex, const GLfloat &val)
{
  *__ATTRIBUTE_VALUE__(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex2f(GLuint vertexIndex, const Vec2f &val)
{
  *(Vec2f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex3f(GLuint vertexIndex, const Vec3f &val)
{
  *(Vec3f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex4f(GLuint vertexIndex, const Vec4f &val)
{
  *(Vec4f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex9f(GLuint vertexIndex, const Mat3f &val)
{
  *(Mat3f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex16f(GLuint vertexIndex, const Mat4f &val)
{
  *(Mat4f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}

void ShaderInput::setVertex1d(GLuint vertexIndex, const GLdouble &val)
{
  *__ATTRIBUTE_VALUE__(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex2d(GLuint vertexIndex, const Vec2d &val)
{
  *(Vec2d*)__ATTRIBUTE_VALUE__(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex3d(GLuint vertexIndex, const Vec3d &val)
{
  *(Vec3d*)__ATTRIBUTE_VALUE__(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex4d(GLuint vertexIndex, const Vec4d &val)
{
  *(Vec4d*)__ATTRIBUTE_VALUE__(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}

void ShaderInput::setVertex1ui(GLuint vertexIndex, const GLuint &val)
{
  *__ATTRIBUTE_VALUE__(vertexIndex,GLuint) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex2ui(GLuint vertexIndex, const Vec2ui &val)
{
  *(Vec2ui*)__ATTRIBUTE_VALUE__(vertexIndex,GLuint) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex3ui(GLuint vertexIndex, const Vec3ui &val)
{
  *(Vec3ui*)__ATTRIBUTE_VALUE__(vertexIndex,GLuint) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex4ui(GLuint vertexIndex, const Vec4ui &val)
{
  *(Vec4ui*)__ATTRIBUTE_VALUE__(vertexIndex,GLuint) = val;
  stamp_ += 1;
}

void ShaderInput::setVertex1i(GLuint vertexIndex, const GLint &val)
{
  *__ATTRIBUTE_VALUE__(vertexIndex,GLint) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex2i(GLuint vertexIndex, const Vec2i &val)
{
  *(Vec2i*)__ATTRIBUTE_VALUE__(vertexIndex,GLint) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex3i(GLuint vertexIndex, const Vec3i &val)
{
  *(Vec3i*)__ATTRIBUTE_VALUE__(vertexIndex,GLint) = val;
  stamp_ += 1;
}
void ShaderInput::setVertex4i(GLuint vertexIndex, const Vec4i &val)
{
  *(Vec4i*)__ATTRIBUTE_VALUE__(vertexIndex,GLint) = val;
  stamp_ += 1;
}

const GLfloat& ShaderInput::getVertex1f(GLuint vertexIndex) const
{
  return *__ATTRIBUTE_VALUE__(vertexIndex,GLfloat);
}
const Vec2f& ShaderInput::getVertex2f(GLuint vertexIndex) const
{
  return *(Vec2f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat);
}
const Vec3f& ShaderInput::getVertex3f(GLuint vertexIndex) const
{
  return *(Vec3f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat);
}
const Vec4f& ShaderInput::getVertex4f(GLuint vertexIndex) const
{
  return *(Vec4f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat);
}
const Mat3f& ShaderInput::getVertex9f(GLuint vertexIndex) const
{
  return *(Mat3f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat);
}
const Mat4f& ShaderInput::getVertex16f(GLuint vertexIndex) const
{
  return *(Mat4f*)__ATTRIBUTE_VALUE__(vertexIndex,GLfloat);
}

const GLdouble& ShaderInput::getVertex1d(GLuint vertexIndex) const
{
  return *__ATTRIBUTE_VALUE__(vertexIndex,GLdouble);
}
const Vec2d& ShaderInput::getVertex2d(GLuint vertexIndex) const
{
  return *(Vec2d*)__ATTRIBUTE_VALUE__(vertexIndex,GLdouble);
}
const Vec3d& ShaderInput::getVertex3d(GLuint vertexIndex) const
{
  return *(Vec3d*)__ATTRIBUTE_VALUE__(vertexIndex,GLdouble);
}
const Vec4d& ShaderInput::getVertex4d(GLuint vertexIndex) const
{
  return *(Vec4d*)__ATTRIBUTE_VALUE__(vertexIndex,GLdouble);
}

const GLuint& ShaderInput::getVertex1ui(GLuint vertexIndex) const
{
  return *__ATTRIBUTE_VALUE__(vertexIndex,GLuint);
}
const Vec2ui& ShaderInput::getVertex2ui(GLuint vertexIndex) const
{
  return *(Vec2ui*)__ATTRIBUTE_VALUE__(vertexIndex,GLuint);
}
const Vec3ui& ShaderInput::getVertex3ui(GLuint vertexIndex) const
{
  return *(Vec3ui*)__ATTRIBUTE_VALUE__(vertexIndex,GLuint);
}
const Vec4ui& ShaderInput::getVertex4ui(GLuint vertexIndex) const
{
  return *(Vec4ui*)__ATTRIBUTE_VALUE__(vertexIndex,GLuint);
}

const GLint& ShaderInput::getVertex1i(GLuint vertexIndex) const
{
  return *__ATTRIBUTE_VALUE__(vertexIndex,GLint);
}
const Vec2i& ShaderInput::getVertex2i(GLuint vertexIndex) const
{
  return *(Vec2i*)__ATTRIBUTE_VALUE__(vertexIndex,GLint);
}
const Vec3i& ShaderInput::getVertex3i(GLuint vertexIndex) const
{
  return *(Vec3i*)__ATTRIBUTE_VALUE__(vertexIndex,GLint);
}
const Vec4i& ShaderInput::getVertex4i(GLuint vertexIndex) const
{
  return *(const Vec4i*)__ATTRIBUTE_VALUE__(vertexIndex,GLint);
}

#undef __ATTRIBUTE_VALUE__

/////////////
/////////////
////////////

void ShaderInput::setUniformDataUntyped(byte *data)
{
  setInstanceData(1,1,data);
  isVertexAttribute_ = GL_FALSE;
}

void ShaderInput::setVertexData(
    GLuint numVertices,
    const byte *vertexData)
{
  isVertexAttribute_ = GL_TRUE;
  numVertices_ = numVertices;
  numInstances_ = 1u;
  divisor_ = 0u;
  GLuint size = elementSize_*numVertices_;
  if(inputSize_ != size) {
    if(data_!=NULL) {
      data_ = (byte*) realloc(data_, size);
    } else {
      data_ = (byte*) malloc(size);
    }
    inputSize_ = size;
  }
  if(vertexData) {
    std::memcpy(data_, vertexData, inputSize_);
  }
  stamp_ += 1;
  // make new data stack root
  dataStack_.popBottom();
  dataStack_.pushBottom(data_);
}

void ShaderInput::setInstanceData(
    GLuint numInstances,
    GLuint divisor,
    const byte *instanceData)
{
  isVertexAttribute_ = GL_TRUE;
  numInstances_ = max(1u,numInstances);
  divisor_ = max(1u,divisor);
  numVertices_ = 1u;
  GLuint size = elementSize_*numInstances_/divisor_;
  if(inputSize_ != size) {
    if(data_!=NULL) {
      data_ = (byte*) realloc(data_, size);
    } else {
      data_ = (byte*) malloc(size);
    }
    inputSize_ = size;
  }
  if(instanceData) {
    std::memcpy(data_, instanceData, inputSize_);
  }
  stamp_ += 1;
  // make new data stack root
  dataStack_.popBottom();
  dataStack_.pushBottom(data_);
}

void ShaderInput::deallocateData()
{
  // set null data pointer
  dataStack_.popBottom();
  dataStack_.pushBottom(NULL);
  // and delete the data
  if(data_!=NULL) {
    free(data_);
    data_ = NULL;
  }
}

GLboolean ShaderInput::hasData()
{
  return data_!=NULL || buffer_!=0;
}

const byte* ShaderInput::data() const
{
  return dataStack_.top();
}
byte* ShaderInput::dataPtr()
{
  return dataStack_.top();
}
byte* ShaderInput::ownedData()
{
  return data_;
}

void ShaderInput::pushData(byte *data)
{
  dataStack_.push(data);
  stamp_ += 1;
}
void ShaderInput::popData()
{
  dataStack_.pop();
  stamp_ += 1;
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

ref_ptr<ShaderInput> ShaderInput::copy(const ref_ptr<ShaderInput> &in, GLboolean copyData)
{
  ref_ptr<ShaderInput> cp = create(in->name(), in->dataType(), in->valsPerElement());
  cp->stride_ = in->stride_;
  cp->offset_ = in->offset_;
  cp->inputSize_ = in->inputSize_;
  cp->elementSize_ = in->elementSize_;
  cp->elementCount_ = in->elementCount_;
  cp->numVertices_ = in->numVertices_;
  cp->numInstances_ = in->numInstances_;
  cp->divisor_ = in->divisor_;
  cp->buffer_ = 0;
  cp->bufferStamp_ = 0;
  cp->normalize_ = in->normalize_;
  cp->isVertexAttribute_ = in->isVertexAttribute_;
  cp->isConstant_ = in->isConstant_;
  cp->transpose_ = in->transpose_;
  cp->stamp_ = in->stamp_;
  cp->forceArray_ = in->forceArray_;

  cp->data_ = (byte*) malloc(cp->inputSize_);
  if(copyData && in->data_!=NULL) {
    std::memcpy(cp->data_, in->data_, cp->inputSize_);
  }
  // make data_ stack root
  cp->dataStack_.push(cp->data_);

  return cp;
}

/////////////
/////////////
////////////

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
  enableAttribute_ = &ShaderInput::enableAttributei;
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
  enableAttribute_ = &ShaderInput::enableAttributei;
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
  enableAttribute_ = &ShaderInput::enableAttributeMat3;
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
  enableAttribute_ = &ShaderInput::enableAttributeMat4;
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
