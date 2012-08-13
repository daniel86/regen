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
  AttributeState();

  const list< ref_ptr<VertexAttribute> >& interleavedAttributes();
  const list< ref_ptr<VertexAttribute> >& sequentialAttributes();

  /**
   * vertex attributes.
   */
  list< ref_ptr<VertexAttribute> >* attributesPtr();
  /**
   * vertex attributes.
   */
  const list< ref_ptr<VertexAttribute> >& attributes() const;

  /**
   * Returns true if an attribute with given name was added.
   */
  bool hasAttribute(const string &name) const;

  /**
   * Get attribute with specified name.
   */
  AttributeIteratorConst getAttribute(const string &name) const;

  VertexAttribute* getAttributePtr(const string &name);

  /**
   * Set a vertex attribute.
   * uploadAttributes() must be called before the attributes are
   * uploaded to a VBO.
   */
  virtual AttributeIteratorConst setAttribute(ref_ptr<VertexAttribute> attribute);
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttributefv> attribute);
  AttributeIteratorConst setAttribute(ref_ptr<VertexAttributeuiv> attribute);


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
  list< ref_ptr<VertexAttribute> > attributes_;
  set<string> attributeMap_;

  list< ref_ptr<VertexAttribute> > interleavedAttributes_;
  list< ref_ptr<VertexAttribute> > sequentialAttributes_;

  void removeAttribute( const string &name );
  virtual void removeAttribute(ref_ptr<VertexAttribute> att);
};

#endif /* ATTRIBUTE_STATE_H_ */
