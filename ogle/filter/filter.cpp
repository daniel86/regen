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
  inputState_->set_name("inputTexture");
}

void Filter::setInput(const ref_ptr<Texture> &input)
{
  set_input(input);

  //GLuint inputSize = min(input_->width(), input_->height());
  //GLuint bufferSize  = scaleFactor_*inputSize;
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

/////////////////
/////////////////


FilterSequence::FilterSequence(const ref_ptr<Texture> &input)
: State(), input_(input)
{
  viewport_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("viewport"));
  viewport_->setUniformData( Vec2f(
      (GLfloat)input->width(), (GLfloat)input->height()) );
  joinShaderInput(ref_ptr<ShaderInput>::cast(viewport_));

  // use layered geometry shader for 3d textures
  if(dynamic_cast<TextureCube*>(input_.get()))
  {
    shaderDefine("IS_2D_TEXTURE", "FALSE");
    shaderDefine("IS_ARRAY_TEXTURE", "FALSE");
    shaderDefine("IS_CUBE_TEXTURE", "TRUE");
  }
  else if(dynamic_cast<Texture3D*>(input_.get()))
  {
    Texture3D *tex3D = (Texture3D*)input_.get();
    shaderDefine("NUM_TEXTURE_LAYERS", FORMAT_STRING(tex3D->depth()));
    shaderDefine("IS_2D_TEXTURE", "FALSE");
    shaderDefine("IS_ARRAY_TEXTURE", "TRUE");
    shaderDefine("IS_CUBE_TEXTURE", "FALSE");
  }
  else
  {
    shaderDefine("IS_2D_TEXTURE", "TRUE");
    shaderDefine("IS_CUBE_TEXTURE", "FALSE");
    shaderDefine("IS_ARRAY_TEXTURE", "FALSE");
  }
}

const ref_ptr<Texture>& FilterSequence::input() const
{
  return input_;
}
const ref_ptr<Texture>& FilterSequence::output() const
{
  if(filterSequence_.empty()) {
    // no filter added yet, return identity
    return input_;
  }
  ref_ptr<Filter> lastFilter = *filterSequence_.rbegin();
  GLenum last = lastFilter->outputAttachment();
  if(last == GL_COLOR_ATTACHMENT0) {
    return lastFilter->output()->tex0_;
  } else {
    return lastFilter->output()->tex1_;
  }
}

void FilterSequence::addFilter(const ref_ptr<Filter> &f)
{
  if(filterSequence_.empty()) {
    // no filter was added before, create a new framebuffer
    f->setInput(input_);
  }
  else {
    ref_ptr<Filter> lastFilter = *filterSequence_.rbegin();
    // another filter was added before
    if(isApprox(f->scaleFactor(), 1.0)) {
      // filter does not apply scaling. We gonna ping pong render
      // to the framebuffer provided by previous filter.
      f->setInput(lastFilter->output(), lastFilter->outputAttachment());
    }
    else {
      // create a new framebuffer for this filter
      GLenum last = lastFilter->outputAttachment();
      if(last == GL_COLOR_ATTACHMENT0) {
        f->setInput(lastFilter->output()->tex0_);
      } else {
        f->setInput(lastFilter->output()->tex1_);
      }
    }
  }

  filterSequence_.push_back(f);
}
void FilterSequence::removeFilter(const ref_ptr<Filter> &f)
{
  // XXX
}

void FilterSequence::createShader(ShaderConfig &cfg)
{
  for(list< ref_ptr<Filter> >::iterator
      it=filterSequence_.begin(); it!=filterSequence_.end(); ++it)
  {
    Filter *f = (Filter*) (*it).get();
    f->createShader(cfg);
  }
}

void FilterSequence::resize()
{
  FrameBufferObject *last = NULL;
  //GLuint size = min(input_->width(), input_->height());
  GLuint width = input_->width();
  GLuint height = input_->height();

  for(list< ref_ptr<Filter> >::iterator
      it=filterSequence_.begin(); it!=filterSequence_.end(); ++it)
  {
    Filter *f = (Filter*) (*it).get();
    FrameBufferObject *fbo = f->output()->fbo_.get();

    if(last != fbo) {
      width  *= f->scaleFactor();
      height *= f->scaleFactor();
      fbo->resize(width, height, fbo->depth());
    }

    last = fbo;
  }
}

void FilterSequence::enable(RenderState *rs)
{
  State::enable(rs);

  if(filterSequence_.empty()) { return; }

  Filter *firstFilter = (Filter*)(*filterSequence_.begin()).get();

  FrameBufferObject *last = firstFilter->output()->fbo_.get();
  viewport_->setVertex2f(0, last->viewport()->getVertex2f(0));

  rs->pushFBO(last);
  for(list< ref_ptr<Filter> >::iterator
      it=filterSequence_.begin(); it!=filterSequence_.end(); ++it)
  {
    Filter *f = (Filter*) (*it).get();

    // enable fbo if necessary
    FrameBufferObject *fbo = f->output()->fbo_.get();
    if(last != fbo) {
      rs->popFBO();
      rs->pushFBO(fbo);
      viewport_->setVertex2f(0, fbo->viewport()->getVertex2f(0));
      last = fbo;
    }

    f->enable(rs);
    f->disable(rs);
  }
  rs->popFBO();
}
