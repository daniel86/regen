/*
 * mesh-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef MESH_STATE_H_
#define MESH_STATE_H_

#include <ogle/states/attribute-state.h>
#include <ogle/gl-types/vertex-attribute.h>

class MeshState : public AttributeState
{
public:
  MeshState(GLenum primitive);

  /**
   * Geometric primitive of this mesh.
   */
  GLenum primitive() const;
  void set_primitive(GLenum primitive);

  /**
   * Number of vertices of this mesh.
   */
  GLuint numVertices() const;

  /**
   * Get the position attribute.
   */
  const AttributeIteratorConst& vertices() const;
  /**
   * Get the normal attribute.
   */
  const AttributeIteratorConst& normals() const;
  /**
   * Get the color attribute.
   */
  const AttributeIteratorConst& colors() const;

  /**
   * transform feedback attributes.
   */
  const list< ref_ptr<VertexAttribute> >& tfAttributes() const;
  GLenum transformFeedbackPrimitive() const;
  ref_ptr<VertexBufferObject>& transformFeedbackBuffer();

  void updateTransformFeedbackBuffer();

  AttributeIteratorConst setTransformFeedbackAttribute(ref_ptr<VertexAttribute> attribute);

  bool hasTransformFeedbackAttribute(const string &name) const;
  ref_ptr<VertexAttribute> getTransformFeedbackAttribute(const string &name);
  AttributeIteratorConst getTransformFeedbackAttribute(const string &name) const;
  VertexAttribute* getTransformFeedbackAttributePtr(const string &name);

  list< ref_ptr<VertexAttribute> >* tfAttributesPtr();

  virtual void draw(GLuint numInstances);
  virtual void drawTransformFeedback(GLuint numInstances);

  virtual AttributeIteratorConst setAttribute(
      ref_ptr<VertexAttribute> attribute);

  virtual void enable(RenderState*);
  virtual void configureShader(ShaderConfiguration*);

  virtual string name();

protected:
  GLenum primitive_;

  // data buffer vars
  GLuint numVertices_;
  AttributeIteratorConst vertices_;
  AttributeIteratorConst normals_;
  AttributeIteratorConst colors_;

  ref_ptr<VertexBufferObject> tfVBO_;
  list< ref_ptr<VertexAttribute> > tfAttributes_;
  GLenum transformFeedbackPrimitive_;
  ref_ptr<State> transformFeedbackState_;
  map< string, ref_ptr<VertexAttribute> > tfAttributeMap_;

  void removeTransformFeedbackAttribute(const string &name);
  void removeTransformFeedbackAttribute(ref_ptr<VertexAttribute> att);

  virtual void removeAttribute(ref_ptr<VertexAttribute> att);
};

/**
 * Uses IBO for accessing the vertex data.
 */
class IndexedMeshState : public MeshState
{
public:
  IndexedMeshState(GLenum primitive);

  void setFaceIndicesui(
      GLuint *faceIndices,
      GLuint numCubeFaceIndices,
      GLuint numCubeFaces);

  void setIndices(
      ref_ptr< VertexAttribute > indices,
      GLuint maxIndex);

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
  ref_ptr<VertexAttribute>& indices();

  // override
  virtual void draw(GLuint numInstances);

  virtual string name();

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

  virtual string name();
protected:
  ref_ptr<MeshState> attState_;
};

#endif /* MESH_STATE_H_ */
