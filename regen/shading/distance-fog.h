/*
 * distance-fog.h
 *
 *  Created on: 07.02.2013
 *      Author: daniel
 */

#ifndef DISTANCE_FOG_H_
#define DISTANCE_FOG_H_

#include <regen/states/fullscreen-pass.h>
#include <regen/states/shader-state.h>
#include <regen/states/texture-state.h>

namespace regen {
  /**
   * \brief Fog with distance attenuation.
   */
  class DistanceFog : public FullscreenPass
  {
  public:
    DistanceFog();

    /**
     * @param depth GBuffer depth.
     */
    void set_gBuffer(const ref_ptr<Texture> &depth);
    /**
     * @param color TBuffer color.
     * @param depth TBuffer min depth.
     */
    void set_tBuffer(const ref_ptr<Texture> &color, const ref_ptr<Texture> &depth);

    /**
     * @param t TextureCube fog color.
     */
    void set_skyColor(const ref_ptr<TextureCube> &t);
    /**
     * @return constant fog color.
     */
    const ref_ptr<ShaderInput3f>& fogColor() const;
    /**
     * @return inner and outer fog distance to camera.
     */
    const ref_ptr<ShaderInput2f>& fogDistance() const;
    /**
     * @return constant fog density.
     */
    const ref_ptr<ShaderInput1f>& fogDensity() const;

  protected:
    ref_ptr<ShaderInput3f> fogColor_;
    ref_ptr<ShaderInput2f> fogDistance_;
    ref_ptr<ShaderInput1f> fogDensity_;

    ref_ptr<TextureState> skyColorTexture_;
    ref_ptr<TextureState> tDepthTexture_;
    ref_ptr<TextureState> tColorTexture_;
    ref_ptr<TextureState> gDepthTexture_;
  };
} // namespace

#endif /* DISTANCE_FOG_H_ */
