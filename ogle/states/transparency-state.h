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
#include <ogle/shading/light-state.h>
#include <ogle/states/shader-state.h>
#include <ogle/shading/direct.h>
#include <ogle/meshes/mesh-state.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/texture.h>

namespace ogle {
/**
 * Sets up blending and custom render target for rendering
 * transparent objects.
 */
class TransparencyState : public DirectShading
{
public:
  enum Mode {
    MODE_FRONT_TO_BACK,
    MODE_BACK_TO_FRONT,
    MODE_SUM,
    MODE_AVERAGE_SUM,
    MODE_NONE
  };

  TransparencyState(
      Mode mode,
      GLuint bufferWidth, GLuint bufferHeight,
      const ref_ptr<Texture> &depthTexture,
      GLboolean useDoublePrecision=GL_FALSE);

  Mode mode() const;

  /**
   * Texture with accumulated alpha values.
   */
  const ref_ptr<Texture>& colorTexture() const;
  /**
   * Only used for average sum transparency.
   */
  const ref_ptr<Texture>& counterTexture() const;

  const ref_ptr<FBOState>& fboState() const;

protected:
  Mode mode_;
  ref_ptr<FrameBufferObject> fbo_;
  ref_ptr<FBOState> fboState_;
  ref_ptr<Texture> colorTexture_;
  ref_ptr<Texture> counterTexture_;
};

///////////
//////////

/**
 * \brief Resolves result of transparency rendering pass.
 *
 * For example the average-sum technique requires resolving.
 */
class AccumulateTransparency : public FullscreenPass
{
public:
  AccumulateTransparency(TransparencyState::Mode transparencyMode);

  void setColorTexture(const ref_ptr<Texture> &t);
  void setCounterTexture(const ref_ptr<Texture> &t);

protected:
  ref_ptr<TextureState> alphaColorTexture_;
  ref_ptr<TextureState> alphaCounterTexture_;
};

} // end ogle namespace

#endif /* TRANSPARENCY_STATE_H_ */
