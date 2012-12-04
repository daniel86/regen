/*
 * transparency-state.h
 *
 *  Created on: 03.12.2012
 *      Author: daniel
 */

#ifndef TRANSPARENCY_STATE_H_
#define TRANSPARENCY_STATE_H_

#include <ogle/states/state.h>
#include <ogle/states/fbo-state.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>

// TODO: regular front-to-back / back-to-front order dependend alpha blending
//      * using regular rgba rendertarget
//      * implement model matrix sorting
enum TransparencyMode {
  TRANSPARENCY_SUM,
  TRANSPARENCY_AVERAGE_SUM,
  TRANSPARENCY_NONE
};

/**
 * Sets up blending and custom render target for rendering
 * transparent objects.
 */
class TransparencyState : public State
{
public:
  TransparencyState(
      TransparencyMode mode,
      GLuint bufferWidth, GLuint bufferHeight,
      ref_ptr<Texture> &depthTexture,
      GLboolean useDoublePrecision=GL_FALSE);

  /**
   * Texture with accumulated alpha values.
   */
  ref_ptr<Texture>& colorTexture();
  /**
   * Only used for average sum transparency.
   */
  ref_ptr<Texture>& counterTexture();

  void resize(GLuint bufferWidth, GLuint bufferHeight);

  virtual void enable(RenderState *rs);
  virtual void disable(RenderState *rs);

protected:
  ref_ptr<FrameBufferObject> fbo_;
  ref_ptr<FBOState> fboState_;
  ref_ptr<Texture> colorTexture_;
  ref_ptr<Texture> counterTexture_;
};

#endif /* TRANSPARENCY_STATE_H_ */
