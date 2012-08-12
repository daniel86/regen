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
 * Provides vertex attributes.
 */
class AttributeState : public State
{
public:
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
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttribute> attribute);
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttributefv> attribute);
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttributeuiv> attribute);
  AttributeIteratorConst setTransformFeedbackAttribute(ref_ptr<VertexAttribute> attribute);

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

  virtual void draw(GLuint numInstances);
  void drawTransformFeedback(GLuint numInstances);

  GLenum transformFeedbackPrimitive() const;
  ref_ptr<VertexAttribute> getTransformFeedbackAttribute(const string &name);

  virtual void enable(RenderState*);
  virtual void configureShader(ShaderConfiguration*);

  /**
   * Is there any attribute not associated to a VBO ?
   */
  virtual GLboolean isBufferSet();
  /**
   * Set the buffer object associated to the attributes.
   * buffer=0 is considered to be unhandled.
   */
  virtual void setBuffer(GLuint buffer=0);

  virtual string name();

protected:
  GLenum primitive_;

  // data buffer vars
  GLuint numVertices_;
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

  void removeAttribute( const string &name );
  void removeTransformFeedbackAttribute(ref_ptr<VertexAttribute> att);

  void removeAttribute(ref_ptr<VertexAttribute> att);
  void removeTransformFeedbackAttribute(const string &name);
};

/**
 * Uses IBO for accessing the vertex data.
 */
class IndexedAttributeState : public AttributeState
{
public:
  IndexedAttributeState(GLenum primitive);

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
  virtual GLboolean isBufferSet();
  virtual void setBuffer(GLuint buffer=0);

  virtual string name();

protected:
  GLuint numIndices_;
  GLuint maxIndex_;
  ref_ptr<VertexAttribute> indices_;
};

class TFAttributeState : public State
{
public:
  TFAttributeState(ref_ptr<AttributeState> attState);
  virtual void enable(RenderState*);

  virtual string name();
protected:
  ref_ptr<AttributeState> attState_;
};

#endif /* ATTRIBUTE_STATE_H_ */
