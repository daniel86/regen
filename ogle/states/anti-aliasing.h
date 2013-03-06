/*
 * anti-aliasing.h
 *
 *  Created on: 16.02.2013
 *      Author: daniel
 */

#ifndef ANTI_ALIASING_H_
#define ANTI_ALIASING_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

namespace ogle {

class AntiAliasing : public State
{
public:
  AntiAliasing(const ref_ptr<Texture> &input);

  void createShader(ShaderConfig &cfg);

  const ref_ptr<ShaderInput1f>& spanMax() const;
  const ref_ptr<ShaderInput1f>& reduceMul() const;
  const ref_ptr<ShaderInput1f>& reduceMin() const;
  const ref_ptr<ShaderInput3f>& luma() const;

protected:
  ref_ptr<ShaderState> shader_;

  ref_ptr<Texture> input_;

  ref_ptr<ShaderInput1f> spanMax_;
  ref_ptr<ShaderInput1f> reduceMul_;
  ref_ptr<ShaderInput1f> reduceMin_;
  ref_ptr<ShaderInput3f> luma_;
};

} // end ogle namespace

#endif /* ANTI_ALIASING_H_ */
