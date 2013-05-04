/*
 * tonemap.h
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#ifndef TONEMAP_H_
#define TONEMAP_H_

#include <regen/states/fullscreen-pass.h>
#include <regen/states/texture-state.h>

namespace regen {
  /**
   * \brief Implements a tonemap operation.
   *
   * In addition to the bloom and exposure operations,
   * this class implements an effect that approximates streaming rays
   * one would often see emanating from bright objects and
   * a vignette effect.
   */
  class Tonemap : public FullscreenPass
  {
  public:
    /**
     * @param input input texture.
     * @param blurredInput blurred input texture.
     */
    Tonemap(
        const ref_ptr<Texture> &input,
        const ref_ptr<Texture> &blurredInput);

    /**
     * @return mix factor for input and blurred input.
     */
    const ref_ptr<ShaderInput1f>& blurAmount() const;
    /**
     * @return overall exposure factor.
     */
    const ref_ptr<ShaderInput1f>& exposure() const;
    /**
     * @return gamma correction factor.
     */
    const ref_ptr<ShaderInput1f>& gamma() const;
    /**
     * @return streaming rays factor.
     */
    const ref_ptr<ShaderInput1f>& effectAmount() const;
    /**
     * @return number of radial blur samples for streaming rays.
     */
    const ref_ptr<ShaderInput1f>& radialBlurSamples() const;
    /**
     * @return initial scale of texture coordinates for streaming rays.
     */
    const ref_ptr<ShaderInput1f>& radialBlurStartScale() const;
    /**
     * @return scale factor of texture coordinates for streaming rays.
     */
    const ref_ptr<ShaderInput1f>& radialBlurScaleMul() const;
    /**
     * @return inner distance for vignette effect.
     */
    const ref_ptr<ShaderInput1f>& vignetteInner() const;
    /**
     * @return outer distance for vignette effect.
     */
    const ref_ptr<ShaderInput1f>& vignetteOuter() const;

  protected:
    ref_ptr<ShaderInput1f> effectAmount_;
    ref_ptr<ShaderInput1f> blurAmount_;
    ref_ptr<ShaderInput1f> exposure_;
    ref_ptr<ShaderInput1f> gamma_;
    ref_ptr<ShaderInput1f> radialBlurSamples_;
    ref_ptr<ShaderInput1f> radialBlurStartScale_;
    ref_ptr<ShaderInput1f> radialBlurScaleMul_;
    ref_ptr<ShaderInput1f> vignetteInner_;
    ref_ptr<ShaderInput1f> vignetteOuter_;
  };
} // namespace

#endif /* TONEMAP_H_ */
