/*
 * vbo-node.cpp
 *
 *  Created on: 02.08.2012
 *      Author: daniel
 */

#include "vbo-state.h"

#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>

#define DEBUG_VBO

static void getAttributeSizes(
    const list< ShaderInputState* > &data,
    list<GLuint> &sizesRet,
    GLuint &sizeSumRet)
{
  // check if we have enough space in the vbo
  for(list< ShaderInputState* >::const_iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    ShaderInputState *att = *it;

    const list< ref_ptr<VertexAttribute> > &sequential = att->sequentialAttributes();
    if(sequential.size()>0) {
      GLuint size = VertexBufferObject::attributeStructSize(sequential);
      sizesRet.push_back(size);
      sizeSumRet += size;
    }

    const list< ref_ptr<VertexAttribute> > &interleaved = att->interleavedAttributes();
    if(interleaved.size()>0) {
      GLuint size = VertexBufferObject::attributeStructSize(interleaved);
      sizesRet.push_back(size);
      sizeSumRet += size;
    }
  }
}

VBOState::VBOState(const ref_ptr<VertexBufferObject> &vbo)
: State(),
  vbo_(vbo)
{
}

VBOState::VBOState(GLuint bufferSize, VertexBufferObject::Usage usage)
: State()
{
  vbo_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(usage, bufferSize));
}

VBOState::VBOState(list< ShaderInputState* > &geomNodes, GLuint minBufferSize, VertexBufferObject::Usage usage)
: State()
{
  list<GLuint> sizes; GLuint sizeSum;
  getAttributeSizes(geomNodes, sizes, sizeSum);

  vbo_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(usage, max(minBufferSize, sizeSum)));
  add(geomNodes, GL_TRUE);
}

const ref_ptr<VertexBufferObject>& VBOState::vbo() const
{
  return vbo_;
}

void VBOState::resize(GLuint bufferSize)
{
  vbo_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(vbo_->usage(), bufferSize));

  for(map<ShaderInputState*,GeomIteratorData>::iterator
      it=geometry_.begin(); it!=geometry_.end(); ++it)
  {
    ShaderInputState *geomData = it->first;
    GeomIteratorData itData = it->second;
    itData.interleavedIt = vbo_->allocateInterleaved(
        geomData->interleavedAttributes());
    itData.sequentialIt = vbo_->allocateSequential(
        geomData->sequentialAttributes());
  }
}

GLboolean VBOState::add(list< ShaderInputState* > &data, GLboolean allowResizing)
{
  // remove previously added attributes if there are any
  for(list< ShaderInputState* >::iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    remove(*it);
  }

  list<GLuint> sizes; GLuint sizeSum;
  getAttributeSizes(data, sizes, sizeSum);
  if(!vbo_->canAllocate(sizes, sizeSum))
  {
    if(allowResizing) {
      resize(vbo_->bufferSize() + sizeSum);
    } else {
      return GL_FALSE;
    }
  }

  // add geometry data to vbo
  for(list< ShaderInputState* >::iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    ShaderInputState *geomData = *it;
    map<ShaderInputState*,GeomIteratorData>::iterator
        geomIt = geometry_.find(geomData);
    if(geomIt==geometry_.end()) {
      GeomIteratorData itData;
      itData.interleavedIt = vbo_->allocateInterleaved(
          geomData->interleavedAttributes());
#ifdef DEBUG_VBO
      if(!geomData->interleavedAttributes().empty()) {
        DEBUG_LOG("allocated interleaved(" << (*itData.interleavedIt)->start << ", " << (*itData.interleavedIt)->end << ")" );
      }
#endif

      itData.sequentialIt = vbo_->allocateSequential(
          geomData->sequentialAttributes());
#ifdef DEBUG_VBO
      if(!geomData->sequentialAttributes().empty()) {
        DEBUG_LOG("allocated sequential(" << (*itData.sequentialIt)->start << ", " << (*itData.sequentialIt)->end << ")" );
      }
#endif

      geometry_[geomData] = itData;
    }
  }
  return GL_TRUE;
}

void VBOState::remove(ShaderInputState *geom)
{
  map<ShaderInputState*,GeomIteratorData>::iterator needle = geometry_.find(geom);
  if(needle!=geometry_.end()) {
    // erase from vbo
#ifdef DEBUG_VBO
    if(!geom->interleavedAttributes().empty()) {
      DEBUG_LOG("free interleaved(" << (*needle->second.interleavedIt)->start << ", " <<
          (*needle->second.interleavedIt)->end << ")" );
    }
#endif
    vbo_->free(needle->second.interleavedIt);

#ifdef DEBUG_VBO
    if(!geom->sequentialAttributes().empty()) {
      DEBUG_LOG("free sequential(" << (*needle->second.sequentialIt)->start << ", " <<
          (*needle->second.sequentialIt)->end << ")" );
    }
#endif
    vbo_->free(needle->second.sequentialIt);

    geometry_.erase(needle);
  }
}

void VBOState::enable(RenderState *state)
{
  state->pushVBO(vbo_.get());
  State::enable(state);
}

void VBOState::disable(RenderState *state)
{
  State::disable(state);
  state->popVBO();
}
