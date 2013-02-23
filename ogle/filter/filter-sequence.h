/*
 * filter-sequence.h
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#ifndef FILTER_SEQUENCE_H_
#define FILTER_SEQUENCE_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/fbo-state.h>

#include <ogle/filter/filter.h>

/**
 * Filters input texture by applying a sequence of
 * filters to the input.
 */
class FilterSequence : public State
{
public:
  FilterSequence(const ref_ptr<Texture> &input);
  void createShader(ShaderConfig &cfg);

  /**
   * Should be called when input texture size changes.
   */
  void resize();

  void addFilter(const ref_ptr<Filter> &f);
  void removeFilter(const ref_ptr<Filter> &f);

  const ref_ptr<Texture>& input() const;
  const ref_ptr<Texture>& output() const;

  // override
  virtual void enable(RenderState *state);

protected:
  list< ref_ptr<Filter> > filterSequence_;
  ref_ptr<Texture> input_;
  ref_ptr<ShaderInput2f> viewport_;
};

#endif /* FILTER_SEQUENCE_H_ */
