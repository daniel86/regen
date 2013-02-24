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
          GLuint elementCount,
          GLboolean normalize)
  : name_(name),
    dataType_(dataType),
    dataTypeBytes_(dataTypeBytes),
    stride_(0),
    offset_(0),
    size_(0),
    elementCount_(elementCount),
    numVertices_(0u),
    numInstances_(0u),
    valsPerElement_(valsPerElement),
    divisor_(0),
    buffer_(0),
    normalize_(normalize),
    isVertexAttribute_(GL_TRUE),
    data_(NULL)
{
  elementSize_ = dataTypeBytes*valsPerElement*elementCount;
}
VertexAttribute::VertexAttribute(
    const VertexAttribute &other,
    GLboolean copyData)
: name_(other.name_),
  dataType_(other.dataType_),
  dataTypeBytes_(other.dataTypeBytes_),
  stride_(other.stride_),
  offset_(other.offset_),
  size_(other.size_),
  elementSize_(other.elementSize_),
  elementCount_(other.elementCount_),
  numVertices_(other.numVertices_),
  numInstances_(other.numInstances_),
  valsPerElement_(other.valsPerElement_),
  divisor_(other.divisor_),
  buffer_(other.buffer_),
  normalize_(other.normalize_),
  isVertexAttribute_(other.isVertexAttribute_),
  stamp_(0u)
{
  data_ = new byte[size_];
  if(copyData) {
    std::memcpy(data_, other.data_, size_);
  }
}
VertexAttribute::~VertexAttribute()
{
  deallocateData();
}

GLuint VertexAttribute::stamp() const
{
  return stamp_;
}

GLboolean VertexAttribute::hasData()
{
  return data_!=NULL || buffer_!=0;
}

const byte* VertexAttribute::data() const
{
  return data_;
}
byte* VertexAttribute::dataPtr()
{
  return data_;
}
void VertexAttribute::set_dataPtr(byte *dataPtr)
{
  data_ = dataPtr;
}

