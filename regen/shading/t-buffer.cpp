/*
 * t-buffer.cpp
 *
 *  Created on: 15.03.2013
 *      Author: daniel
 */

#include <regen/states/depth-state.h>
#include <regen/states/state-configurer.h>

#include "t-buffer.h"
using namespace regen;

TBuffer::TBuffer(
    Mode mode, const Vec2ui &bufferSize,
    const ref_ptr<Texture> &depthTexture)
: DirectShading(), mode_(mode)
{
  // use custom FBO with float format.
  // two attachments are used, one sums the color the other
  // sums the number of invocations.
  fbo_ = ref_ptr<FBO>::alloc(bufferSize.x,bufferSize.y);
  if(depthTexture.get()) {
    RenderState::get()->drawFrameBuffer().push(fbo_->id());
    fbo_->set_depthAttachment(depthTexture);
    RenderState::get()->drawFrameBuffer().pop();
  }

  GLboolean useFloatBuffer = GL_FALSE;
  switch(mode) {
  case MODE_SUM:
  case MODE_AVERAGE_SUM:
    useFloatBuffer = GL_TRUE;
    break;
  case MODE_FRONT_TO_BACK:
  case MODE_BACK_TO_FRONT:
    break;
  }
  if(useFloatBuffer) {
    colorTexture_ = fbo_->addTexture(
        1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA16F, GL_FLOAT);
  } else {
    colorTexture_ = fbo_->addTexture(
        1, GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE);
  }

  switch(mode) {
  case MODE_AVERAGE_SUM: {
    // with nvidia i get incomplete attachment error using GL_R16F.
    counterTexture_ = fbo_->addTexture(
        1, GL_TEXTURE_2D, GL_RG, GL_RG16F, GL_FLOAT);

    accumulateState_ = ref_ptr<FullscreenPass>::alloc("transparency.avgSum");
    accumulateState_->joinStatesFront(
        ref_ptr<TextureState>::alloc(counterTexture_, "tCounterTexture"));
    accumulateState_->joinStatesFront(
        ref_ptr<TextureState>::alloc(colorTexture_, "tColorTexture"));
    // enable alpha blending
    accumulateState_->joinStatesFront(ref_ptr<BlendState>::alloc(BLEND_MODE_ALPHA));
    // disable depth test/write
    ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
    depth->set_useDepthTest(GL_FALSE);
    depth->set_useDepthWrite(GL_FALSE);
    accumulateState_->joinStatesFront(depth);

    accumulateState_ = ref_ptr<State>::alloc();
    break;
  }
  case MODE_FRONT_TO_BACK:
  case MODE_BACK_TO_FRONT:
  case MODE_SUM:
    accumulateState_ = ref_ptr<State>::alloc();
    break;
  }
  FBO_ERROR_LOG();

  GLuint numOutputs = 1;
  switch(mode) {
  case MODE_FRONT_TO_BACK:
    shaderDefine("USE_FRONT_TO_BACK_ALPHA", "TRUE");
    break;
  case MODE_BACK_TO_FRONT:
    shaderDefine("USE_BACK_TO_FRONT_ALPHA", "TRUE");
    break;
  case MODE_AVERAGE_SUM:
    numOutputs = 2;
    shaderDefine("USE_AVG_SUM_ALPHA", "TRUE");
    break;
  case MODE_SUM:
    shaderDefine("USE_SUM_ALPHA", "TRUE");
    break;
  }

  {
    fboState_ = ref_ptr<FBOState>::alloc(fbo_);

    // enable & clear attachments to zero
    ClearColorState::Data clearData;
    clearData.clearColor = Vec4f(0.0f);
    for(GLuint i=0u; i<numOutputs; ++i) {
      clearData.colorBuffers.buffers_.push_back(GL_COLOR_ATTACHMENT0+i);
    }
    fboState_->setClearColor(clearData);
    joinStates(fboState_);
  }

  // enable depth test and disable depth write
  {
    ref_ptr<DepthState> depth = ref_ptr<DepthState>::alloc();
    depth->set_useDepthWrite(GL_FALSE);
    depth->set_useDepthTest(GL_TRUE);
    joinStates(depth);
  }

  // disable face culling to see backsides of transparent objects
  switch(mode) {
  case MODE_FRONT_TO_BACK:
  case MODE_BACK_TO_FRONT:
    break;
  case MODE_AVERAGE_SUM:
  case MODE_SUM:
    joinStates(ref_ptr<ToggleState>::alloc(RenderState::CULL_FACE, GL_FALSE));
    break;
  }

  // enable additive blending
  switch(mode) {
  case MODE_FRONT_TO_BACK:
    joinStates(ref_ptr<BlendState>::alloc(BLEND_MODE_FRONT_TO_BACK));
    break;
  case MODE_BACK_TO_FRONT:
    joinStates(ref_ptr<BlendState>::alloc(BLEND_MODE_BACK_TO_FRONT));
    break;
  case MODE_AVERAGE_SUM:
  case MODE_SUM:
    joinStates(ref_ptr<BlendState>::alloc(BLEND_MODE_ADD));
    break;
  }
}

void TBuffer::createShader(const StateConfig &cfg)
{
  State *s = accumulateState_.get();
  FullscreenPass *fp = dynamic_cast<FullscreenPass*>(s);
  if(fp!=NULL) {
    StateConfigurer cfg_(cfg);
    cfg_.addState(s);
    fp->createShader(cfg_.cfg());
  }
}

void TBuffer::disable(RenderState *rs)
{
  accumulateState_->enable(rs);
  accumulateState_->disable(rs);
  State::disable(rs);
}

TBuffer::Mode TBuffer::mode() const
{ return mode_; }
const ref_ptr<Texture>& TBuffer::colorTexture() const
{ return colorTexture_; }
const ref_ptr<FBOState>& TBuffer::fboState() const
{ return fboState_; }
