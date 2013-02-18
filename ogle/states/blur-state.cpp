/*
 * blur-state.cpp
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#include <ogle/meshes/rectangle.h>
#include <ogle/states/shader-configurer.h>

#include "blur-state.h"

BlurState::BlurState(
    const ref_ptr<Texture> &input,
    const Vec2ui &textureSize)
: State(), input_(input)
{
  { // create the render target
    ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
        new FrameBufferObject(textureSize.x, textureSize.y, GL_NONE));
    framebuffer_ = ref_ptr<FBOState>::manage(new FBOState(fbo));
    joinStates(ref_ptr<State>::cast(framebuffer_));

    blurTexture0_ = fbo->addTexture(1, input_->format(), input_->internalFormat());
    blurTexture1_ = fbo->addTexture(1, input_->format(), input_->internalFormat());
  }

  sigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  sigma_->setUniformData(4.0f);
  sigma_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(sigma_));

  numPixels_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("numBlurPixels"));
  numPixels_->setUniformData(4.0f);
  numPixels_->set_isConstant(GL_TRUE);
  joinShaderInput(ref_ptr<ShaderInput>::cast(numPixels_));

  ref_ptr<State> blurSequence = ref_ptr<State>::manage(new StateSequence);
  joinStates(blurSequence);

  { // downsample input texture to blur buffer size -> GL_COLOR_ATTACHMENT0
    downsampleShader_ = ref_ptr<ShaderState>::manage(new ShaderState);

    ref_ptr<DrawBufferState> drawBuffer = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    drawBuffer->colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
    downsampleShader_->joinStates(ref_ptr<State>::cast(drawBuffer));

    ref_ptr<TextureState> texState =
        ref_ptr<TextureState>::manage(new TextureState(input_));
    texState->set_name("originalTexture");
    downsampleShader_->joinStates(ref_ptr<State>::cast(texState));

    downsampleShader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    blurSequence->joinStates(ref_ptr<State>::cast(downsampleShader_));
  }
  { // calculate blur in horizontal direction -> GL_COLOR_ATTACHMENT1
    blurHorizontalShader_ = ref_ptr<ShaderState>::manage(new ShaderState);

    ref_ptr<DrawBufferState> drawBuffer = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    drawBuffer->colorBuffers.push_back(GL_COLOR_ATTACHMENT1);
    blurHorizontalShader_->joinStates(ref_ptr<State>::cast(drawBuffer));

    ref_ptr<TextureState> blurTexState =
        ref_ptr<TextureState>::manage(new TextureState(blurTexture0_));
    blurTexState->set_name("blurTexture");
    blurHorizontalShader_->joinStates(ref_ptr<State>::cast(blurTexState));

    blurHorizontalShader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    blurSequence->joinStates(ref_ptr<State>::cast(blurHorizontalShader_));
  }
  { // calculate blur in vertical direction -> GL_COLOR_ATTACHMENT0
    blurVerticalShader_ = ref_ptr<ShaderState>::manage(new ShaderState);

    ref_ptr<DrawBufferState> drawBuffer = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
    drawBuffer->colorBuffers.push_back(GL_COLOR_ATTACHMENT0);
    blurVerticalShader_->joinStates(ref_ptr<State>::cast(drawBuffer));

    ref_ptr<TextureState> blurTexState =
        ref_ptr<TextureState>::manage(new TextureState(blurTexture1_));
    blurTexState->set_name("blurTexture");
    blurVerticalShader_->joinStates(ref_ptr<State>::cast(blurTexState));

    blurVerticalShader_->joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
    blurSequence->joinStates(ref_ptr<State>::cast(blurVerticalShader_));
  }
}

void BlurState::createShader(ShaderConfig &cfg)
{
  {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(downsampleShader_.get());
    downsampleShader_->createShader(_cfg.cfg(), "blur.downsample");
  }
  {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(blurHorizontalShader_.get());
    _cfg.define("BLUR_HORIZONTAL", "TRUE");
    blurHorizontalShader_->createShader(_cfg.cfg(), "blur");
  }
  {
    ShaderConfigurer _cfg(cfg);
    _cfg.addState(blurVerticalShader_.get());
    _cfg.define("BLUR_HORIZONTAL", "FALSE");
    blurVerticalShader_->createShader(_cfg.cfg(), "blur");
  }
}

const ref_ptr<Texture>& BlurState::blurTexture() const
{
  return blurTexture0_;
}
const ref_ptr<FBOState>& BlurState::framebuffer() const
{
  return framebuffer_;
}

void BlurState::set_sigma(GLfloat sigma)
{
  sigma_->setVertex1f(0,sigma);
}
const ref_ptr<ShaderInput1f>& BlurState::sigma() const
{
  return sigma_;
}

void BlurState::set_numPixels(GLfloat numPixels)
{
  numPixels_->setVertex1f(0,numPixels);
}
const ref_ptr<ShaderInput1f>& BlurState::numPixels() const
{
  return numPixels_;
}