void VertexAttribute::setVertexData(
    GLuint numVertices,
    const byte *vertexData)
{
  isVertexAttribute_ = GL_TRUE;
  numVertices_ = numVertices;
  numInstances_ = 1u;
  divisor_ = 0u;
  size_ = elementSize_*numVertices_;
  if(data_) {
    delete[] data_;
  }
  data_ = new byte[size_];
  if(vertexData) {
    std::memcpy(data_, vertexData, size_);
  }
  stamp_ += 1;
}
void VertexAttribute::setInstanceData(
    GLuint numInstances,
    GLuint divisor,
    const byte *instanceData)
{
  isVertexAttribute_ = GL_TRUE;
  numInstances_ = max(1u,numInstances);
  divisor_ = max(1u,divisor);
  numVertices_ = 1u;
  size_ = elementSize_*numInstances_/divisor_;
  data_ = new byte[size_];
  if(instanceData) {
    std::memcpy(data_, instanceData, size_);
  }
  stamp_ += 1;
}
void VertexAttribute::deallocateData()
{
  if(data_!=NULL) delete[] data_;
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
void VertexAttribute::set_bufferIterator(VBOBlockIterator it)
{
  bufferIterator_ = it;
}
VBOBlockIterator VertexAttribute::bufferIterator()
{
  return bufferIterator_;
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
void VertexAttribute::set_elementCount(GLuint v)
{
  elementCount_ = v;
}
GLuint VertexAttribute::valsPerElement() const
{
  return valsPerElement_;
}
GLuint VertexAttribute::numInstances() const
{
  return numInstances_;
}
GLuint VertexAttribute::divisor() const
{
  return divisor_;
}
GLuint VertexAttribute::numVertices() const
{
  return numVertices_;
}
void VertexAttribute::set_numVertices(GLuint numVertices)
{
  numVertices_ = numVertices;
}
GLboolean VertexAttribute::normalize() const
{
  return normalize_;
}
void VertexAttribute::set_transpose(GLboolean transpose)
{
  transpose_ = transpose;
}
GLboolean VertexAttribute::transpose() const
{
  return transpose_;
}

#define ATTRIBUTE_VALUE(vertexIndex, Type) \
    (((Type*) dataPtr()) + (vertexIndex*valsPerElement()) )

void VertexAttribute::setVertex1f(GLuint vertexIndex, const GLfloat &val)
{
  *ATTRIBUTE_VALUE(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex2f(GLuint vertexIndex, const Vec2f &val)
{
  *(Vec2f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex3f(GLuint vertexIndex, const Vec3f &val)
{
  *(Vec3f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex4f(GLuint vertexIndex, const Vec4f &val)
{
  *(Vec4f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex9f(GLuint vertexIndex, const Mat3f &val)
{
  *(Mat3f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex16f(GLuint vertexIndex, const Mat4f &val)
{
  *(Mat4f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat) = val;
  stamp_ += 1;
}

void VertexAttribute::setVertex1d(GLuint vertexIndex, const GLdouble &val)
{
  *ATTRIBUTE_VALUE(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex2d(GLuint vertexIndex, const Vec2d &val)
{
  *(Vec2d*)ATTRIBUTE_VALUE(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex3d(GLuint vertexIndex, const Vec3d &val)
{
  *(Vec3d*)ATTRIBUTE_VALUE(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex4d(GLuint vertexIndex, const Vec4d &val)
{
  *(Vec4d*)ATTRIBUTE_VALUE(vertexIndex,GLdouble) = val;
  stamp_ += 1;
}

void VertexAttribute::setVertex1ui(GLuint vertexIndex, const GLuint &val)
{
  *ATTRIBUTE_VALUE(vertexIndex,GLuint) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex2ui(GLuint vertexIndex, const Vec2ui &val)
{
  *(Vec2ui*)ATTRIBUTE_VALUE(vertexIndex,GLuint) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex3ui(GLuint vertexIndex, const Vec3ui &val)
{
  *(Vec3ui*)ATTRIBUTE_VALUE(vertexIndex,GLuint) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex4ui(GLuint vertexIndex, const Vec4ui &val)
{
  *(Vec4ui*)ATTRIBUTE_VALUE(vertexIndex,GLuint) = val;
  stamp_ += 1;
}

void VertexAttribute::setVertex1i(GLuint vertexIndex, const GLint &val)
{
  *ATTRIBUTE_VALUE(vertexIndex,GLint) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex2i(GLuint vertexIndex, const Vec2i &val)
{
  *(Vec2i*)ATTRIBUTE_VALUE(vertexIndex,GLint) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex3i(GLuint vertexIndex, const Vec3i &val)
{
  *(Vec3i*)ATTRIBUTE_VALUE(vertexIndex,GLint) = val;
  stamp_ += 1;
}
void VertexAttribute::setVertex4i(GLuint vertexIndex, const Vec4i &val)
{
  *(Vec4i*)ATTRIBUTE_VALUE(vertexIndex,GLint) = val;
  stamp_ += 1;
}

/////

GLfloat& VertexAttribute::getVertex1f(GLuint vertexIndex)
{
  return *ATTRIBUTE_VALUE(vertexIndex,GLfloat);
}
Vec2f& VertexAttribute::getVertex2f(GLuint vertexIndex)
{
  return *(Vec2f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat);
}
Vec3f& VertexAttribute::getVertex3f(GLuint vertexIndex)
{
  return *(Vec3f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat);
}
Vec4f& VertexAttribute::getVertex4f(GLuint vertexIndex)
{
  return *(Vec4f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat);
}
Mat3f& VertexAttribute::getVertex9f(GLuint vertexIndex)
{
  return *(Mat3f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat);
}
Mat4f& VertexAttribute::getVertex16f(GLuint vertexIndex)
{
  return *(Mat4f*)ATTRIBUTE_VALUE(vertexIndex,GLfloat);
}

GLdouble& VertexAttribute::getVertex1d(GLuint vertexIndex)
{
  return *ATTRIBUTE_VALUE(vertexIndex,GLdouble);
}
Vec2d& VertexAttribute::getVertex2d(GLuint vertexIndex)
{
  return *(Vec2d*)ATTRIBUTE_VALUE(vertexIndex,GLdouble);
}
Vec3d& VertexAttribute::getVertex3d(GLuint vertexIndex)
{
  return *(Vec3d*)ATTRIBUTE_VALUE(vertexIndex,GLdouble);
}
Vec4d& VertexAttribute::getVertex4d(GLuint vertexIndex)
{
  return *(Vec4d*)ATTRIBUTE_VALUE(vertexIndex,GLdouble);
}

GLuint& VertexAttribute::getVertex1ui(GLuint vertexIndex)
{
  return *ATTRIBUTE_VALUE(vertexIndex,GLuint);
}
Vec2ui& VertexAttribute::getVertex2ui(GLuint vertexIndex)
{
  return *(Vec2ui*)ATTRIBUTE_VALUE(vertexIndex,GLuint);
}
Vec3ui& VertexAttribute::getVertex3ui(GLuint vertexIndex)
{
  return *(Vec3ui*)ATTRIBUTE_VALUE(vertexIndex,GLuint);
}
Vec4ui& VertexAttribute::getVertex4ui(GLuint vertexIndex)
{
  return *(Vec4ui*)ATTRIBUTE_VALUE(vertexIndex,GLuint);
}

GLint& VertexAttribute::getVertex1i(GLuint vertexIndex)
{
  return *ATTRIBUTE_VALUE(vertexIndex,GLint);
}
Vec2i& VertexAttribute::getVertex2i(GLuint vertexIndex)
{
  return *(Vec2i*)ATTRIBUTE_VALUE(vertexIndex,GLint);
}
Vec3i& VertexAttribute::getVertex3i(GLuint vertexIndex)
{
  return *(Vec3i*)ATTRIBUTE_VALUE(vertexIndex,GLint);
}
Vec4i& VertexAttribute::getVertex4i(GLuint vertexIndex)
{
  return *(Vec4i*)ATTRIBUTE_VALUE(vertexIndex,GLint);
}

#undef ATTRIBUTE_VALUE

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
void VertexAttribute::enablei(GLint location) const
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
void VertexAttribute::enableMat4(GLint location) const
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
void VertexAttribute::enableMat3(GLint location) const
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
void VertexAttribute::enableMat2(GLint location) const
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
