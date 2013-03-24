/*
 * vbo-manager.h
 *
 *  Created on: 02.08.2012
 *      Author: daniel
 */

#ifndef VBO_NODE_H_
#define VBO_NODE_H_

using namespace std;
#include <stack>
#include <map>

#include <regen/gl-types/vbo.h>
#include <regen/gl-types/vertex-attribute.h>

namespace ogle {
/**
 * \brief Helps using VertexBufferObject's.
 */
class VBOManager
{
public:
  /**
   * @return the currently active VertexBufferObject's.
   */
  static const ref_ptr<VertexBufferObject>& activeBuffer();

  /**
   * @param v the default VertexBufferObject size.
   */
  static void set_defaultBufferSize(GLuint v);
  /**
   * @return the default VertexBufferObject size.
   */
  static GLuint set_defaultBufferSize();

  /**
   * @param v default usage hint.
   */
  static void set_defaultUsage(VertexBufferObject::Usage v);
  /**
   * @return default usage hint.
   */
  static VertexBufferObject::Usage set_defaultUsage();

  /**
   * Adds a VertexAttribute to a VertexBufferObject.
   * @param in the VertexAttribute.
   */
  static void add(const ref_ptr<VertexAttribute> &in);
  /**
   * Removes previously added VertexAttribute.
   * @param in the VertexAttribute.
   */
  static void remove(VertexAttribute &in);

protected:
  static GLuint defaultBufferSize_;
  static VertexBufferObject::Usage defaultUsage_;

  static ref_ptr<VertexBufferObject> activeVBO_;
  static map<GLuint, ref_ptr<VertexBufferObject> > bufferIDs_;

  static void createBuffer(
      GLuint bufferSize, GLuint minSize,
      VertexBufferObject::Usage usage=VertexBufferObject::USAGE_DYNAMIC);
};

} // end ogle namespace

#endif /* VBO_NODE_H_ */
