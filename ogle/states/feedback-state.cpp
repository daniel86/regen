/*
 * feedback-state.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include "feedback-state.h"

FeedbackState::FeedbackState(
      const GLenum &feedbackPrimitive,
      const ref_ptr<VertexBufferObject> &feedbackBuffer)
: State(),
  feedbackPrimitive_(feedbackPrimitive),
  feedbackBuffer_(feedbackBuffer)
{
}

FeedbackStateInterleaved::FeedbackStateInterleaved(
    const GLenum &feedbackPrimitive,
    const ref_ptr<VertexBufferObject> &feedbackBuffer)
: FeedbackState(feedbackPrimitive,feedbackBuffer)
{
}
void FeedbackStateInterleaved::enable(RenderState *rs)
{
  if(!rs->isTransformFeedbackAcive()) {
    glBindBufferRange(
        GL_TRANSFORM_FEEDBACK_BUFFER,
        0, feedbackBuffer_->id(),
        0, feedbackBuffer_->bufferSize());
  }
  rs->beginTransformFeedback(feedbackPrimitive_);
}
void FeedbackStateInterleaved::disable(RenderState *rs)
{
  rs->endTransformFeedback();
  if(!rs->isTransformFeedbackAcive()) {
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
  }
}

FeedbackStateSeparate::FeedbackStateSeparate(
    const GLenum &feedbackPrimitive,
    const ref_ptr<VertexBufferObject> &feedbackBuffer,
    const list< ref_ptr<VertexAttribute> > &attributes)
: FeedbackState(feedbackPrimitive,feedbackBuffer),
  attributes_(attributes)
{
}
void FeedbackStateSeparate::enable(RenderState *rs)
{
  if(!rs->isTransformFeedbackAcive())
  {
    GLint bufferIndex=0;
    for(list< ref_ptr<VertexAttribute> >::const_iterator
        it=attributes_.begin(); it!=attributes_.end(); ++it)
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
void FeedbackStateSeparate::disable(RenderState *rs)
{
  rs->endTransformFeedback();
  if(!rs->isTransformFeedbackAcive()) {
    for(GLuint bufferIndex=0u; bufferIndex<attributes_.size(); ++bufferIndex)
    {
      glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bufferIndex, 0);
    }
  }
}
