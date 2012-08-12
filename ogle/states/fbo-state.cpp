/*
 * fbo-node.cpp
 *
 *  Created on: 03.08.2012
 *      Author: daniel
 */

#include "fbo-state.h"
#include <ogle/utility/string-util.h>

FBOState::FBOState(ref_ptr<FrameBufferObject> &fbo)
: State(),
  fbo_(fbo)
{
  viewportUniform_ = ref_ptr<UniformVec2>::manage(
      new UniformVec2("viewport", 1, Vec2f(0.0, 0.0)));
  viewportUniform_->set_value( Vec2f(
      (float)fbo_->width(), (float)fbo_->height() ) );
  joinUniform(ref_ptr<Uniform>::cast(viewportUniform_));
}

string FBOState::name()
{
  return FORMAT_STRING("FBOState(" << fbo_->id() << ")");
}

void FBOState::setClearDepth()
{
  if(clearDepthCallable_.get()) {
    removeEnabler(ref_ptr<Callable>::cast(clearDepthCallable_));
  }
  clearDepthCallable_ = ref_ptr<ClearDepthState>::manage(new ClearDepthState);
  addEnabler(ref_ptr<Callable>::cast(clearDepthCallable_));
}

void FBOState::setClearColor(const ClearColorData &data)
{
  if(clearColorCallable_.get()) {
    removeEnabler(ref_ptr<Callable>::cast(clearColorCallable_));
  }
  clearColorCallable_ = ref_ptr<ClearColorState>::manage(new ClearColorState);
  clearColorCallable_->data.push_back(data);
  addEnabler(ref_ptr<Callable>::cast(clearColorCallable_));
}
void FBOState::setClearColor(const list<ClearColorData> &data)
{
  if(clearColorCallable_.get()) {
    removeEnabler(ref_ptr<Callable>::cast(clearColorCallable_));
  }
  clearColorCallable_ = ref_ptr<ClearColorState>::manage(new ClearColorState);
  for(list<ClearColorData>::const_iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    clearColorCallable_->data.push_back(*it);
  }
  addEnabler(ref_ptr<Callable>::cast(clearColorCallable_));
}

ref_ptr<Texture> FBOState::addDefaultDrawBuffer(
    bool pingPongBuffer, GLenum colorAttachment)
{
  ref_ptr<ShaderFragmentOutput> output =
      ref_ptr<ShaderFragmentOutput>::manage(new DefaultFragmentOutput);
  output->set_colorAttachment(colorAttachment);
  addDrawBuffer(output);
  return fbo_->addRectangleTexture(pingPongBuffer ? 2 : 1);
}

ref_ptr<Texture> FBOState::addDrawBuffer(
    ref_ptr<ShaderFragmentOutput> output)
{
  if(drawBufferCallable_.get()==NULL) {
    drawBufferCallable_ = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    addEnabler(ref_ptr<Callable>::cast(drawBufferCallable_));
  }
  drawBufferCallable_->colorBuffers.push_back(
      GL_COLOR_ATTACHMENT0 - output->colorAttachment());
  fragmentOutputs_.push_back(output);
}

void FBOState::enable(RenderState *state)
{
  cout << "FBOState::enable" << endl;
  state->pushFBO(fbo_.get());
  State::enable(state);
}

void FBOState::disable(RenderState *state)
{
  cout << "FBOState::disable" << endl;
  State::disable(state);
  state->popFBO();
}

void FBOState::resize(GLuint width, GLuint height)
{
  fbo_->resize(width, height);
  viewportUniform_->set_value( Vec2f(
      (float)fbo_->width(), (float)fbo_->height() ) );
}

ref_ptr<FrameBufferObject>& FBOState::fbo()
{
  return fbo_;
}

void FBOState::configureShader(ShaderConfiguration *cfg)
{
  State::configureShader(cfg);
  cfg->setFragmentOutputs(fragmentOutputs_);
}

ClearDepthState::ClearDepthState()
: Callable()
{
}
void ClearDepthState::call()
{
  glClear(GL_DEPTH_BUFFER_BIT);
}

ClearColorState::ClearColorState()
: Callable()
{
}
void ClearColorState::call()
{
  for(list<ClearColorData>::iterator
      it=data.begin(); it!=data.end(); ++it)
  {
    ClearColorData &colData = *it;
    glDrawBuffer(colData.colorAttachment);
    glClearColor(
        colData.clearColor.x,
        colData.clearColor.y,
        colData.clearColor.z,
        colData.clearColor.w);
    glClear(GL_COLOR_BUFFER_BIT);
  }
}

DrawBufferState::DrawBufferState()
: Callable()
{
}
void DrawBufferState::call()
{
  glDrawBuffers(colorBuffers.size(), &colorBuffers[0]);
}

