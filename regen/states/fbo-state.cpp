/*
 * fbo-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include <regen/utility/string-util.h>
#include <regen/states/atomic-states.h>

#include "fbo-state.h"
using namespace ogle;

FBOState::FBOState(const ref_ptr<FrameBufferObject> &fbo)
: State(), fbo_(fbo), useMRT_(GL_FALSE)
{
  joinShaderInput(ref_ptr<ShaderInput>::cast(fbo->viewport()));
  joinShaderInput(ref_ptr<ShaderInput>::cast(fbo->inverseViewport()));
}

void FBOState::setClearDepth()
{
  if(clearDepthCallable_.get()) {
    disjoinStates(ref_ptr<State>::cast(clearDepthCallable_));
  }
  clearDepthCallable_ = ref_ptr<ClearDepthState>::manage(new ClearDepthState);
  joinStates(ref_ptr<State>::cast(clearDepthCallable_));

  // make sure clearing is done before draw buffer configuration
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
    joinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
}

void FBOState::setClearColor(const ClearColorState::Data &data)
{
  if(clearColorCallable_.get() == NULL) {
    clearColorCallable_ = ref_ptr<ClearColorState>::manage(new ClearColorState);
    joinStates(ref_ptr<State>::cast(clearColorCallable_));
  }
  clearColorCallable_->data.push_back(data);

  // make sure clearing is done before draw buffer configuration
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
    joinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
}
void FBOState::setClearColor(const list<ClearColorState::Data> &data)
{
  if(clearColorCallable_.get()) {
    disjoinStates(ref_ptr<State>::cast(clearColorCallable_));
  }
  clearColorCallable_ = ref_ptr<ClearColorState>::manage(new ClearColorState);
  for(list<ClearColorState::Data>::const_iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    clearColorCallable_->data.push_back(*it);
  }
  joinStates(ref_ptr<State>::cast(clearColorCallable_));

  // make sure clearing is done before draw buffer configuration
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
    joinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
}

void FBOState::addDrawBuffer(GLenum colorAttachment)
{
  if(!useMRT_ && drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
    drawBufferCallable_ = ref_ptr<State>();
  }
  useMRT_ = GL_TRUE;
  if(drawBufferCallable_.get()==NULL) {
    drawBufferCallable_ = ref_ptr<State>::manage(new DrawBufferState);
    joinStates(drawBufferCallable_);
  }
  DrawBufferState *s = (DrawBufferState*) drawBufferCallable_.get();
  s->colorBuffers.push_back(colorAttachment);
}

void FBOState::setDrawBufferOntop(const ref_ptr<Texture> &t, GLenum baseAttachment)
{
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
  drawBufferCallable_ = ref_ptr<State>::manage(new DrawBufferOntop(t,baseAttachment));
  joinStates(drawBufferCallable_);
  useMRT_ = GL_FALSE;
}
void FBOState::setDrawBufferUpdate(const ref_ptr<Texture> &t, GLenum baseAttachment)
{
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
  drawBufferCallable_ = ref_ptr<State>::manage(new DrawBufferUpdate(t,baseAttachment));
  joinStates(drawBufferCallable_);
  useMRT_ = GL_FALSE;
}

void FBOState::enable(RenderState *state)
{
  state->fbo().push(fbo_.get());
  State::enable(state);
}

void FBOState::disable(RenderState *state)
{
  State::disable(state);
  state->fbo().pop();
}

void FBOState::resize(GLuint width, GLuint height)
{
  fbo_->resize(width, height, fbo_->depth());
}

const ref_ptr<FrameBufferObject>& FBOState::fbo()
{
  return fbo_;
}
