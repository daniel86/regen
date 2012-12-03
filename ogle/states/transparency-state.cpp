/*
 * transparency-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "transparency-state.h"
#include <ogle/states/render-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/utility/gl-error.h>

TransparencyState::TransparencyState(
    TransparencyMode mode,
    GLuint bufferWidth, GLuint bufferHeight,
    ref_ptr<Texture> &depthTexture,
    GLboolean useDoublePrecision)
: State()
{
  // use custom FBO with float format.
  // two attachments are used, one sums the color the other
  // sums the number of invocations.
  fbo_ = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(bufferWidth, bufferHeight));
  fbo_->set_depthAttachment(*((Texture2D*)depthTexture.get()));

  GLuint numOutputs;
  colorTexture_ = fbo_->addTexture(1, GL_RGBA,
      useDoublePrecision ? GL_RGBA32F : GL_RGBA16F);
  switch(mode) {
  case TRANSPARENCY_AVERAGE_SUM:
    counterTexture_ = fbo_->addTexture(1, GL_RG,
        useDoublePrecision ? GL_RG32F : GL_RG16F);
    shaderDefine("USE_AVG_SUM_ALPHA", "TRUE");
    numOutputs = 2;
    break;
  case TRANSPARENCY_SUM:
    numOutputs = 1;
    shaderDefine("USE_SUM_ALPHA", "TRUE");
    break;
  case TRANSPARENCY_NONE:
    assert(0);
  }
  handleFBOError("TransparencyState");

  {
    fboState_ = ref_ptr<FBOState>::manage(new FBOState(fbo_));

    // enable & clear attachments to zero
    ClearColorData clearData;
    clearData.clearColor = Vec4f(0.0f);
    for(GLuint i=0u; i<numOutputs; ++i) {
      clearData.colorBuffers.push_back(GL_COLOR_ATTACHMENT0+i);
    }
    fboState_->setClearColor(clearData);
  }

  // enable depth test and disable depth write
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthWrite(GL_FALSE);
  joinStates(ref_ptr<State>::cast(depth));

  // enable additive blending
  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));
}

ref_ptr<Texture>& TransparencyState::colorTexture()
{
  return colorTexture_;
}
ref_ptr<Texture>& TransparencyState::counterTexture()
{
  return counterTexture_;
}

void TransparencyState::enable(RenderState *rs)
{
  // XXX: problems with shadows
  //glDisable(GL_CULL_FACE);
  fboState_->enable(rs);
  State::enable(rs);
}
void TransparencyState::disable(RenderState *rs)
{
  State::disable(rs);
  fboState_->disable(rs);
  //glEnable(GL_CULL_FACE);
}

void TransparencyState::resize(GLuint bufferWidth, GLuint bufferHeight)
{
  fbo_->resize(bufferWidth, bufferHeight);
}
