/*
 * feedback-state.cpp
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#include "feedback-state.h"

// XXX: use new render state

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
void FeedbackStateInterleaved::enable(RenderState *state)
{
  glBindBufferRange(
      GL_TRANSFORM_FEEDBACK_BUFFER,
      0, feedbackBuffer_->id(),
      0, feedbackBuffer_->bufferSize());
  glBeginTransformFeedback(feedbackPrimitive_);
}
void FeedbackStateInterleaved::disable(RenderState *state)
{
  glEndTransformFeedback();
  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
}

FeedbackStateSeparate::FeedbackStateSeparate(
    const GLenum &feedbackPrimitive,
    const ref_ptr<VertexBufferObject> &feedbackBuffer,
    const list< ref_ptr<VertexAttribute> > &attributes)
: FeedbackState(feedbackPrimitive,feedbackBuffer),
  attributes_(attributes)
{
}
void FeedbackStateSeparate::enable(RenderState *state)
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
  glBeginTransformFeedback(feedbackPrimitive_);
}
void FeedbackStateSeparate::disable(RenderState *state)
{
  glEndTransformFeedback();
  for(GLuint bufferIndex=0u; bufferIndex<attributes_.size(); ++bufferIndex)
  {
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, bufferIndex, 0);
  }
}
