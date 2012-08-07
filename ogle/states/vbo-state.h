/*
 * vbo-node.h
 *
 *  Created on: 02.08.2012
 *      Author: daniel
 */

#ifndef VBO_NODE_H_
#define VBO_NODE_H_

#include <ogle/states/state.h>
#include <ogle/states/attribute-state.h>
#include <ogle/gl-types/vbo.h>

/**
 * A node containing a VBO for vertex and index data.
 * GeomNode's are used to populate the VBO.
 */
class VBOState : public State
{
public:
  static GLuint getDefaultSize();

  VBOState(
      GLuint bufferSize=getDefaultSize(),
      VertexBufferObject::Usage usage=
          VertexBufferObject::USAGE_DYNAMIC);
  VBOState(
      list< AttributeState* > &geomNodes,
      GLuint minBufferSize=getDefaultSize(),
      VertexBufferObject::Usage usage=
          VertexBufferObject::USAGE_DYNAMIC);
  VBOState(ref_ptr<VertexBufferObject> &vbo);

  bool add(list< AttributeState* > &data);
  void remove(AttributeState *geom);

  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);

protected:
  ref_ptr<VertexBufferObject> vbo_;

  struct GeomIteratorData {
    VBOBlockIterator interleavedIt;
    VBOBlockIterator sequentialIt;
  };
  map<AttributeState*,GeomIteratorData> geometry_;

  stack<VertexBufferObject*> vbos_;
};

#endif /* VBO_NODE_H_ */
