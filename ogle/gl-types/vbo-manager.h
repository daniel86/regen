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

#include <ogle/gl-types/vbo.h>
#include <ogle/gl-types/vertex-attribute.h>

class VBOManager
{
public:
  static const ref_ptr<VertexBufferObject>& activeBuffer();

  static void set_defaultBufferSize(GLuint v);
  static GLuint set_defaultBufferSize();

  static void set_defaultUsage(VertexBufferObject::Usage v);
  static VertexBufferObject::Usage set_defaultUsage();

  static void add(const ref_ptr<VertexAttribute> &in);

  static void remove(const ref_ptr<VertexAttribute> &in);

protected:
  static GLuint defaultBufferSize_;
  static VertexBufferObject::Usage defaultUsage_;

  static ref_ptr<VertexBufferObject> activeVBO_;
  static map<GLuint, ref_ptr<VertexBufferObject> > bufferIDs_;

  static void createBuffer(
      GLuint bufferSize, GLuint minSize,
      VertexBufferObject::Usage usage=VertexBufferObject::USAGE_DYNAMIC);
};

#endif /* VBO_NODE_H_ */
