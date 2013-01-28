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
  const ShaderInputIteratorConst& positions() const;
  /**
   * Get the normal attribute.
   */
  const ShaderInputIteratorConst& normals() const;
  /**
   * Get the color attribute.
   */
  const ShaderInputIteratorConst& colors() const;

  /**
   * GL draw call.
   */
  virtual void draw(GLuint numInstances);
  /**
   * GL draw call for transform feedback record.
   */
  virtual void drawFeedback(GLuint numInstances);

  // ShaderInputState override
  virtual ShaderInputIteratorConst setInput(const ref_ptr<ShaderInput> &in);
  // State override
  virtual void enable(RenderState*);
  virtual void disable(RenderState *state);

  ////////////////////////////////
  ////// Transform Feedback //////
  ////////////////////////////////

  void set_feedbackPrimitive(GLenum primitive);
  GLenum feedbackPrimitive() const;

  void set_feedbackMode(GLenum mode);
  GLenum feedbackMode() const;

  void set_feedbackStage(GLenum stage);
  GLenum feedbackStage() const;

  const ref_ptr<VertexBufferObject>& feedbackBuffer();
  void createFeedbackBuffer();

  virtual AttributeIteratorConst setFeedbackAttribute(
      const string &attributeName, GLenum dataType, GLuint valsPerElement);
  AttributeIteratorConst setFeedbackAttribute(const ref_ptr<VertexAttribute> &in);

  const list< ref_ptr<VertexAttribute> >& feedbackAttributes() const;
  GLboolean hasFeedbackAttribute(const string &name) const;
  ref_ptr<VertexAttribute> getFeedbackAttribute(const string &name);
  AttributeIteratorConst getFeedbackAttribute(const string &name) const;

protected:
  GLenum primitive_;

  // data buffer vars
  GLuint numVertices_;
  ShaderInputIteratorConst vertices_;
  ShaderInputIteratorConst normals_;
  ShaderInputIteratorConst colors_;

  GLenum feedbackPrimitive_;
  GLenum feedbackMode_;
  GLenum feedbackStage_;
  ref_ptr<State> feedbackState_;
  ref_ptr<VertexBufferObject> feedbackBuffer_;
  list< ref_ptr<VertexAttribute> > feedbackAttributes_;
  map< string, ref_ptr<VertexAttribute> > feedbackAttributeMap_;

  void removeFeedbackAttribute(const string &name);

  virtual void removeInput(const ref_ptr<ShaderInput> &in);
};

/**
 * Uses IBO for accessing the vertex data.
 */
class IndexedMeshState : public MeshState
{
public:
  IndexedMeshState(GLenum primitive);

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
  virtual void drawFeedback(GLuint numInstances);
  virtual AttributeIteratorConst setFeedbackAttribute(
      const string &attributeName, GLenum dataType, GLuint valsPerElement);

protected:
  GLuint numIndices_;
  GLuint maxIndex_;
  ref_ptr<VertexAttribute> indices_;
};

class FeedbackMeshState : public State
{
public:
  FeedbackMeshState(const ref_ptr<MeshState> &mesh);
  // override
  virtual void enable(RenderState*);
  virtual void disable(RenderState *state);
protected:
  ref_ptr<MeshState> mesh_;
};

#endif /* MESH_STATE_H_ */
