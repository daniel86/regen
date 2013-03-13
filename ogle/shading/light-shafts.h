/*
 * light-shafts.h
 *
 *  Created on: 07.02.2013
 *      Author: daniel
 */

#ifndef LIGHT_SHAFTS_H_
#define LIGHT_SHAFTS_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/shading/light-state.h>

namespace ogle {

class SkyLightShaft : public State
{
public:
  SkyLightShaft(
      const ref_ptr<Light> &sun,
      const ref_ptr<Texture> &colorTexture,
      const ref_ptr<Texture> &depthTexture);
  void createShader(ShaderState::Config &cfg);

  const ref_ptr<ShaderInput1f>& scatteringDensity() const;
  const ref_ptr<ShaderInput1f>& scatteringSamples() const;
  const ref_ptr<ShaderInput1f>& scatteringExposure() const;
  const ref_ptr<ShaderInput1f>& scatteringDecay() const;
  const ref_ptr<ShaderInput1f>& scatteringWeight() const;

  virtual void enable(RenderState *rs);

protected:
  ref_ptr<MeshState> mesh_;
  ref_ptr<Light> sun_;

  ref_ptr<ShaderState> shader_;
  GLint lightDirLoc_;

  ref_ptr<ShaderInput1f> scatteringDensity_;
  ref_ptr<ShaderInput1f> scatteringSamples_;
  ref_ptr<ShaderInput1f> scatteringExposure_;
  ref_ptr<ShaderInput1f> scatteringDecay_;
  ref_ptr<ShaderInput1f> scatteringWeight_;
};

} // end ogle namespace

#endif /* LIGHT_SHAFTS_H_ */
