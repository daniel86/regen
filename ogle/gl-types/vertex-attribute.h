/*
 * vertex-attribute.h
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#ifndef VERTEX_ATTRIBUTE_H_
#define VERTEX_ATTRIBUTE_H_

#include <GL/glew.h>
#include <GL/gl.h>

#include <cstdlib>
#include <sstream>
#include <vector>
#include <cstring>
using namespace std;

#include <ogle/utility/logging.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/vector.h>

// default attribute names
#define ATTRIBUTE_NAME_POS "pos"
#define ATTRIBUTE_NAME_NOR "nor"
#define ATTRIBUTE_NAME_TAN "tan"
#define ATTRIBUTE_NAME_COL0 "col0"
#define ATTRIBUTE_NAME_COL1 "col1"

/**
 * Offset in the VBO
 */
#ifndef BUFFER_OFFSET
  #define BUFFER_OFFSET(i) ((char *)NULL + (i))
#endif

#ifndef byte
  typedef unsigned char byte;
#endif

/**
 * Vertex attributes are used to communicate from "outside"
 * to the vertex shader. Unlike uniform variables,
 * values are provided per vertex (and not globally for all vertices).
 */
class VertexAttribute
{
public:
  /**
   * The data pointer.
   */
  ref_ptr< vector<byte> > data;

  VertexAttribute(
          const string &name,
          GLenum dataType=GL_FLOAT,
          GLuint dataTypeBytes=sizeof(GLfloat),
          GLuint valsPerElement=3,
          GLboolean normalize=false,
          GLuint elementCount=1);
  ~VertexAttribute();

  /**
   * Name of this attribute used in shader programs.
   */
  const string& name() const;
  /**
   * Name of this attribute used in shader programs.
   */
  void set_name(const string &s);

  /**
   * Vertex data pointer.
   * Initially NULL.
   */
  byte* dataPtr();

  /**
   * Sets data of the attribute.
   */
  void setVertexData(
      GLuint numVertices,
      const byte *vertexData=NULL);
  /**
   * Sets data of the attribute.
   */
  void setInstanceData(
      GLuint numInstances,
      GLuint divisor,
      const byte *instanceData=NULL);
  void deallocateData();

  /**
   * Specifies the data type of each component in the array.
   * Symbolic constants GL_FLOAT,GL_DOUBLE,.. accepted.
   */
  GLenum dataType() const;
  /**
   * Size of a single instance of the data type in bytes.
   */
  GLuint dataTypeBytes() const;
  /**
   * Specifies the byte offset between consecutive generic vertex attributes.
   * If stride is 0, the generic vertex attributes are understood to be tightly
   * packed in the array. The initial value is 0.
   */
  void set_stride(GLuint stride);
  /**
   * VBO that contains this vertex data.
   */
  void set_buffer(GLuint buffer);
  /**
   * VBO that contains this vertex data.
   */
  GLuint buffer() const;
  /**
   * Specifies the byte offset between consecutive generic vertex attributes.
   * If stride is 0, the generic vertex attributes are understood to be tightly
   * packed in the array. The initial value is 0.
   */
  GLuint stride() const;
  /**
   * Attribute size for all vertices.
   */
  GLuint size() const;
  /**
   * Attribute size for all vertices.
   */
  void set_size(GLuint size);
  /**
   * Attribute size for a single vertex.
   */
  GLuint elementSize() const;
  /**
   * Offset in the VBO to the first
   * attribute element.
   */
  void set_offset(GLuint offset);
  /**
   * Offset in the VBO to the first
   * attribute element.
   */
  GLuint offset() const;
  /**
   * Number of array elements.
   * returns 1 if this is not an array attribute.
   */
  GLuint elementCount() const;
  /**
   * Specifies the number of components per generic vertex attribute.
   * Must be 1, 2, 3, or 4.
   */
  GLuint valsPerElement() const;
  /**
   * Used for instanced attributes.
   */
  GLuint numInstances() const;
  /**
   * Specify the number of instances that will pass between updates
   * of the generic attribute at slot index.
   */
  GLuint divisor();
  /**
   * Specifies whether fixed-point data values should be normalized (GL_TRUE)
   * or converted directly as fixed-point values (GL_FALSE) when they are accessed.
   */
  GLboolean normalize() const;

  GLuint numVertices() const;

  virtual void enable(GLint location) const;
protected:
  string name_;
  GLenum dataType_;
  GLuint dataTypeBytes_;
  GLuint stride_;
  GLuint offset_;
  GLuint size_;
  GLuint elementSize_;
  GLuint elementCount_;
  GLuint numVertices_;
  GLuint numInstances_;
  GLuint valsPerElement_;
  GLuint divisor_;
  GLuint buffer_;
  GLboolean normalize_;
};

