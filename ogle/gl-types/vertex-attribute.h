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
#include <ogle/algebra/matrix.h>

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

#include <ogle/gl-types/vbo.h>

/**
 * Vertex attributes are used to communicate from "outside"
 * to the vertex shader. Unlike uniform variables,
 * values are provided per vertex (and not globally for all vertices).
 */
class VertexAttribute
{
public:
  VertexAttribute(
          const string &name,
          GLenum dataType,
          GLuint dataTypeBytes,
          GLuint valsPerElement,
          GLuint elementCount,
          GLboolean normalize);
  VertexAttribute(
      const VertexAttribute &other,
      GLboolean copyData=GL_FALSE);
  ~VertexAttribute();

  /**
   * Compare stamps to check if the input data changed.
   */
  GLuint stamp() const;

  /**
   * Name of this attribute used in shader programs.
   */
  const string& name() const;
  /**
   * Name of this attribute used in shader programs.
   */
  void set_name(const string &s);

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
  void set_bufferIterator(VBOBlockIterator);
  VBOBlockIterator bufferIterator();
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
  void set_elementCount(GLuint);
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
  GLuint divisor() const;
  /**
   * Specifies whether fixed-point data values should be normalized (GL_TRUE)
   * or converted directly as fixed-point values (GL_FALSE) when they are accessed.
   */
  GLboolean normalize() const;

  void set_transpose(GLboolean transpose);
  GLboolean transpose() const;

  GLuint numVertices() const;
  void set_numVertices(GLuint numVertices);

  /**
   * Bind the attribute to the given shader location.
   */
  void enable(GLint location) const;
  /**
   * only the integer types GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT,
   * GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT are accepted.
   * Values are always left as integer values.
   */
  void enablei(GLint location) const;
  /**
   * Matrix attributes have special enable functions.
   */
  void enableMat4(GLint location) const;
  /**
   * Matrix attributes have special enable functions.
   */
  void enableMat3(GLint location) const;
  /**
   * Matrix attributes have special enable functions.
   */
  void enableMat2(GLint location) const;

  /**
   * Allocates RAM for the attribute and does a memcpy
   * if the data pointer is not null.
   * numVertices*elementSize bytes will be allocated.
   */
  void setVertexData(
      GLuint numVertices,
      const byte *vertexData=NULL);
  /**
   * Allocates RAM for the attribute and does a memcpy
   * if the data pointer is not null.
   * numInstances*elementSize/divisor bytes will be allocated.
   */
  void setInstanceData(
      GLuint numInstances,
      GLuint divisor,
      const byte *instanceData=NULL);

  /**
   * Vertex data pointer.
   * Initially NULL, must be allocated with setVertexData or setInstanceData.
   */
  byte* dataPtr();
  const byte* data() const;
  void set_dataPtr(byte*);

  /**
   * Returns true if this attribute is allocated in RAM.
   */
  GLboolean hasData();
  /**
   * Deallocates RAM space for previously allocated attribute.
   * If the attribute data is saved in a VBO you may want
   * to free the data from RAM.
   */
  void deallocateData();

  void setVertex1f(GLuint vertexIndex, const GLfloat &val);
  void setVertex2f(GLuint vertexIndex, const Vec2f &val);
  void setVertex3f(GLuint vertexIndex, const Vec3f &val);
  void setVertex4f(GLuint vertexIndex, const Vec4f &val);
  void setVertex9f(GLuint vertexIndex, const Mat3f &val);
  void setVertex16f(GLuint vertexIndex, const Mat4f &val);

  void setVertex1d(GLuint vertexIndex, const GLdouble &val);
  void setVertex2d(GLuint vertexIndex, const Vec2d &val);
  void setVertex3d(GLuint vertexIndex, const Vec3d &val);
  void setVertex4d(GLuint vertexIndex, const Vec4d &val);

  void setVertex1ui(GLuint vertexIndex, const GLuint &val);
  void setVertex2ui(GLuint vertexIndex, const Vec2ui &val);
  void setVertex3ui(GLuint vertexIndex, const Vec3ui &val);
  void setVertex4ui(GLuint vertexIndex, const Vec4ui &val);

  void setVertex1i(GLuint vertexIndex, const GLint &val);
  void setVertex2i(GLuint vertexIndex, const Vec2i &val);
  void setVertex3i(GLuint vertexIndex, const Vec3i &val);
  void setVertex4i(GLuint vertexIndex, const Vec4i &val);

  GLfloat& getVertex1f(GLuint vertexIndex);
  Vec2f& getVertex2f(GLuint vertexIndex);
  Vec3f& getVertex3f(GLuint vertexIndex);
  Vec4f& getVertex4f(GLuint vertexIndex);
  Mat3f& getVertex9f(GLuint vertexIndex);
  Mat4f& getVertex16f(GLuint vertexIndex);

  GLdouble& getVertex1d(GLuint vertexIndex);
  Vec2d& getVertex2d(GLuint vertexIndex);
  Vec3d& getVertex3d(GLuint vertexIndex);
  Vec4d& getVertex4d(GLuint vertexIndex);

  GLuint& getVertex1ui(GLuint vertexIndex);
  Vec2ui& getVertex2ui(GLuint vertexIndex);
  Vec3ui& getVertex3ui(GLuint vertexIndex);
  Vec4ui& getVertex4ui(GLuint vertexIndex);

  GLint& getVertex1i(GLuint vertexIndex);
  Vec2i& getVertex2i(GLuint vertexIndex);
  Vec3i& getVertex3i(GLuint vertexIndex);
  Vec4i& getVertex4i(GLuint vertexIndex);

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
  VBOBlockIterator bufferIterator_;
  GLboolean normalize_;
  GLboolean isVertexAttribute_;
  GLboolean transpose_;
  byte *data_;
  GLuint stamp_;
};

typedef list< ref_ptr<VertexAttribute> >::const_iterator AttributeIteratorConst;

#endif /* VERTEX_ATTRIBUTE_H_ */
