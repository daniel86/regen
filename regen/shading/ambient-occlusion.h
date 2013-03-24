/*
 * ambient-occlusion.h
 *
 *  Created on: 25.02.2013
 *      Author: daniel
 */

#ifndef __SHADING_AMBIENT_OCCLUSION_H_
#define __SHADING_AMBIENT_OCCLUSION_H_

#include <regen/states/state.h>
#include <regen/states/filter.h>

namespace regen {
/**
 * \brief Computes dynamic ambient occlusion.
 *
 * The AO is computed on a downsampled render target and blurred with
 * a separable blur afterwards.
 * @see http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter14.html
 */
class AmbientOcclusion : public FilterSequence
{
public:
  /**
   * @param input the input depth texture.
   * @param sizeScale size scale of input texture.
   */
  AmbientOcclusion(const ref_ptr<Texture> &input, GLfloat sizeScale);

  /**
   * @return the AO output.
   */
  const ref_ptr<Texture>& aoTexture() const;
  /**
   * @return the AO sampling radius.
   */
  const ref_ptr<ShaderInput1f>& aoSamplingRadius() const;
  /**
   * @return the AO bias.
   */
  const ref_ptr<ShaderInput1f>& aoBias() const;
  /**
   * @return attenuation similar to how lights are attenuated in FFP.
   */
  const ref_ptr<ShaderInput2f>& aoAttenuation() const;
  /**
   * @return blur sigma.
   */
  const ref_ptr<ShaderInput1f>& blurSigma() const;
  /**
   * @return blur width.
   */
  const ref_ptr<ShaderInput1f>& blurNumPixels() const;

protected:
  ref_ptr<ShaderInput1f> blurSigma_;
  ref_ptr<ShaderInput1f> blurNumPixels_;
  ref_ptr<ShaderInput1f> aoSamplingRadius_;
  ref_ptr<ShaderInput1f> aoBias_;
  ref_ptr<ShaderInput2f> aoAttenuation_;
  GLfloat sizeScale_;
};
} // namespace

#endif /* __SHADING_AMBIENT_OCCLUSION_H_ */
