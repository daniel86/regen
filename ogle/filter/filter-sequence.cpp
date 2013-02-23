/*
 * filter-sequence.cpp
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#include "filter-sequence.h"

FilterSequence::FilterSequence(const ref_ptr<Texture> &input)
: State(), input_(input)
{
  viewport_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("viewport"));
  viewport_->setUniformData( Vec2f(
      (GLfloat)input->width(), (GLfloat)input->height()) );
  joinShaderInput(ref_ptr<ShaderInput>::cast(viewport_));
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
  GLuint size = min(input_->width(), input_->height());

  for(list< ref_ptr<Filter> >::iterator
      it=filterSequence_.begin(); it!=filterSequence_.end(); ++it)
  {
    Filter *f = (Filter*) (*it).get();
    FrameBufferObject *fbo = f->output()->fbo_.get();

    if(last != fbo) {
      size *= f->scaleFactor();
      fbo->resize(size, size, fbo->depth());
    }

    last = fbo;
  }
}

void FilterSequence::enable(RenderState *state)
{
  FrameBufferObject *last = NULL;

  for(list< ref_ptr<Filter> >::iterator
      it=filterSequence_.begin(); it!=filterSequence_.end(); ++it)
  {
    Filter *f = (Filter*) (*it).get();

    // enable fbo if necessary
    FrameBufferObject *fbo = f->output()->fbo_.get();
    if(last != fbo) {
      fbo->bind();
      fbo->set_viewport();
      viewport_->setVertex2f(0, fbo->viewport()->getVertex2f(0));
      last = fbo;
    }

    f->enable(state);
    f->disable(state);
  }
}
