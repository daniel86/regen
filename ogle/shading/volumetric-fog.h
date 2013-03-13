/*
 * volumetric-fog.h
 *
 *  Created on: 07.02.2013
 *      Author: daniel
 */

#ifndef VOLUMETRIC_FOG_H_
#define VOLUMETRIC_FOG_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/shading/light-pass.h>

namespace ogle {
/**
 * \brief Fog volume for spot and point lights.
 */
class VolumetricFog : public State
{
public:
  VolumetricFog();
  /**
   * @param cfg the shader configuration.
   */
  void createShader(ShaderState::Config &cfg);

  void set_gDepthTexture(const ref_ptr<Texture> &t);
  void set_tBuffer(const ref_ptr<Texture> &color, const ref_ptr<Texture> &depth);

  /**
   * @param l a light.
   * @param exposure fog exposure.
   * @param radiusScale light radius scale.
   * @param coneScale light cone angle scale.
   */
  void addSpotLight(
      const ref_ptr<Light> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale,
      const ref_ptr<ShaderInput2f> &coneScale);
  /**
   * @param l a light.
   * @param exposure fog exposure.
   * @param radiusScale light radius scale.
   */
  void addPointLight(
      const ref_ptr<Light> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale);
  /**
   * @param l previously added light.
   */
  void removeLight(Light *l);

  /**
   * @return inner and outer fog distance to camera.
   */
  const ref_ptr<ShaderInput2f>& fogDistance() const;

protected:
  ref_ptr<TextureState> tDepthTexture_;
  ref_ptr<TextureState> tColorTexture_;
  ref_ptr<TextureState> gDepthTexture_;

  ref_ptr<StateSequence> fogSequence_;
  ref_ptr<LightPass> spotFog_;
  ref_ptr<LightPass> pointFog_;

  ref_ptr<ShaderInput2f> fogDistance_;
};
} // namespace

#endif /* VOLUMETRIC_FOG_H_ */
