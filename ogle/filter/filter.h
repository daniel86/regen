/*
 * filter.h
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#ifndef FILTER_H_
#define FILTER_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/fbo-state.h>

// Ping-Pong filter output target
struct FilterOutput {
  ref_ptr<FrameBufferObject> fbo_;
  ref_ptr<Texture> tex0_;
  ref_ptr<Texture> tex1_;
};

/**
 * Baseclass for filter operations.
 */
class Filter : public State
{
public:
  /**
   * Note: You have to call setInput() once or add the filter to a
   * FilterSequence before using the filter.
   */
  Filter(const string &shaderKey, GLfloat scaleFactor);
  void createShader(ShaderConfig &cfg);

  /**
   * Scale factor that is applied to the input texture when
   * filtering.
   */
  GLfloat scaleFactor() const;

  /**
   * Filter render target with ping-pong attachment points.
   */
  const ref_ptr<FilterOutput>& output() const;
  /**
   * The color attachment point for the filter result texture.
   */
  GLenum outputAttachment() const;

  /**
   * Set input texture and create a framebuffer for this filter.
   */
  void setInput(const ref_ptr<Texture> &input);
  /**
   * Set input texture and use provided framebuffer.
   */
  void setInput(const ref_ptr<FilterOutput> &lastOutput, GLenum lastAttachment);

protected:
  ref_ptr<Texture> input_;
  ref_ptr<FilterOutput> out_;
  GLenum outputAttachment_;

  ref_ptr<DrawBufferState> drawBufferState_;
  ref_ptr<TextureState> inputState_;
  ref_ptr<ShaderState> shader_;

  string shaderKey_;
  GLfloat scaleFactor_;

  void set_input(const ref_ptr<Texture> &input);
};

#endif /* FILTER_H_ */
