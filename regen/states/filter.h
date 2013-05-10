/*
 * filter.h
 *
 *  Created on: 23.02.2013
 *      Author: daniel
 */

#ifndef FILTER_H_
#define FILTER_H_

#include <regen/states/fullscreen-pass.h>
#include <regen/states/texture-state.h>
#include <regen/states/fbo-state.h>

namespace regen {
  /**
   * \brief Apply a filter on input Texture.
   */
  class Filter : public FullscreenPass
  {
  public:
    /**
     * \brief Ping-Pong filter output target.
     */
    struct Output {
      /** FBO reference. */
      ref_ptr<FBO> fbo_;
      /** Ping texture. */
      ref_ptr<Texture> tex0_;
      /** Pong texture. */
      ref_ptr<Texture> tex1_;
    };

    /**
     * \note You have to call setInput() once or add the filter to a
     * FilterSequence before using the filter.
     */
    Filter(const string &shaderKey, GLfloat scaleFactor=1.0);

    /**
     * @param v toggles binding the input texture before filter is executed.
     */
    void set_bindInput(GLboolean v);
    /**
     * @param v target format.
     */
    void set_format(GLenum v);
    /**
     * @param v target internal format.
     */
    void set_internalFormat(GLenum v);
    /**
     * @param v target pixel type.
     */
    void set_pixelType(GLenum v);

    /**
     * Scale factor that is applied to the input texture when
     * filtering.
     */
    GLfloat scaleFactor() const;

    /**
     * Filter render target with ping-pong attachment points.
     */
    const ref_ptr<Output>& output() const;
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
    void setInput(const ref_ptr<Output> &lastOutput, GLenum lastAttachment);

  protected:
    ref_ptr<Texture> input_;
    ref_ptr<Output> out_;
    GLenum outputAttachment_;

    ref_ptr<DrawBufferState> drawBufferState_;
    ref_ptr<TextureState> inputState_;
    ref_ptr<ShaderState> shader_;

    GLfloat scaleFactor_;

    GLenum format_;
    GLenum internalFormat_;
    GLenum pixelType_;
    GLboolean bindInput_;

    void set_input(const ref_ptr<Texture> &input);
    ref_ptr<Texture> createTexture();
  };
} // end namespace

namespace regen {
  /**
   * \brief Filters input texture by applying a sequence of
   * filters to the input.
   */
  class FilterSequence : public State, public Resizable
  {
  public:
    /**
     * @param input the input texture.
     * @param bindInput bind and activate input before filtering.
     */
    FilterSequence(const ref_ptr<Texture> &input, GLboolean bindInput=GL_TRUE);
    /**
     * Creates filter shaders.
     * @param cfg the shader config.
     */
    void createShader(StateConfig &cfg);
    /**
     * Should be called when input texture size changes.
     */
    void resize();

    /**
     * Set color for clearing the color buffer
     * before the filter is executed.
     * @param v
     */
    void setClearColor(const Vec4f &v);

    /**
     * @param v target format.
     */
    void set_format(GLenum v);
    /**
     * @param v target internal format.
     */
    void set_internalFormat(GLenum v);
    /**
     * @param v target pixel type.
     */
    void set_pixelType(GLenum v);

    /**
     * Adds a filter to the sequence of filters.
     * @param f
     */
    void addFilter(const ref_ptr<Filter> &f);

    /**
     * @return the input texture.
     */
    const ref_ptr<Texture>& input() const;
    /**
     * @return the output texture.
     */
    const ref_ptr<Texture>& output() const;

    // override
    void enable(RenderState *state);

  protected:
    list< ref_ptr<Filter> > filterSequence_;
    ref_ptr<Texture> input_;
    ref_ptr<ShaderInput2f> viewport_;
    ref_ptr<ShaderInput2f> inverseViewport_;

    GLboolean clearFirstFilter_;
    Vec4f clearColor_;

    GLboolean bindInput_;
    GLenum format_;
    GLenum internalFormat_;
    GLenum pixelType_;
  };
} // end namespace

#endif /* FILTER_H_ */
