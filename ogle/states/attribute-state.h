/*
 * attribute-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef ATTRIBUTE_STATE_H_
#define ATTRIBUTE_STATE_H_

#include <ogle/states/state.h>
#include <ogle/gl-types/vertex-attribute.h>

typedef list< ref_ptr<VertexAttribute> >::const_iterator
    AttributeIteratorConst;

/**
 * A face of this primitive set.
 */
typedef struct {
  ref_ptr< vector<GLuint> > indexes_;
}MeshFace;

class AttributeState : public State
{
public:
  /**
   * Default constructor.
   */
  AttributeState(GLenum primitive);

  /**
   * Geometric primitive of this mesh.
   */
  GLenum primitive() const;
  void set_primitive(GLenum primitive);

  const list< ref_ptr<VertexAttribute> >& interleavedAttributes();
  const list< ref_ptr<VertexAttribute> >& sequentialAttributes();

  /**
   * Number of vertices of this mesh.
   */
  GLuint numVertices() const;

  /**
   * Number of instances of this mesh.
   */
  GLuint numInstances() const;


  void setFaces(vector<MeshFace> &faces, GLuint numFaceVertices);

  /**
   * The faces of this primitive set.
   */
  const vector<MeshFace>& faces() const;

  /**
   * Index data is used to access the index buffer.
   */
  void set_vboOffset(
      GLuint minIndex,
      GLuint maxIndex,
      GLuint offset=0);
  /**
   * Number of indexes to vertex data.
   */
  GLuint numIndices() const;

  /**
   * The maximal index to access in the index buffer.
   */
  GLuint maxIndex();

  /**
   * vertex attributes.
   */
  list< ref_ptr<VertexAttribute> >* attributesPtr();
  list< ref_ptr<VertexAttribute> >* tfAttributesPtr();
  /**
   * vertex attributes.
   */
  const list< ref_ptr<VertexAttribute> >& attributes() const;
  /**
   * transform feedback attributes.
   */
  const list< ref_ptr<VertexAttribute> >& tfAttributes() const;
  /**
   * indexes to the vertex data of this primitive set.
   */
  ref_ptr<VertexAttribute>& indices();

  /**
   * Returns true if an attribute with given name was added.
   */
  bool hasAttribute(const string &name) const;
  bool hasTransformFeedbackAttribute(const string &name) const;

  /**
   * Get attribute with specified name.
   */
  AttributeIteratorConst getAttribute(const string &name) const;
  AttributeIteratorConst getTransformFeedbackAttribute(const string &name) const;

  VertexAttribute* getAttributePtr(const string &name);
  VertexAttribute* getTransformFeedbackAttributePtr(const string &name);

  /**
   * Set a vertex attribute.
   * uploadAttributes() must be called before the attributes are
   * uploaded to a VBO.
   */
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttribute> &attribute);
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttributefv> &attribute);
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttributeuiv> &attribute);
  AttributeIteratorConst setTransformFeedbackAttribute(ref_ptr<VertexAttribute> &attribute);

  void set_indices(
      ref_ptr< VertexAttribute > &indices,
      GLuint maxIndex);

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
   * Offset for accessing VBO.
   */
  void* vboOffset();

  /**
   * Just call the GL draw function to render this primitive.
   */
  virtual void draw();
  /**
   * Calls glDrawArrays with last recorded
   * transform feedback data.
   */
  void drawTransformFeedback();

  GLenum transformFeedbackPrimitive() const;
  /**
   * Adds a transform feedback attribute.
   * Note: stride and offset set in updateShader() if the attribute was successfully found.
   * @return false if att was overwritten because an attribute with same name added before,
   *    then att will point to the previously added attribute after this call
   */
  bool addTransformFeedbackAttribute(ref_ptr<VertexAttribute> &att);
  ref_ptr<VertexAttribute> getTransformFeedbackAttribute(const string &name);

  virtual void enable(RenderState*);
  virtual void configureShader(ShaderConfiguration*);

protected:
  GLenum primitive_;

  // index buffer vars
  GLuint numIndices_;
  GLuint maxIndex_;
  ref_ptr<VertexAttribute> indices_;
  ref_ptr< vector<GLuint> > indexData_;
  vector<MeshFace> faces_;

  // data buffer vars
  GLuint numVertices_;
  GLuint numInstances_;
  GLuint minVBOIndex_;
  GLuint maxVBOIndex_;
  void* vboOffset_;
  AttributeIteratorConst vertices_;
  AttributeIteratorConst normals_;
  AttributeIteratorConst colors_;

  list< ref_ptr<VertexAttribute> > tfAttributes_;
  list< ref_ptr<VertexAttribute> > attributes_;
  set<string> attributeMap_;

  GLenum transformFeedbackPrimitive_;
  ref_ptr<State> transformFeedbackState_;
  map< string, ref_ptr<VertexAttribute> > tfAttributeMap_;

  list< ref_ptr<VertexAttribute> > interleavedAttributes_;
  list< ref_ptr<VertexAttribute> > sequentialAttributes_;

  // draw function
  typedef void (AttributeState::*DrawMeshFunc)();
  DrawMeshFunc drawMesh_;
  void drawMeshInstanced();
  void drawMeshUninstanced();

  void removeAttribute( const string &name );
  void removeTransformFeedbackAttribute(ref_ptr<VertexAttribute> &att);

  void removeAttribute( ref_ptr<VertexAttribute> &att);
  void removeTransformFeedbackAttribute(const string &name);
};

#endif /* ATTRIBUTE_STATE_H_ */
