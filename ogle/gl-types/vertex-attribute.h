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

#include <ogle/utility/stack.h>
#include <ogle/utility/logging.h>
#include <ogle/utility/ref-ptr.h>
#include <ogle/algebra/vector.h>
#include <ogle/algebra/matrix.h>
#include <ogle/gl-types/vbo.h>

namespace ogle {

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
   * Sets a new stamp value.
   */
  void nextStamp();

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
   * Iterator should be exclusively owned by this instance.
   */
  void set_buffer(GLuint buffer, VBOBlockIterator it);
  /**
   * VBO that contains this vertex data.
   */
  GLuint buffer() const;
  /**
   * data with stamp was uploaded to GL.
   */
  GLuint bufferStamp() const;
  /**
   * Iterator to allocated VBO block.
   */
  VBOBlockIterator& bufferIterator();
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
   * Returns pointer owned by this instance or the top of the
   * data pointer stack.
   */
  byte* dataPtr();
  /**
   * Vertex data pointer.
   * Returns pointer owned by this instance or the top of the
   * data pointer stack.
   */
  const byte* data() const;

  /**
   * Pushes a data pointer onto the stack without doing a copy.
   * Caller have to make sure the pointer stays valid until the data
   * is pushed.
   */
  void pushData(byte *data);
  /**
   * Pop data pointer you previously pushed.
   * This does not delete the data pointer, it's owned by caller.
   * Last pop will reset to data pointer owned by this instance.
   */
  void popData();

  /**
   * Deallocates data pointer owned by this instance.
   */
  void deallocateData();

  /**
   * Returns true if this attribute is allocated in RAM
   * or if it was uploaded to GL already.
   */
  GLboolean hasData();

  /// Note: below functions are applied to active stack data only.

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

  const GLfloat& getVertex1f(GLuint vertexIndex) const;
  const Vec2f& getVertex2f(GLuint vertexIndex) const;
  const Vec3f& getVertex3f(GLuint vertexIndex) const;
  const Vec4f& getVertex4f(GLuint vertexIndex) const;
  const Mat3f& getVertex9f(GLuint vertexIndex) const;
  const Mat4f& getVertex16f(GLuint vertexIndex) const;

  const GLdouble& getVertex1d(GLuint vertexIndex) const;
  const Vec2d& getVertex2d(GLuint vertexIndex) const;
  const Vec3d& getVertex3d(GLuint vertexIndex) const;
  const Vec4d& getVertex4d(GLuint vertexIndex) const;

  const GLuint& getVertex1ui(GLuint vertexIndex) const;
  const Vec2ui& getVertex2ui(GLuint vertexIndex) const;
  const Vec3ui& getVertex3ui(GLuint vertexIndex) const;
  const Vec4ui& getVertex4ui(GLuint vertexIndex) const;

  const GLint& getVertex1i(GLuint vertexIndex) const;
  const Vec2i& getVertex2i(GLuint vertexIndex) const;
  const Vec3i& getVertex3i(GLuint vertexIndex) const;
  const Vec4i& getVertex4i(GLuint vertexIndex) const;

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
  GLuint bufferStamp_;
  VBOBlockIterator bufferIterator_;
  GLboolean normalize_;
  GLboolean isVertexAttribute_;
  GLboolean transpose_;
  byte *data_;
  Stack<byte*> dataStack_;
  GLuint stamp_;
};

typedef list< ref_ptr<VertexAttribute> >::const_iterator AttributeIteratorConst;

} // end ogle namespace

#endif /* VERTEX_ATTRIBUTE_H_ */
