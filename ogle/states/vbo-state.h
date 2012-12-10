/*
 * vbo-node.h
 *
 *  Created on: 02.08.2012
 *      Author: daniel
 */

#ifndef VBO_NODE_H_
#define VBO_NODE_H_

#include <stack>

#include <ogle/states/state.h>
#include <ogle/states/shader-input-state.h>
#include <ogle/gl-types/vbo.h>

/**
 * Provides VBO for child states.
 * The VBO can be populated with AttributeState's.
 */
class VBOState : public State
{
public:
  static GLuint getDefaultSize();

  VBOState(
      GLuint bufferSize,
      VertexBufferObject::Usage usage);
  VBOState(
      list< ShaderInputState* > &geomNodes,
      GLuint minBufferSize,
      VertexBufferObject::Usage usage);
  VBOState(ref_ptr<VertexBufferObject> vbo);

  void resize(GLuint bufferSize);

  GLboolean add(list< ShaderInputState* > &data, GLboolean allowResizing);
  void remove(ShaderInputState *geom);

  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);

  ref_ptr<VertexBufferObject>& vbo() { return vbo_; };

protected:
  ref_ptr<VertexBufferObject> vbo_;

  struct GeomIteratorData {
    VBOBlockIterator interleavedIt;
    VBOBlockIterator sequentialIt;
  };
  map<ShaderInputState*,GeomIteratorData> geometry_;

  stack<VertexBufferObject*> vbos_;
};

#endif /* VBO_NODE_H_ */
