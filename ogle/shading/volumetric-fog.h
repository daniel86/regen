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
 *
 */
class VolumetricFog : public State
{
public:
  VolumetricFog();

  void createShader(ShaderState::Config &cfg);

  void set_gDepthTexture(const ref_ptr<Texture> &t);
  void set_tBuffer(const ref_ptr<Texture> &color, const ref_ptr<Texture> &depth);

  void addLight(
      const ref_ptr<SpotLight> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale,
      const ref_ptr<ShaderInput2f> &coneScale);
  void addLight(
      const ref_ptr<PointLight> &l,
      const ref_ptr<ShaderInput1f> &exposure,
      const ref_ptr<ShaderInput2f> &radiusScale);

  void removeLight(SpotLight *l);
  void removeLight(PointLight *l);

  const ref_ptr<ShaderInput1f>& fogStart() const;
  const ref_ptr<ShaderInput1f>& fogEnd() const;

protected:
  ref_ptr<TextureState> tDepthTexture_;
  ref_ptr<TextureState> tColorTexture_;
  ref_ptr<TextureState> gDepthTexture_;

  ref_ptr<StateSequence> fogSequence_;
  ref_ptr<LightPass> spotFog_;
  ref_ptr<LightPass> pointFog_;

  ref_ptr<ShaderInput1f> fogStart_;
  ref_ptr<ShaderInput1f> fogEnd_;
};
} // namespace

#endif /* VOLUMETRIC_FOG_H_ */
