/*
 * ambient-occlusion.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_AMBIENT_OCCLUSION_H_
#define __SHADING_AMBIENT_OCCLUSION_H_

#include <ogle/states/state.h>
#include <ogle/filter/filter.h>

namespace ogle {

/**
 * Computes dynamic ambient occlusion as described in gpugems2_chapter14.
 * The AO is computed on a downsampled render target and blurred with
 * a separable blur afterwards.
 */
class AmbientOcclusion : public State
{
public:
  AmbientOcclusion(GLfloat sizeScale);
  void createResources(ShaderConfig &cfg, const ref_ptr<Texture> &input);
  void resize();

  /**
   * The Ambient Occlusion Texture.
   * You could simply multiply the color with the AO factor
   * to combine a shaded scene with the AO texture.
   */
  const ref_ptr<Texture>& aoTexture() const;

  const ref_ptr<ShaderInput1f>& aoSamplingRadius() const;
  const ref_ptr<ShaderInput1f>& aoBias() const;
  /**
   * AO is attenuated similar to how lights are attenuated in FFP.
   */
  const ref_ptr<ShaderInput2f>& aoAttenuation() const;
  const ref_ptr<ShaderInput1f>& blurSigma() const;
  const ref_ptr<ShaderInput1f>& blurNumPixels() const;

protected:
  ref_ptr<FilterSequence> filter_;
  ref_ptr<ShaderInput1f> blurSigma_;
  ref_ptr<ShaderInput1f> blurNumPixels_;
  ref_ptr<ShaderInput1f> aoSamplingRadius_;
  ref_ptr<ShaderInput1f> aoBias_;
  ref_ptr<ShaderInput2f> aoAttenuation_;
  GLfloat sizeScale_;
};

} // end ogle namespace

#endif /* __SHADING_AMBIENT_OCCLUSION_H_ */
