/*
 * blur-node.cpp
 *
 *  Created on: 12.11.2012
 *      Author: daniel
 */

#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

#include "blur-node.h"

class SwitchBufferIndex : public State
{
public:
  SwitchBufferIndex(ref_ptr<Texture> texture, GLuint renderSource)
  : State(),
    texture_(texture),
    renderSource_(renderSource)
  { }
  virtual void disable(RenderState *state) {
    texture_->set_bufferIndex(renderSource_);
  }
  ref_ptr<Texture> texture_;
  GLuint renderSource_;
};
class SwitchDrawBuffer : public State
{
public:
  SwitchDrawBuffer(ref_ptr<Texture> texture, GLenum renderTarget, GLuint renderSource)
  : State(),
    texture_(texture),
    renderTarget_(renderTarget),
    renderSource_(renderSource)
  { }
  virtual void disable(RenderState *state) {
    glDrawBuffer(renderTarget_);
    texture_->set_bufferIndex(renderSource_);
  }
  ref_ptr<Texture> texture_;
  GLenum renderTarget_;
  GLuint renderSource_;
};

BlurNode::BlurNode(
    ref_ptr<Texture> &input,
    ref_ptr<MeshState> &orthoQuad,
    GLfloat sizeScale)
: StateNode(),
  input_(input),
  sizeScale_(sizeScale)
{
  GLfloat blurWidth = sizeScale*input_->width();
  GLfloat blurHeight = sizeScale*input_->height();

  { // create the render target
    ref_ptr<FrameBufferObject> fbo = ref_ptr<FrameBufferObject>::manage(
        new FrameBufferObject(blurWidth, blurHeight, GL_NONE));
    framebuffer_ = ref_ptr<FBOState>::manage(new FBOState(fbo));
    state_ = ref_ptr<State>::cast(framebuffer_);
    // first pass draws to attachment 0
    framebuffer_->addDrawBuffer(GL_COLOR_ATTACHMENT0);

    blurredTexture_ = fbo->addTexture(2, input->format(), input->internalFormat());
  }

  { // downsample -> GL_COLOR_ATTACHMENT0
    downsample_ = ref_ptr<ShaderState>::manage(new ShaderState);
    downsample_->joinStates(ref_ptr<State>::cast(orthoQuad));
    downsample_->joinStates(ref_ptr<State>::manage(
        new SwitchDrawBuffer(blurredTexture_, GL_COLOR_ATTACHMENT1, 1u)));

    ref_ptr<TextureState> texState = ref_ptr<TextureState>::manage(new TextureState(input));
    texState->set_name("originalTexture");
    downsample_->joinStates(ref_ptr<State>::cast(texState));

    downsampleNode_ = new StateNode(ref_ptr<State>::cast(downsample_));
    addChild(ref_ptr<StateNode>::manage(downsampleNode_));
  }

  { // horizontal blur -> GL_COLOR_ATTACHMENT1
    blurHorizontal_ = ref_ptr<ShaderState>::manage(new ShaderState);
    blurHorizontal_->joinStates(ref_ptr<State>::cast(orthoQuad));
    blurHorizontal_->joinStates(ref_ptr<State>::manage(
        new SwitchDrawBuffer(blurredTexture_, GL_COLOR_ATTACHMENT0, 0u)));

    ref_ptr<TextureState> blurTexState = ref_ptr<TextureState>::manage(new TextureState(blurredTexture_));
    blurTexState->set_name("blurTexture");
    blurHorizontal_->joinStates(ref_ptr<State>::cast(blurTexState));

    blurHorizontalNode_ = new StateNode(ref_ptr<State>::cast(blurHorizontal_));
    addChild(ref_ptr<StateNode>::manage(blurHorizontalNode_));
  }

  { // vertical blur -> GL_COLOR_ATTACHMENT0
    blurVertical_ = ref_ptr<ShaderState>::manage(new ShaderState);
    blurVertical_->joinStates(ref_ptr<State>::cast(orthoQuad));
    blurVertical_->joinStates(ref_ptr<State>::manage(
        new SwitchBufferIndex(blurredTexture_, 0u)));

    ref_ptr<TextureState> blurTexState = ref_ptr<TextureState>::manage(new TextureState(blurredTexture_));
    blurTexState->set_name("blurTexture");
    blurVertical_->joinStates(ref_ptr<State>::cast(blurTexState));

    blurVerticalNode_ = new StateNode(ref_ptr<State>::cast(blurVertical_));
    addChild(ref_ptr<StateNode>::manage(blurVerticalNode_));
  }

  sigma_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("blurSigma"));
  sigma_->setUniformData(4.0f);
  sigma_->set_isConstant(GL_TRUE);
  state_->joinShaderInput(ref_ptr<ShaderInput>::cast(sigma_));

  numPixels_ = ref_ptr<ShaderInput1f>::manage(new ShaderInput1f("numBlurPixels"));
  numPixels_->setUniformData(4.0f);
  numPixels_->set_isConstant(GL_TRUE);
  state_->joinShaderInput(ref_ptr<ShaderInput>::cast(numPixels_));
}

void BlurNode::set_sigma(GLfloat sigma)
{
  sigma_->setVertex1f(0,sigma);
}
ref_ptr<ShaderInput1f>& BlurNode::sigma()
{
  return sigma_;
}

void BlurNode::set_numPixels(GLfloat numPixels)
{
  numPixels_->setVertex1f(0,numPixels);
}
ref_ptr<ShaderInput1f>& BlurNode::numPixels()
{
  return numPixels_;
}

void BlurNode::resize()
{
  GLfloat blurWidth = sizeScale_*input_->width();
  GLfloat blurHeight = sizeScale_*input_->height();
  framebuffer_->resize(blurWidth, blurHeight);
}


void BlurNode::set_parent(StateNode *parent)
{
  StateNode::set_parent(parent);

  map<string, string> shaderConfig_;
  map<GLenum, string> shaderNames_;

  { // downsample -> GL_COLOR_ATTACHMENT0
    ShaderConfig shaderCfg;
    downsampleNode_->configureShader(&shaderCfg);
    downsample_->createShader(shaderCfg, "blur.downsample");
  }

  { // horizontal blur -> GL_COLOR_ATTACHMENT1
    ShaderConfig shaderCfg;
    blurHorizontalNode_->configureShader(&shaderCfg);
    shaderCfg.define("BLUR_HORIZONTAL", "TRUE");
    blurHorizontal_->createShader(shaderCfg, "blur");
  }

  { // vertical blur -> GL_COLOR_ATTACHMENT0
    ShaderConfig shaderCfg;
    blurVerticalNode_->configureShader(&shaderCfg);
    shaderCfg.define("BLUR_VERTICAL", "TRUE");
    blurVertical_->createShader(shaderCfg, "blur");
  }
}

ref_ptr<Texture>& BlurNode::blurredTexture()
{
  return blurredTexture_;
}
