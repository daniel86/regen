/*
 * filter.cpp
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#include "filter.h"

#include <ogle/states/shader-configurer.h>
#include <ogle/meshes/rectangle.h>
#include <ogle/utility/string-util.h>

Filter::Filter(const string &shaderKey, GLfloat scaleFactor)
: State(), shaderKey_(shaderKey), scaleFactor_(scaleFactor)
{
  drawBufferState_ = ref_ptr<DrawBufferState>::manage(new DrawBufferState);
  joinStates(ref_ptr<State>::cast(drawBufferState_));

  inputState_ = ref_ptr<TextureState>::manage(new TextureState);
  inputState_->set_name("inputTexture");
  joinStates(ref_ptr<State>::cast(inputState_));

  shader_ = ref_ptr<ShaderState>::manage(new ShaderState);
  joinStates(ref_ptr<State>::cast(shader_));

  joinStates(ref_ptr<State>::cast(Rectangle::getUnitQuad()));
}

const ref_ptr<FilterOutput>& Filter::output() const
{
  return out_;
}
GLenum Filter::outputAttachment() const
{
  return outputAttachment_;
}
GLfloat Filter::scaleFactor() const
{
  return scaleFactor_;
}

void Filter::createShader(ShaderConfig &cfg)
{
  ShaderConfigurer _cfg(cfg);
  _cfg.addState(this);
  shader_->createShader(_cfg.cfg(), shaderKey_);
}

void Filter::set_input(const ref_ptr<Texture> &input)
{
  input_ = input;
  inputState_->set_texture(input_);
}

void Filter::setInput(const ref_ptr<Texture> &input)
{
  set_input(input);

  //GLuint inputSize = min(input_->width(), input_->height());
  //GLuint bufferSize  = scaleFactor_*inputSize;
  //GLuint inputSize = min(input_->width(), input_->height());
  GLuint bufferW  = scaleFactor_*input_->width();
  GLuint bufferH  = scaleFactor_*input_->height();

  GLuint inputDepth = 1;
  if(dynamic_cast<Texture3D*>(input_.get())) {
    inputDepth = ((Texture3D*)input_.get())->depth();
  }

  // create the render target. As this is the first filter with this target
  // the attachment point is GL_COLOR_ATTACHMENT0 and only a single texture
  // is added to the fbo.
  out_ = ref_ptr<FilterOutput>::manage(new FilterOutput);
  out_->fbo_ = ref_ptr<FrameBufferObject>::manage(new FrameBufferObject(
      bufferW,bufferH,inputDepth,
      GL_NONE,GL_NONE,GL_NONE));
  out_->tex0_ = out_->fbo_->addTexture(1, input_->targetType(),
      input_->format(), input_->internalFormat(), input_->pixelType());

  // call drawBuffer( GL_COLOR_ATTACHMENT0 )
  outputAttachment_ = GL_COLOR_ATTACHMENT0;
  drawBufferState_->colorBuffers.clear();
  drawBufferState_->colorBuffers.push_back(outputAttachment_);
}

void Filter::setInput(
    const ref_ptr<FilterOutput> &lastOutput, GLenum lastAttachment)
{
  // take last output as input
  if(lastAttachment == GL_COLOR_ATTACHMENT0) {
    input_ = lastOutput->tex0_;
  } else {
    input_ = lastOutput->tex1_;
  }
  set_input(input_);

  out_ = lastOutput;
  // make sure two textures created for ping pong rendering
  if(!out_->tex1_.get()) {
    out_->tex1_ = out_->fbo_->addTexture(1,
        input_->targetType(),
        input_->format(),
        input_->internalFormat(),
        input_->pixelType());
  }

  // call drawBuffer( outputAttachment_ )
  outputAttachment_ = GL_COLOR_ATTACHMENT0 + (GL_COLOR_ATTACHMENT1 - lastAttachment);
  drawBufferState_->colorBuffers.clear();
  drawBufferState_->colorBuffers.push_back(outputAttachment_);
}
