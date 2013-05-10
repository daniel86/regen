/*
 * fbo-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>

#include "fbo-state.h"
using namespace regen;

FBOState::FBOState(const ref_ptr<FBO> &fbo)
: State(), fbo_(fbo), useMRT_(GL_FALSE)
{
  joinShaderInput(fbo->viewport());
  joinShaderInput(fbo->inverseViewport());
}

void FBOState::setClearDepth()
{
  if(clearDepthCallable_.get()) {
    disjoinStates(clearDepthCallable_);
  }
  clearDepthCallable_ = ref_ptr<ClearDepthState>::alloc();
  joinStates(clearDepthCallable_);

  // make sure clearing is done before draw buffer configuration
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(drawBufferCallable_);
    joinStates(drawBufferCallable_);
  }
}

void FBOState::setClearColor(const ClearColorState::Data &data)
{
  if(clearColorCallable_.get() == NULL) {
    clearColorCallable_ = ref_ptr<ClearColorState>::alloc(fbo_);
    joinStates(clearColorCallable_);
  }
  clearColorCallable_->data.push_back(data);

  // make sure clearing is done before draw buffer configuration
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(drawBufferCallable_);
    joinStates(drawBufferCallable_);
  }
}
void FBOState::setClearColor(const list<ClearColorState::Data> &data)
{
  if(clearColorCallable_.get()) {
    disjoinStates(clearColorCallable_);
  }
  clearColorCallable_ = ref_ptr<ClearColorState>::alloc(fbo_);
  for(list<ClearColorState::Data>::const_iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    clearColorCallable_->data.push_back(*it);
  }
  joinStates(clearColorCallable_);

  // make sure clearing is done before draw buffer configuration
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(drawBufferCallable_);
    joinStates(drawBufferCallable_);
  }
}

void FBOState::addDrawBuffer(GLenum colorAttachment)
{
  if(!useMRT_ && drawBufferCallable_.get()!=NULL) {
    disjoinStates(drawBufferCallable_);
    drawBufferCallable_ = ref_ptr<State>();
  }
  useMRT_ = GL_TRUE;
  if(drawBufferCallable_.get()==NULL) {
    drawBufferCallable_ = ref_ptr<DrawBufferState>::alloc(fbo_);
    joinStates(drawBufferCallable_);
  }
  DrawBufferState *s = (DrawBufferState*) drawBufferCallable_.get();
  s->colorBuffers.buffers_.push_back(colorAttachment);
}

void FBOState::setDrawBufferOntop(const ref_ptr<Texture> &t, GLenum baseAttachment)
{
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(drawBufferCallable_);
  }
  drawBufferCallable_ = ref_ptr<DrawBufferOntop>::alloc(fbo_,t,baseAttachment);
  joinStates(drawBufferCallable_);
  useMRT_ = GL_FALSE;
}
void FBOState::setDrawBufferUpdate(const ref_ptr<Texture> &t, GLenum baseAttachment)
{
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(drawBufferCallable_);
  }
  drawBufferCallable_ = ref_ptr<DrawBufferUpdate>::alloc(fbo_,t,baseAttachment);
  joinStates(drawBufferCallable_);
  useMRT_ = GL_FALSE;
}

void FBOState::enable(RenderState *state)
{
  state->drawFrameBuffer().push(fbo_->id());
  state->viewport().push(fbo_->glViewport());
  State::enable(state);
}

void FBOState::disable(RenderState *state)
{
  State::disable(state);
  state->viewport().pop();
  state->drawFrameBuffer().pop();
}

void FBOState::resize(GLuint width, GLuint height)
{
  fbo_->resize(width, height, fbo_->depth());
}

const ref_ptr<FBO>& FBOState::fbo()
{ return fbo_; }
