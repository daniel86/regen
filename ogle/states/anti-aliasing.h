/*
 * anti-aliasing.h
 *
 *  Created on: 16.02.2013
 *      Author: daniel
 */

#ifndef ANTI_ALIASING_H_
#define ANTI_ALIASING_H_

#include <ogle/states/fullscreen-pass.h>
#include <ogle/states/texture-state.h>

namespace ogle {
/**
 * \brief Fast approximate anti-aliasing (FXAA).
 * @see http://en.wikipedia.org/wiki/Fast_approximate_anti-aliasing
 */
class AntiAliasing : public FullscreenPass
{
public:
  AntiAliasing(const ref_ptr<Texture> &input);

  const ref_ptr<ShaderInput1f>& spanMax() const;
  const ref_ptr<ShaderInput1f>& reduceMul() const;
  const ref_ptr<ShaderInput1f>& reduceMin() const;
  const ref_ptr<ShaderInput3f>& luma() const;

protected:
  ref_ptr<Texture> input_;

  ref_ptr<ShaderInput1f> spanMax_;
  ref_ptr<ShaderInput1f> reduceMul_;
  ref_ptr<ShaderInput1f> reduceMin_;
  ref_ptr<ShaderInput3f> luma_;
};
} // namespace

#endif /* ANTI_ALIASING_H_ */
