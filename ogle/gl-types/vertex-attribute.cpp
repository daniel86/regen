/*
 * vertex-attribute.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "vertex-attribute.h"

#include <ogle/utility/string-util.h>

VertexAttribute::VertexAttribute(
          const string &name,
          GLenum dataType,
          GLuint dataTypeBytes,
          GLuint valsPerElement,
          GLboolean normalize,
          GLuint elementCount)
  : name_(name),
    dataType_(dataType),
    dataTypeBytes_(dataTypeBytes),
    valsPerElement_(valsPerElement),
    elementCount_(elementCount),
    normalize_(normalize),
    divisor_(0),
    stride_(0),
    offset_(0),
    size_(0),
    buffer_(0)
{
  elementSize_ = dataTypeBytes*valsPerElement*elementCount;
}
VertexAttribute::~VertexAttribute()
{
  deallocateData();
}

GLboolean VertexAttribute::hasData()
{
  return data_.get()!=NULL;
}

byte* VertexAttribute::dataPtr()
{
  return data_->data();
}

void VertexAttribute::setVertexData(
    GLuint numVertices,
    const byte *vertexData)
{
  numVertices_ = numVertices;
  numInstances_ = 0;
  divisor_ = 0;
  size_ = elementSize_*numVertices_;
  data_ = ref_ptr< vector<byte> >::manage( new vector<byte>(size_) );
  if(vertexData) {
    byte *ptr = &(*data_.get())[0];
    std::memcpy(ptr, vertexData, size_);
  }
}
void VertexAttribute::setInstanceData(
    GLuint numInstances,
    GLuint divisor,
    const byte *instanceData)
{
  numInstances_ = max(1u,numInstances);
  divisor_ = max(1u,divisor);
  numVertices_ = 0;
  size_ = elementSize_*numInstances_/divisor_;
  data_ = ref_ptr< vector<byte> >::manage( new vector<byte>(size_) );
  if(instanceData) {
    byte *ptr = &(*data_.get())[0];
    std::memcpy(ptr, instanceData, size_);
  }
}
void VertexAttribute::deallocateData()
{
  data_ = ref_ptr< vector<byte> >();
}

const string& VertexAttribute::name() const
{
  return name_;
}
void VertexAttribute::set_name(const string &s)
{
  name_ = s;
}
GLenum VertexAttribute::dataType() const
{
  return dataType_;
}
GLuint VertexAttribute::dataTypeBytes() const
{
  return dataTypeBytes_;
}
void VertexAttribute::set_stride(GLuint stride)
{
  stride_ = stride;
}
GLuint VertexAttribute::stride() const
{
  return stride_;
}
void VertexAttribute::set_buffer(GLuint buffer)
{
  buffer_ = buffer;
}
GLuint VertexAttribute::buffer() const
{
  return buffer_;
}
GLuint VertexAttribute::size() const
{
  return size_;
}
void VertexAttribute::set_size(GLuint size)
{
  size_ = size;
}
GLuint VertexAttribute::elementSize() const
{
    return elementSize_;
}
void VertexAttribute::set_offset(GLuint offset)
{
  offset_ = offset;
}
GLuint VertexAttribute::offset() const
{
  return offset_;
}
GLuint VertexAttribute::elementCount() const
{
    return elementCount_;
}
GLuint VertexAttribute::valsPerElement() const
{
  return valsPerElement_;
}
GLuint VertexAttribute::numInstances() const
{
  return numInstances_;
}
GLuint VertexAttribute::divisor()
{
  return divisor_;
}
GLuint VertexAttribute::numVertices() const
{
  return numVertices_;
}
GLboolean VertexAttribute::normalize() const
{
  return normalize_;
}

void VertexAttribute::enable(GLint location) const
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
    glVertexAttribDivisorARB(loc, divisor_);
  }
}

VertexAttributeI::VertexAttributeI(
    const string &name,
    GLenum dataType,
    GLuint dataTypeBytes,
    GLuint valsPerElement,
    GLuint elementCount)
: VertexAttribute(name, dataType,
    dataTypeBytes, valsPerElement,
    false, elementCount)
{
}
void VertexAttributeI::enable(GLint location) const
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
    glVertexAttribDivisorARB(loc, divisor_);
  }
}

VertexAttributefv::VertexAttributefv(
    const string &name,
    GLuint valsPerElement,
    GLboolean normalize)
: VertexAttribute(name, GL_FLOAT,
    sizeof(GLfloat), valsPerElement,
    normalize, 1)
{
}
VertexAttributeuiv::VertexAttributeuiv(
    const string &name,
    GLuint valsPerElement)
: VertexAttributeI(name, GL_UNSIGNED_INT,
    sizeof(GLuint), valsPerElement)
{
}
VertexAttributeiv::VertexAttributeiv(
    const string &name,
    GLuint valsPerElement)
: VertexAttributeI(name, GL_INT,
    sizeof(GLint), valsPerElement)
{
}

TexcoAttribute::TexcoAttribute(
    GLuint channel,
    GLuint valsPerElement,
    GLboolean normalize)
: VertexAttributefv(FORMAT_STRING("uv" << channel), valsPerElement, normalize),
  channel_(channel)
{
}
GLuint TexcoAttribute::channel() const
{
  return channel_;
}
TangentAttribute::TangentAttribute(GLboolean normalize)
: VertexAttributefv("tan", 4, normalize)
{
}

NormalAttribute::NormalAttribute(GLboolean normalize)
: VertexAttributefv(ATTRIBUTE_NAME_NOR, 3, normalize)
{
}

AttributeMat4::AttributeMat4(const string &name, GLboolean normalize)
: VertexAttribute(name, GL_FLOAT,
    sizeof(GLfloat), 16, normalize, 1)
{
}
void AttributeMat4::enable(GLint location) const
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

    glVertexAttribDivisorARB(loc0, divisor_);
    glVertexAttribDivisorARB(loc1, divisor_);
    glVertexAttribDivisorARB(loc2, divisor_);
    glVertexAttribDivisorARB(loc3, divisor_);
  }
}

AttributeMat3::AttributeMat3(const string &name, GLboolean normalize)
: VertexAttribute(name, GL_FLOAT,
    sizeof(GLfloat), 9, normalize, 1)
{
}
void AttributeMat3::enable(GLint location) const
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

    glVertexAttribDivisorARB(loc0, divisor_);
    glVertexAttribDivisorARB(loc1, divisor_);
    glVertexAttribDivisorARB(loc2, divisor_);
  }
}

AttributeMat2::AttributeMat2(const string &name, GLboolean normalize)
: VertexAttribute(name, GL_FLOAT,
    sizeof(GLfloat), 4, normalize, 1)
{
}
void AttributeMat2::enable(GLint location) const
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

    glVertexAttribDivisorARB(loc0, divisor_);
    glVertexAttribDivisorARB(loc1, divisor_);
  }
}

VertexAttributeUint::VertexAttributeUint(
      const string &name,
      GLuint valsPerElement)
: VertexAttributeI(name, GL_UNSIGNED_INT,
    sizeof(GLuint), valsPerElement)
{
}
