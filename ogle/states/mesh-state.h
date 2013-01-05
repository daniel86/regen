/*
 * mesh-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef MESH_STATE_H_
#define MESH_STATE_H_

#include <ogle/states/shader-input-state.h>
#include <ogle/gl-types/vertex-attribute.h>
#include <ogle/gl-types/vbo.h>

/**
 * State that actually does a draw call.
 */
class MeshState : public ShaderInputState
{
public:
  MeshState(GLenum primitive);

  /**
   * Face primitive of this mesh.
   */
  GLenum primitive() const;
  /**
   * Face primitive of this mesh.
   */
  void set_primitive(GLenum primitive);

  /**
   * Number of vertices of this mesh.
   */
  GLuint numVertices() const;

  /**
   * Get the position attribute.
   */
  const ShaderInputIteratorConst& vertices() const;
  /**
   * Get the normal attribute.
   */
  const ShaderInputIteratorConst& normals() const;
  /**
   * Get the color attribute.
   */
  const ShaderInputIteratorConst& colors() const;

  /**
   * transform feedback attributes.
   */
  const list< ref_ptr<VertexAttribute> >& tfAttributes() const;
  list< ref_ptr<VertexAttribute> >* tfAttributesPtr();

  GLenum transformFeedbackPrimitive() const;
  const ref_ptr<VertexBufferObject>& transformFeedbackBuffer();

  void updateTransformFeedbackBuffer();

  virtual AttributeIteratorConst setTransformFeedbackAttribute(const ref_ptr<ShaderInput> &in);

  GLboolean hasTransformFeedbackAttribute(const string &name) const;
  ref_ptr<VertexAttribute> getTransformFeedbackAttribute(const string &name);
  AttributeIteratorConst getTransformFeedbackAttribute(const string &name) const;

  virtual void draw(GLuint numInstances);
  virtual void drawTransformFeedback(GLuint numInstances);

  virtual ShaderInputIteratorConst setInput(const ref_ptr<ShaderInput> &in);

  virtual void enable(RenderState*);
  virtual void disable(RenderState *state);

protected:
  GLenum primitive_;

  // data buffer vars
  GLuint numVertices_;
  ShaderInputIteratorConst vertices_;
  ShaderInputIteratorConst normals_;
  ShaderInputIteratorConst colors_;

  ref_ptr<VertexBufferObject> tfVBO_;
  list< ref_ptr<VertexAttribute> > tfAttributes_;
  GLenum tfPrimitive_;
  ref_ptr<State> transformFeedbackState_;
  map< string, ref_ptr<ShaderInput> > tfAttributeMap_;

  void removeTransformFeedbackAttribute(const string &name);
  void removeTransformFeedbackAttribute(const ref_ptr<ShaderInput> &att);

  virtual void removeInput(const ref_ptr<ShaderInput> &in);
};

/**
 * Uses IBO for accessing the vertex data.
 */
class IndexedMeshState : public MeshState
{
public:
  IndexedMeshState(GLenum primitive);

  virtual list< ref_ptr<VertexAttribute> > sequentialAttributes();

  void setFaceIndicesui(GLuint *faceIndices, GLuint numCubeFaceIndices, GLuint numCubeFaces);

  void setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex);

  /**
   * Number of indexes to vertex data.
   */
  GLuint numIndices() const;

  /**
   * The maximal index to access in the index buffer.
   */
  GLuint maxIndex();

  /**
   * indexes to the vertex data of this primitive set.
   */
  const ref_ptr<VertexAttribute>& indices() const;

  // override
  virtual void draw(GLuint numInstances);
  virtual void drawTransformFeedback(GLuint numInstances);

  virtual AttributeIteratorConst setTransformFeedbackAttribute(const ref_ptr<ShaderInput> &in);

protected:
  GLuint numIndices_;
  GLuint maxIndex_;
  ref_ptr<VertexAttribute> indices_;
};

class TFMeshState : public State
{
public:
  TFMeshState(ref_ptr<MeshState> attState);
  virtual void enable(RenderState*);
  virtual void disable(RenderState *state);
protected:
  ref_ptr<MeshState> attState_;
};

#endif /* MESH_STATE_H_ */
