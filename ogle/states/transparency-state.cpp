/*
 * transparency-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "transparency-state.h"
#include <ogle/meshes/rectangle.h>
#include <ogle/states/render-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/blend-state.h>
#include <ogle/states/cull-state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/states/depth-state.h>
#include <ogle/utility/gl-error.h>
#include <ogle/utility/string-util.h>
#include <ogle/render-tree/shader-configurer.h>

TransparencyState::TransparencyState(
    TransparencyMode mode,
    GLuint bufferWidth, GLuint bufferHeight,
    const ref_ptr<Texture> &depthTexture,
    GLboolean useDoublePrecision)
: DirectShading(),
  mode_(mode)
{
  // use custom FBO with float format.
  // two attachments are used, one sums the color the other
  // sums the number of invocations.
  fbo_ = ref_ptr<FrameBufferObject>::manage(
      new FrameBufferObject(bufferWidth, bufferHeight));
  if(depthTexture.get()) {
    fbo_->set_depthAttachment(*((Texture2D*)depthTexture.get()));
  }

  GLboolean useFloatBuffer;
  switch(mode) {
  case TRANSPARENCY_MODE_SUM:
  case TRANSPARENCY_MODE_AVERAGE_SUM:
    useFloatBuffer = GL_TRUE;
    break;
  case TRANSPARENCY_MODE_FRONT_TO_BACK:
  case TRANSPARENCY_MODE_BACK_TO_FRONT:
  case TRANSPARENCY_MODE_NONE:
    useFloatBuffer = GL_FALSE;
    break;
  }
  if(useFloatBuffer) {
    colorTexture_ = fbo_->addTexture(1, GL_RGBA, useDoublePrecision ? GL_RGBA32F : GL_RGBA16F);
  } else {
    colorTexture_ = fbo_->addTexture(1, GL_RGBA, GL_RGBA);
  }

  switch(mode) {
  case TRANSPARENCY_MODE_AVERAGE_SUM:
    // with nvidia i get incomplete attachment error using GL_R16F.
    counterTexture_ = fbo_->addTexture(1, GL_RG, useDoublePrecision ? GL_RG32F : GL_RG16F);
    break;
  case TRANSPARENCY_MODE_FRONT_TO_BACK:
  case TRANSPARENCY_MODE_BACK_TO_FRONT:
  case TRANSPARENCY_MODE_SUM:
  case TRANSPARENCY_MODE_NONE:
    break;
  }
  handleFBOError("TransparencyState");

  GLuint numOutputs;
  switch(mode) {
  case TRANSPARENCY_MODE_FRONT_TO_BACK:
    numOutputs = 1;
    shaderDefine("USE_FRONT_TO_BACK_ALPHA", "TRUE");
    break;
  case TRANSPARENCY_MODE_BACK_TO_FRONT:
    numOutputs = 1;
    shaderDefine("USE_BACK_TO_FRONT_ALPHA", "TRUE");
    break;
  case TRANSPARENCY_MODE_AVERAGE_SUM:
    numOutputs = 2;
    shaderDefine("USE_AVG_SUM_ALPHA", "TRUE");
    break;
  case TRANSPARENCY_MODE_SUM:
    numOutputs = 1;
    shaderDefine("USE_SUM_ALPHA", "TRUE");
    break;
  case TRANSPARENCY_MODE_NONE:
    break;
  }

  {
    fboState_ = ref_ptr<FBOState>::manage(new FBOState(fbo_));

    // enable & clear attachments to zero
    ClearColorData clearData;
    clearData.clearColor = Vec4f(0.0f);
    for(GLuint i=0u; i<numOutputs; ++i) {
      clearData.colorBuffers.push_back(GL_COLOR_ATTACHMENT0+i);
    }
    fboState_->setClearColor(clearData);
    joinStates(ref_ptr<State>::cast(fboState_));
  }

  // enable depth test and disable depth write
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthWrite(GL_FALSE);
  joinStates(ref_ptr<State>::cast(depth));

  // disable face culling to see backsides of transparent objects
  switch(mode) {
  case TRANSPARENCY_MODE_FRONT_TO_BACK:
  case TRANSPARENCY_MODE_BACK_TO_FRONT:
    break;
  case TRANSPARENCY_MODE_AVERAGE_SUM:
  case TRANSPARENCY_MODE_SUM:
  case TRANSPARENCY_MODE_NONE:
    joinStates(ref_ptr<State>::manage(new CullDisableState));
    break;
  }

  // enable additive blending
  switch(mode) {
  case TRANSPARENCY_MODE_FRONT_TO_BACK:
    joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_FRONT_TO_BACK)));
    break;
  case TRANSPARENCY_MODE_BACK_TO_FRONT:
    joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_BACK_TO_FRONT)));
    break;
  case TRANSPARENCY_MODE_AVERAGE_SUM:
  case TRANSPARENCY_MODE_SUM:
  case TRANSPARENCY_MODE_NONE:
    joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ADD)));
    break;
  }
}

TransparencyMode TransparencyState::mode() const
{
  return mode_;
}

const ref_ptr<Texture>& TransparencyState::colorTexture() const
{
  return colorTexture_;
}
const ref_ptr<Texture>& TransparencyState::counterTexture() const
{
  return counterTexture_;
}
const ref_ptr<FBOState>& TransparencyState::fboState() const
{
  return fboState_;
}

///////////////
///////////////

AccumulateTransparency::AccumulateTransparency(TransparencyMode transparencyMode)
: State()
{
  switch(transparencyMode) {
  case TRANSPARENCY_MODE_AVERAGE_SUM:
    shaderDefine("USE_AVG_SUM_ALPHA", "TRUE");
    break;
  case TRANSPARENCY_MODE_SUM:
    shaderDefine("USE_SUM_ALPHA", "TRUE");
    break;
  case TRANSPARENCY_MODE_FRONT_TO_BACK:
  case TRANSPARENCY_MODE_BACK_TO_FRONT:
  case TRANSPARENCY_MODE_NONE:
    break;
  }

  // enable alpha blending
  joinStates(ref_ptr<State>::manage(new BlendState(BLEND_MODE_ALPHA)));
  // disable depth test/write
  ref_ptr<DepthState> depth = ref_ptr<DepthState>::manage(new DepthState);
  depth->set_useDepthTest(GL_FALSE);
  depth->set_useDepthWrite(GL_FALSE);
  joinStates(ref_ptr<State>::cast(depth));

  accumulationShader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(accumulationShader_));

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

void AccumulateTransparency::setColorTexture(const ref_ptr<Texture> &t)
{
  if(alphaColorTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(alphaColorTexture_));
  }
  alphaColorTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  alphaColorTexture_->set_name("tColorTexture");
  if(t.get()) joinStatesFront(ref_ptr<State>::cast(alphaColorTexture_));
}

void AccumulateTransparency::setCounterTexture(const ref_ptr<Texture> &t)
{
  if(alphaCounterTexture_.get()) {
    disjoinStates(ref_ptr<State>::cast(alphaCounterTexture_));
  }
  alphaCounterTexture_ = ref_ptr<TextureState>::manage(new TextureState(t));
  alphaCounterTexture_->set_name("tCounterTexture");
  if(t.get()) joinStatesFront(ref_ptr<State>::cast(alphaCounterTexture_));
}

void AccumulateTransparency::createShader(ShaderConfig &cfg)
{
  accumulationShader_->createShader(cfg, "transparency.resolve");
}
