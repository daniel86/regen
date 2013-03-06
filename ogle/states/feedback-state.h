/*
 * feedback-state.h
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#ifndef FEEDBACK_STATE_H_
#define FEEDBACK_STATE_H_

#include <ogle/states/state.h>

namespace ogle {

/**
 * Transform feedback baseclass.
 */
class FeedbackState : public State
{
public:
  FeedbackState(
      const GLenum &feedbackPrimitive,
      const ref_ptr<VertexBufferObject> &feedbackBuffer);
protected:
  const GLenum &feedbackPrimitive_;
  const ref_ptr<VertexBufferObject> &feedbackBuffer_;
};

/**
 * Transform feedback, streaming data interleaved to the buffer.
 */
class FeedbackStateInterleaved : public FeedbackState
{
public:
  FeedbackStateInterleaved(
      const GLenum &feedbackPrimitive,
      const ref_ptr<VertexBufferObject> &feedbackBuffer);
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
};

/**
 * Transform feedback, streaming data separate to the buffer.
 */
class FeedbackStateSeparate : public FeedbackState
{
public:
  FeedbackStateSeparate(
      const GLenum &feedbackPrimitive,
      const ref_ptr<VertexBufferObject> &feedbackBuffer,
      const list< ref_ptr<VertexAttribute> > &attributes);
  virtual void enable(RenderState *state);
  virtual void disable(RenderState *state);
protected:
  const list< ref_ptr<VertexAttribute> > &attributes_;
};

} // end ogle namespace

#endif /* FEEDBACK_STATE_H_ */
