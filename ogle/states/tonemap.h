/*
 * tonemap.h
 *
 *  Created on: 13.02.2013
 *      Author: daniel
 */

#ifndef TONEMAP_H_
#define TONEMAP_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

namespace ogle {

class Tonemap : public State
{
public:
  Tonemap(
      const ref_ptr<Texture> &input,
      const ref_ptr<Texture> &blurInput);

  void createShader(ShaderConfig &cfg);

  const ref_ptr<ShaderInput1f>& blurAmount() const;
  const ref_ptr<ShaderInput1f>& effectAmount() const;
  const ref_ptr<ShaderInput1f>& exposure() const;
  const ref_ptr<ShaderInput1f>& gamma() const;
  const ref_ptr<ShaderInput1f>& radialBlurSamples() const;
  const ref_ptr<ShaderInput1f>& radialBlurStartScale() const;
  const ref_ptr<ShaderInput1f>& radialBlurScaleMul() const;
  const ref_ptr<ShaderInput1f>& vignetteInner() const;
  const ref_ptr<ShaderInput1f>& vignetteOuter() const;

protected:
  ref_ptr<ShaderState> shader_;

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

} // end ogle namespace

#endif /* TONEMAP_H_ */
