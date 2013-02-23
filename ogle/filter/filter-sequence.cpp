/*
 * filter-sequence.cpp
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#include <ogle/states/render-state.h>
#include <ogle/utility/string-util.h>

#include "filter-sequence.h"

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
    shaderDefine("HAS_GEOMETRY_SHADER", "TRUE");
    shaderDefine("IS_2D_TEXTURE", "FALSE");
    shaderDefine("IS_ARRAY_TEXTURE", "FALSE");
    shaderDefine("IS_CUBE_TEXTURE", "TRUE");
  }
  else if(dynamic_cast<Texture3D*>(input_.get()))
  {
    Texture3D *tex3D = (Texture3D*)input_.get();
    shaderDefine("HAS_GEOMETRY_SHADER", "TRUE");
    shaderDefine("NUM_TEXTURE_LAYERS", FORMAT_STRING(tex3D->depth()));
    shaderDefine("IS_2D_TEXTURE", "FALSE");
    shaderDefine("IS_ARRAY_TEXTURE", "TRUE");
    shaderDefine("IS_CUBE_TEXTURE", "FALSE");
  }
  else
  {
    shaderDefine("HAS_GEOMETRY_SHADER", "FALSE");
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
