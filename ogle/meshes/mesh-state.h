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

namespace ogle {
/**
 * \brief A collection of vertices, edges and faces that defines the shape of an object in 3D space.
 *
 * When this State is enabled the actual draw call is done. Make sure to setup shader
 * and server side states before.
 */
class MeshState : public ShaderInputState
{
public:
  /**
   * @param primitive face primitive of this mesh.
   */
  MeshState(GLenum primitive);

  /**
   * @return face primitive of this mesh.
   */
  GLenum primitive() const;
  /**
   * @param primitive face primitive of this mesh.
   */
  void set_primitive(GLenum primitive);

  /**
   * @return Number of vertices of this mesh.
   */
  GLuint numVertices() const;

  /**
   * @return the position attribute.
   */
  const ShaderInputItConst& positions() const;
  /**
   * @return the normal attribute.
   */
  const ShaderInputItConst& normals() const;
  /**
   * @return the color attribute.
   */
  const ShaderInputItConst& colors() const;

  /**
   * Render primitives from array data.
   */
  virtual void draw(GLuint numInstances);
  /**
   * Render primitives from transform feedback array data.
   */
  virtual void drawFeedback(GLuint numInstances);

  // ShaderInputState override
  virtual ShaderInputItConst setInput(const ref_ptr<ShaderInput> &in);
  // State override
  virtual void enable(RenderState*);
  virtual void disable(RenderState *state);

  ////////////////////////////////
  ////////////////////////////////

  // XXX: redundant to TF state ? documentation.

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
  ShaderInputItConst vertices_;
  ShaderInputItConst normals_;
  ShaderInputItConst colors_;

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
 * \brief Adds an index buffer to MeshState.
 */
class IndexedMeshState : public MeshState
{
public:
  IndexedMeshState(GLenum primitive);

  /**
   * Sets the index attribute.
   * @param indices the index attribute.
   * @param maxIndex maximal index in the index array.
   */
  void setIndices(const ref_ptr<VertexAttribute> &indices, GLuint maxIndex);

  /**
   * @return number of indices to vertex data.
   */
  GLuint numIndices() const;
  /**
   * @return the maximal index in the index buffer.
   */
  GLuint maxIndex();
  /**
   * @return indexes to the vertex data of this primitive set.
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

} // end ogle namespace

#endif /* MESH_STATE_H_ */
