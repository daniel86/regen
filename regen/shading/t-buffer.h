/*
 * t-buffer.h
 *
 *  Created on: 15.03.2013
 *      Author: daniel
 */

#ifndef T_BUFFER_H_
#define T_BUFFER_H_

#include <ogle/shading/shading-direct.h>

namespace ogle {
/**
 * \brief Transparency-Buffer state.
 *
 * Transparency requires direct shading and most techniques
 * require z sorted geometry.
 */
class TBuffer : public DirectShading
{
public:
  /**
   * \brief Transparency mode.
   */
  enum Mode {
    /**
     * Front to back blending. Geometry should be sorted
     * back to front.
     */
    MODE_FRONT_TO_BACK,
    /**
     * Back to front blending. Geometry should be sorted
     * front to back.
     */
    MODE_BACK_TO_FRONT,
    /**
     * Use add blending. No sorting required.
     */
    MODE_SUM,
    /**
     * Use average add blending. Count samples per texel
     * and divide result by sample count in post pass.
     */
    MODE_AVERAGE_SUM
  };

  /**
   * @param mode the transparency mode.
   * @param bufferSize transparency texture size.
   * @param depthTexture GBuffer depth texture.
   */
  TBuffer(
      Mode mode, const Vec2ui &bufferSize,
      const ref_ptr<Texture> &depthTexture);
  /**
   * @param cfg the shader configuration.
   */
  void createShader(const ShaderState::Config &cfg);

  /**
   * @return the transparency mode.
   */
  Mode mode() const;

  /**
   * The color texture is first attachment point in FBO.
   * @return color output texture.
   */
  const ref_ptr<Texture>& colorTexture() const;
  /**
   * @return the FBO that is used.
   */
  const ref_ptr<FBOState>& fboState() const;

  // override
  void disable(RenderState *rs);

protected:
  Mode mode_;
  ref_ptr<FrameBufferObject> fbo_;
  ref_ptr<FBOState> fboState_;
  ref_ptr<Texture> colorTexture_;
  ref_ptr<Texture> counterTexture_;
  ref_ptr<State> accumulateState_;
};
} // namespace

#endif /* T_BUFFER_H_ */
