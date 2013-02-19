/*
 * fbo-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "fbo-state.h"
#include <ogle/utility/string-util.h>
#include <ogle/states/render-state.h>

ClearDepthState::ClearDepthState()
: State()
{
}
void ClearDepthState::enable(RenderState *state)
{
  glClear(GL_DEPTH_BUFFER_BIT);
}

ClearColorState::ClearColorState()
: State()
{
}
void ClearColorState::enable(RenderState *state)
{
  for(list<ClearColorData>::iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    ClearColorData &colData = *it;
    glDrawBuffers(colData.colorBuffers.size(), &colData.colorBuffers[0]);
    glClearColor(
        colData.clearColor.x,
        colData.clearColor.y,
        colData.clearColor.z,
        colData.clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

DrawBufferState::DrawBufferState()
: State()
{
}
void DrawBufferState::enable(RenderState *state)
{
  glDrawBuffers(colorBuffers.size(), &colorBuffers[0]);
}

/////////////////

DrawBufferTex::DrawBufferTex(
    const ref_ptr<Texture> &_t, GLenum _baseAttachment, GLboolean _isOntop)
: DrawBufferState(),
  tex(_t),
  isOntop(_isOntop),
  baseAttachment(_baseAttachment)
{
}
void DrawBufferTex::enable(RenderState *state)
{
  if(isOntop) {
    glDrawBuffer(baseAttachment + !tex->bufferIndex());
  }
  else {
    glDrawBuffer(baseAttachment + tex->bufferIndex());
    tex->nextBuffer();
  }
}

/////////////////

NextTextureBuffer::NextTextureBuffer(const ref_ptr<Texture> &_t)
: State(), tex(_t)
{
}
void NextTextureBuffer::enable(RenderState *state)
{
  tex->nextBuffer();
}

/////////////////

FBOState::FBOState(const ref_ptr<FrameBufferObject> &fbo)
: State(), fbo_(fbo)
{
  joinShaderInput(ref_ptr<ShaderInput>::cast(fbo->viewport()));
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

void FBOState::setClearColor(const ClearColorData &data)
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
void FBOState::setClearColor(const list<ClearColorData> &data)
{
  if(clearColorCallable_.get()) {
    disjoinStates(ref_ptr<State>::cast(clearColorCallable_));
  }
  clearColorCallable_ = ref_ptr<ClearColorState>::manage(new ClearColorState);
  for(list<ClearColorData>::const_iterator
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

ref_ptr<Texture> FBOState::addDefaultDrawBuffer(
    GLboolean pingPongBuffer, GLenum colorAttachment)
{
  addDrawBuffer(colorAttachment);
  return fbo_->addRectangleTexture(pingPongBuffer ? 2 : 1);
}

vector<GLenum> FBOState::drawBuffers()
{
  if(drawBufferCallable_.get()==NULL) {
    return vector<GLenum>();
  } else {
    return drawBufferCallable_->colorBuffers;
  }
}
void FBOState::addDrawBuffer(GLenum colorAttachment)
{
  if(drawBufferCallable_.get()==NULL) {
    drawBufferCallable_ = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    joinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
  drawBufferCallable_->colorBuffers.push_back(colorAttachment);
}

void FBOState::addDrawBufferOntop(const ref_ptr<Texture> &t, GLenum baseAttachment)
{
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
  drawBufferCallable_ = ref_ptr<DrawBufferState>::manage(
      new DrawBufferTex(t,baseAttachment,GL_TRUE));
  joinStates(ref_ptr<State>::cast(drawBufferCallable_));
}
void FBOState::addDrawBufferUpdate(const ref_ptr<Texture> &t, GLenum baseAttachment)
{
  if(drawBufferCallable_.get()!=NULL) {
    disjoinStates(ref_ptr<State>::cast(drawBufferCallable_));
  }
  drawBufferCallable_ = ref_ptr<DrawBufferState>::manage(
      new DrawBufferTex(t,baseAttachment,GL_FALSE));
  joinStates(ref_ptr<State>::cast(drawBufferCallable_));
}

void FBOState::enable(RenderState *state)
{
  state->pushFBO(fbo_.get());
  State::enable(state);
}

void FBOState::disable(RenderState *state)
{
  State::disable(state);
  state->popFBO();
}

void FBOState::resize(GLuint width, GLuint height)
{
  fbo_->resize(width, height);
}

const ref_ptr<FrameBufferObject>& FBOState::fbo()
{
  return fbo_;
}
