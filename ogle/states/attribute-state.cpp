/*
 * attribute-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "attribute-state.h"

class TransformFeedbackState : public State
{
public:
  TransformFeedbackState(
      const list< ref_ptr<VertexAttribute> > &atts,
      const GLenum &transformFeedbackPrimitive)
  : State(),
    atts_(atts),
    transformFeedbackPrimitive_(transformFeedbackPrimitive)
  {
  }
  virtual void enable(RenderState *state)
  {
    if(state->vbos.isEmpty()) {
      WARN_LOG("no VBO parent set.");
      return;
    }
    VertexBufferObject *vbo = state->vbos.top();
    GLint bufferIndex=0;
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        it=atts_.begin(); it!=atts_.end(); ++it)
    {
      const ref_ptr<VertexAttribute> &att = *it;
      glBindBufferRange(
          GL_TRANSFORM_FEEDBACK_BUFFER,
          bufferIndex++,
          vbo->id(),
          att->offset(),
          att->size());
    }
    glBeginTransformFeedback(transformFeedbackPrimitive_);
    State::enable(state);
  }
  virtual void disable(RenderState *state)
  {
    State::disable(state);
    glEndTransformFeedback();
    for(int bufferIndex=0; bufferIndex<atts_.size(); ++bufferIndex)
    {
      glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bufferIndex, 0);
    }
  }
protected:
  const list< ref_ptr<VertexAttribute> > &atts_;
  const GLenum transformFeedbackPrimitive_;
};

AttributeState::AttributeState(GLenum primitive)
: State(),
  primitive_(primitive),
  numIndices_(0),
  numVertices_(0),
  numInstances_(1),
  drawMesh_( &AttributeState::drawUninstanced )
{
  // TODO: AttributeState: use VAO
  vertices_ = attributes_.end();
  normals_ = attributes_.end();
  colors_ = attributes_.end();

  set_primitive(primitive);

  transformFeedbackState_ = ref_ptr<State>::manage(
      new TransformFeedbackState(tfAttributes_, transformFeedbackPrimitive_));
}

GLenum AttributeState::primitive() const
{
  return primitive_;
}
GLenum AttributeState::transformFeedbackPrimitive() const
{
  return transformFeedbackPrimitive_;
}
void AttributeState::set_primitive(GLenum primitive)
{
  primitive_ = primitive;
  switch(primitive_) {
  case GL_PATCHES:
    transformFeedbackPrimitive_ = GL_TRIANGLES;
    break;
  case GL_POINTS:
    transformFeedbackPrimitive_ = GL_POINTS;
    break;
  case GL_LINES:
  case GL_LINE_LOOP:
  case GL_LINE_STRIP:
  case GL_LINES_ADJACENCY:
  case GL_LINE_STRIP_ADJACENCY:
    transformFeedbackPrimitive_ = GL_LINES;
    break;
  case GL_TRIANGLES:
  case GL_TRIANGLE_STRIP:
  case GL_TRIANGLE_FAN:
  case GL_TRIANGLES_ADJACENCY:
  case GL_TRIANGLE_STRIP_ADJACENCY:
    transformFeedbackPrimitive_ = GL_TRIANGLES;
    break;
  default:
    transformFeedbackPrimitive_ = GL_TRIANGLES;
    break;
  }
}

GLuint AttributeState::numVertices() const
{
  return numVertices_;
}

GLuint AttributeState::numInstances() const
{
  return numInstances_;
}

GLuint AttributeState::numIndices() const
{
  return numIndices_;
}

GLuint AttributeState::maxIndex()
{
  return maxIndex_;
}

const AttributeIteratorConst& AttributeState::vertices() const
{
  return vertices_;
}
const AttributeIteratorConst& AttributeState::normals() const
{
  return normals_;
}
const AttributeIteratorConst& AttributeState::colors() const
{
  return colors_;
}

const vector<MeshFace>& AttributeState::faces() const
{
  return faces_;
}

ref_ptr<VertexAttribute>& AttributeState::indices()
{
  return indices_;
}

AttributeIteratorConst AttributeState::getAttribute(const string &name) const
{
  AttributeIteratorConst it;
  for(it = attributes_.begin(); it != attributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it;
  }
  return it;
}
AttributeIteratorConst AttributeState::getTransformFeedbackAttribute(const string &name) const
{
  AttributeIteratorConst it;
  for(it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it;
  }
  return it;
}
VertexAttribute* AttributeState::getAttributePtr(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = attributes_.begin(); it != attributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it->get();
  }
  return NULL;
}
VertexAttribute* AttributeState::getTransformFeedbackAttributePtr(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it) {
    if(name.compare((*it)->name()) == 0) return it->get();
  }
  return NULL;
}

bool AttributeState::hasAttribute(const string &name) const
{
  return attributeMap_.count(name)>0;
}
bool AttributeState::hasTransformFeedbackAttribute(const string &name) const
{
  return tfAttributeMap_.count(name)>0;
}

list< ref_ptr<VertexAttribute> >* AttributeState::attributesPtr()
{
  return &attributes_;
}
const list< ref_ptr<VertexAttribute> >& AttributeState::attributes() const
{
  return attributes_;
}

list< ref_ptr<VertexAttribute> >* AttributeState::tfAttributesPtr()
{
  return &tfAttributes_;
}
const list< ref_ptr<VertexAttribute> >& AttributeState::tfAttributes() const
{
  return tfAttributes_;
}

const list< ref_ptr<VertexAttribute> >& AttributeState::interleavedAttributes()
{
  return interleavedAttributes_;
}
const list< ref_ptr<VertexAttribute> >& AttributeState::sequentialAttributes()
{
  return sequentialAttributes_;
}

/////////////

void AttributeState::draw()
{
  (this->*drawMesh_)();
}

void AttributeState::drawInstanced()
{
  glDrawElementsInstanced(
      primitive_,
      numIndices_,
      indices_->dataType(),
      BUFFER_OFFSET(attributes_.front()->offset()),
      numInstances_);
}

void AttributeState::drawUninstanced()
{
  glDrawElements(
      primitive_,
      numIndices_,
      indices_->dataType(),
      BUFFER_OFFSET(attributes_.front()->offset()));
}

void AttributeState::drawTransformFeedback()
{
  glDrawArrays(
      transformFeedbackPrimitive_,
      tfAttributes_.front()->offset(),
      numIndices_ * numInstances_);
}

/////////////

void AttributeState::set_indices(
    ref_ptr< VertexAttribute > &indices,
    GLuint maxIndex)
{
  if(indices_.get()) {
    for(list< ref_ptr<VertexAttribute> >::iterator
        it=sequentialAttributes_.begin(); it!=sequentialAttributes_.end(); ++it)
    {
      if(it->get() == indices_.get()) {
        sequentialAttributes_.erase(it);
        break;
      }
    }
  }
  indices_ = indices;
  numIndices_ = indices_->data->size()/indices_->dataTypeBytes();
  maxIndex_ = maxIndex;
  sequentialAttributes_.push_back(indices_);
}

void AttributeState::setFaces(vector<MeshFace> &faces, GLuint numFaceVertices)
{
  // set the new faces
  faces_ = faces;

  GLuint numIndices = 0;
  GLuint maxIndex = 0;

  for(vector<MeshFace>::iterator
      it = faces_.begin(); it != faces_.end(); ++it)
  {
    numIndices += it->indexes_->size();
    for(GLuint i=0; i<it->indexes_->size(); ++i)
    {
      GLuint j = it->indexes_->data()[i];
      if(j>maxIndex) maxIndex=j;
    }
  }

  byte* indices = new byte[numIndices*indices_->dataTypeBytes()];
  GLuint *indicesPtr = (GLuint*)indices;

  GLuint j = 0;
  for(vector<MeshFace>::iterator
      it = faces_.begin(); it != faces_.end(); ++it)
  {
    for(GLuint i=0; i<it->indexes_->size(); ++i)
    {
      indicesPtr[j++] = it->indexes_->data()[i];
    }
  }

  ref_ptr<VertexAttribute> indicesAtt = ref_ptr<VertexAttribute>::manage(
      new VertexAttributeUint("i", 1));
  indicesAtt->setVertexData(numIndices, indices);
  set_indices(indicesAtt, maxIndex);
}

AttributeIteratorConst AttributeState::setAttribute(
    ref_ptr<VertexAttributeuiv> &attribute)
{
  ref_ptr<VertexAttribute> att = attribute;
  setAttribute(att);
}
AttributeIteratorConst AttributeState::setAttribute(
    ref_ptr<VertexAttributefv> &attribute)
{
  ref_ptr<VertexAttribute> att = attribute;
  setAttribute(att);
}
AttributeIteratorConst AttributeState::setAttribute(
    ref_ptr<VertexAttribute> &attribute)
{
  numVertices_ = attribute->numVertices();
  if(attribute->divisor() > 0) {
    numInstances_ = attribute->numInstances();
    drawMesh_ = &AttributeState::drawInstanced;
  } else {
    numInstances_ = 1;
    drawMesh_ = &AttributeState::drawUninstanced;
  }

  if(attributeMap_.count(attribute->name())>0) {
    removeAttribute(attribute->name());
  } else { // insert into map of known attributes
    attributeMap_.insert(attribute->name());
  }

  attributes_.push_back(attribute);
  interleavedAttributes_.push_back(attribute);
  AttributeIteratorConst last = attributes_.end();
  --last;

  if(attribute->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = last;
  } else if(attribute->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = last;
  } else if(attribute->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = last;
  }

  return last;
}

void AttributeState::removeAttribute(ref_ptr<VertexAttribute> &att)
{
  attributeMap_.erase(att->name());

  if(att->name().compare( ATTRIBUTE_NAME_POS ) == 0) {
    vertices_ = attributes_.end();
  } else if(att->name().compare( ATTRIBUTE_NAME_NOR ) == 0) {
    normals_ = attributes_.end();
  } else if(att->name().compare( ATTRIBUTE_NAME_COL0 ) == 0) {
    colors_ = attributes_.end();
  }

  removeAttribute(att->name());
}

void AttributeState::removeAttribute(const string &name)
{
  for(list< ref_ptr<VertexAttribute> >::iterator it = attributes_.begin();
      it != attributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      attributes_.erase(it);
      break;
    }
  }
  for(list< ref_ptr<VertexAttribute> >::iterator it = interleavedAttributes_.begin();
      it != interleavedAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      interleavedAttributes_.erase(it);
      break;
    }
  }
}

/////////////

AttributeIteratorConst AttributeState::setTransformFeedbackAttribute(
    ref_ptr<VertexAttribute> &attribute)
{
  if(tfAttributeMap_.count(attribute->name())>0) {
    removeTransformFeedbackAttribute(attribute->name());
  } else { // insert into map of known attributes
    attributeMap_.insert(attribute->name());
  }

  tfAttributes_.push_back(attribute);
  sequentialAttributes_.push_back(attribute);
  AttributeIteratorConst last = tfAttributes_.end();
  --last;

  if(tfAttributes_.size()==1) {
    joinStates(transformFeedbackState_);
  }

  return last;
}

void AttributeState::removeTransformFeedbackAttribute(ref_ptr<VertexAttribute> &att)
{
  removeTransformFeedbackAttribute(att->name());
}

void AttributeState::removeTransformFeedbackAttribute(const string &name)
{
  tfAttributeMap_.erase(name);
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = tfAttributes_.begin(); it != tfAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      tfAttributes_.erase(it);
      return;
    }
  }
  for(list< ref_ptr<VertexAttribute> >::iterator
      it = sequentialAttributes_.begin(); it != sequentialAttributes_.end(); ++it)
  {
    if(name.compare((*it)->name()) == 0) {
      sequentialAttributes_.erase(it);
      return;
    }
  }

  if(tfAttributes_.size()==0) {
    disjoinStates(transformFeedbackState_);
  }
}

void AttributeState::enable(RenderState *state)
{
  State::enable(state);
  if(!state->shaders.isEmpty()) {
    // if a shader is enabled by a parent node,
    // then try to enable the vbo attributes on the shader.
    Shader *shader = state->shaders.top();
    for(list< ref_ptr<VertexAttribute> >::iterator
        it = attributes_.begin(); it != attributes_.end(); ++it)
    {
      shader->applyAttribute(it->get());
    }
  }
  draw();
}

void AttributeState::configureShader(ShaderConfiguration *shaderCfg)
{
  State::configureShader(shaderCfg);
  shaderCfg->setTransformFeedbackAttributes(tfAttributes_);
  shaderCfg->setAttributes(attributes_);
}

////////////

TFAttributeState::TFAttributeState(
    ref_ptr<AttributeState> &attState)
: attState_(attState)
{

}
void TFAttributeState::enable(RenderState *state)
{
  State::enable(state);
  if(!state->shaders.isEmpty()) {
    // if a shader is enabled by a parent node,
    // then try to enable the vbo attributes on the shader.
    Shader *shader = state->shaders.top();
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        it=attState_->tfAttributes().begin(); it!=attState_->tfAttributes().end(); ++it)
    {
      shader->applyAttribute(it->get());
    }
  }
  attState_->drawTransformFeedback();
}