class VertexAttributeI : public VertexAttribute {
public:
  VertexAttributeI(
      const string &name,
      GLenum dataType=GL_FLOAT,
      GLuint dataTypeBytes=sizeof(GLfloat),
      GLuint valsPerElement=3,
      GLuint elementCount=1);
  virtual void enable(GLint location) const;
};

/**
 * n-dimensional float vector attribute.
 */
class VertexAttributefv : public VertexAttribute {
public:
  VertexAttributefv(const string &name,
      GLuint valsPerElement=3,
      GLboolean normalize=false);
};
class VertexAttributeuiv : public VertexAttributeI {
public:
  VertexAttributeuiv(
      const string &name,
      GLuint valsPerElement=3);
};
class VertexAttributeiv : public VertexAttributeI {
public:
  VertexAttributeiv(
      const string &name,
      GLuint valsPerElement=3);
};

class TexcoAttribute : public VertexAttributefv
{
public:
  TexcoAttribute(
      GLuint channel,
      GLuint valsPerElement=3,
      GLboolean normalize=false);
  GLuint channel() const;
protected:
  GLuint channel_;
};
class TangentAttribute : public VertexAttributefv
{
public:
  TangentAttribute(
      GLuint valsPerElement=3,
      GLboolean normalize=false);
};
class NormalAttribute : public VertexAttributefv
{
public:
  NormalAttribute(GLboolean normalize=false);
};

class AttributeMat4 : public VertexAttribute
{
public:
  AttributeMat4(const string &name, GLboolean normalize=false);
  virtual void enable(GLint location) const;
};
class AttributeMat3 : public VertexAttribute
{
public:
  AttributeMat3(const string &name, GLboolean normalize=false);
  virtual void enable(GLint location) const;
};
class AttributeMat2 : public VertexAttribute
{
public:
  AttributeMat2(const string &name, GLboolean normalize=false);
  virtual void enable(GLint location) const;
};

/**
 * vertex attribute of unsigned integers.
 */
class VertexAttributeUint : public VertexAttributeI {
public:
  VertexAttributeUint(
      const string &name,
      unsigned int valsPerElement=1);
};

#define ATTRIBUTE_VALUE(att, vertexIndex, Type) \
    (((Type*) att->data->data()) + (vertexIndex*att->valsPerElement()) )

inline void setAttributeVertex1f(
    VertexAttributefv *att,
    GLuint vertexIndex,
    const GLfloat &val)
{
  *ATTRIBUTE_VALUE(att,vertexIndex,GLfloat) = val;
}
inline void setAttributeVertex2f(
    VertexAttributefv *att,
    GLuint vertexIndex,
    const Vec2f &val)
{
  *(Vec2f*)ATTRIBUTE_VALUE(att,vertexIndex,GLfloat) = val;
}
inline void setAttributeVertex3f(
    VertexAttributefv *att,
    GLuint vertexIndex,
    const Vec3f &val)
{
  *(Vec3f*)ATTRIBUTE_VALUE(att,vertexIndex,GLfloat) = val;
}
inline void setAttributeVertex4f(
    VertexAttributefv *att,
    GLuint vertexIndex,
    const Vec4f &val)
{
  *(Vec4f*)ATTRIBUTE_VALUE(att,vertexIndex,GLfloat) = val;
}

inline void setAttributeVertex1ui(
    VertexAttributeuiv *att,
    GLuint vertexIndex,
    const GLuint &val)
{
  *ATTRIBUTE_VALUE(att,vertexIndex,GLuint) = val;
}
inline void setAttributeVertex2ui(
    VertexAttributeuiv *att,
    GLuint vertexIndex,
    const Vec2ui &val)
{
  *(Vec2ui*)ATTRIBUTE_VALUE(att,vertexIndex,GLuint) = val;
}
inline void setAttributeVertex3ui(
    VertexAttributeuiv *att,
    GLuint vertexIndex,
    const Vec3ui &val)
{
  *(Vec3ui*)ATTRIBUTE_VALUE(att,vertexIndex,GLuint) = val;
}
inline void setAttributeVertex4ui(
    VertexAttributeuiv *att,
    GLuint vertexIndex,
    const Vec4ui &val)
{
  *(Vec4ui*)ATTRIBUTE_VALUE(att,vertexIndex,GLuint) = val;
}

#undef ATTRIBUTE_VALUE

#endif /* VERTEX_ATTRIBUTE_H_ */
