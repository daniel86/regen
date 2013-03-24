/*
 * feedback-state.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include "feedback-state.h"
using namespace regen;

FeedbackState::FeedbackState(GLenum feedbackPrimitive, GLuint feedbackCount)
: State(), feedbackPrimitive_(feedbackPrimitive), feedbackCount_(feedbackCount), dirty_(GL_FALSE)
{
  feedbackBuffer_ = ref_ptr<VertexBufferObject>::manage(
      new VertexBufferObject(VertexBufferObject::USAGE_STREAM, 0));
  feedbackBufferSize_ = 0;
  set_feedbackMode(GL_SEPARATE_ATTRIBS);
  set_feedbackStage(GL_VERTEX_SHADER);
}

void FeedbackState::set_feedbackCount(GLuint count)
{
  feedbackCount_ = count;
  dirty_ = GL_TRUE;
}

void FeedbackState::set_feedbackMode(GLenum mode)
{
  switch(feedbackMode_) {
  case GL_INTERLEAVED_ATTRIBS:
    enable_ = &FeedbackState::enableInterleaved;
    disable_ = &FeedbackState::disableInterleaved;
    break;
  default:
    enable_ = &FeedbackState::enableSeparate;
    disable_ = &FeedbackState::disableSeparate;
    break;
  }
  dirty_ = GL_TRUE;
}
GLenum FeedbackState::feedbackMode() const
{ return feedbackMode_; }

GLenum FeedbackState::feedbackPrimitive() const
{ return feedbackPrimitive_; }

void FeedbackState::set_feedbackStage(GLenum stage)
{ feedbackStage_ = stage; }
GLenum FeedbackState::feedbackStage() const
{ return feedbackStage_; }

void FeedbackState::addFeedback(const ref_ptr<VertexAttribute> &in)
{
  // remove if already added
  if(feedbackAttributeMap_.count(in->name())>0)
  { removeFeedback(in.get()); }

  GLuint feedbackCount = (feedbackCount_==0 ? in->numVertices() : feedbackCount_);
  feedbackCount_ = feedbackCount;

  // create feedback attribute
  ref_ptr<VertexAttribute> feedback = ref_ptr<VertexAttribute>::cast(ShaderInput::create(
      in->name(), in->dataType(), in->valsPerElement()));
  feedback->set_size(feedbackCount * feedback->elementSize());
  feedback->set_numVertices(feedbackCount);
  feedbackAttributes_.push_front(feedback);
  feedbackAttributeMap_[in->name()] = feedbackAttributes_.begin();

  feedbackBufferSize_ += feedback->size();
  dirty_ = GL_TRUE;
}
void FeedbackState::removeFeedback(VertexAttribute *in)
{
  map<string,FeedbackList::iterator>::iterator it = feedbackAttributeMap_.find(in->name());
  if(it == feedbackAttributeMap_.end()) { return; }

  ref_ptr<VertexAttribute> in_ = *(it->second);
  feedbackBufferSize_ -= in_->size();
  dirty_ = GL_TRUE;

  feedbackAttributes_.erase(it->second);
  feedbackAttributeMap_.erase(it);
}
ref_ptr<VertexAttribute> FeedbackState::getFeedback(const string &name)
{
  map<string,FeedbackList::iterator>::iterator it = feedbackAttributeMap_.find(name);
  if(it == feedbackAttributeMap_.end()) { ref_ptr<VertexAttribute>(); }
  return *(it->second);
}
GLboolean FeedbackState::hasFeedback(const string &name) const
{ return feedbackAttributeMap_.count(name)>0; }

void FeedbackState::enable(RenderState *rs)
{
  if(dirty_) {
    // free previously allocated data
    if(feedbackBuffer_->bufferSize()>0) {
      feedbackBuffer_->free(vboIt_);
    }
    // resize vbo
    feedbackBuffer_->resize(feedbackBufferSize_);
    // and allocate space for attributes
    if(feedbackMode_ == GL_INTERLEAVED_ATTRIBS) {
      vboIt_ = feedbackBuffer_->allocateInterleaved(feedbackAttributes_);
    } else {
      vboIt_ = feedbackBuffer_->allocateSequential(feedbackAttributes_);
    }
    dirty_ = GL_FALSE;
  }

  (this->*enable_)(rs);
}
void FeedbackState::disable(RenderState *rs)
{
  (this->*disable_)(rs);
}

void FeedbackState::enableInterleaved(RenderState *rs)
{
  if(!rs->isTransformFeedbackAcive()) {
    glBindBufferRange(
        GL_TRANSFORM_FEEDBACK_BUFFER,
        0, feedbackBuffer_->id(),
        0, feedbackBuffer_->bufferSize());
  }
  rs->beginTransformFeedback(feedbackPrimitive_);
}
void FeedbackState::disableInterleaved(RenderState *rs)
{
  rs->endTransformFeedback();
  if(!rs->isTransformFeedbackAcive()) {
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
  }
}

void FeedbackState::enableSeparate(RenderState *rs)
{
  if(!rs->isTransformFeedbackAcive())
  {
    GLint bufferIndex=0;
    for(FeedbackList::const_iterator
        it=feedbackAttributes_.begin(); it!=feedbackAttributes_.end(); ++it)
    {
      const ref_ptr<VertexAttribute> &att = *it;
      glBindBufferRange(
          GL_TRANSFORM_FEEDBACK_BUFFER,
          bufferIndex++,
          feedbackBuffer_->id(),
          att->offset(),
          att->size());
    }
  }
  rs->beginTransformFeedback(feedbackPrimitive_);
}
void FeedbackState::disableSeparate(RenderState *rs)
{
  rs->endTransformFeedback();
  if(!rs->isTransformFeedbackAcive()) {
    for(GLuint bufferIndex=0u; bufferIndex<feedbackAttributes_.size(); ++bufferIndex)
    {
      glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bufferIndex, 0);
    }
  }
}

void FeedbackState::draw(GLuint numInstances)
{
  glDrawArraysInstancedEXT(
      feedbackPrimitive_,
      0,
      feedbackCount_,
      numInstances);
}

const list< ref_ptr<VertexAttribute> >& FeedbackState::feedbackAttributes() const
{ return feedbackAttributes_; }
const ref_ptr<VertexBufferObject>& FeedbackState::feedbackBuffer() const
{ return feedbackBuffer_; }
