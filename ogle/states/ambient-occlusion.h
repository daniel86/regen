/*
 * ambient-occlusion.h
 *
 *  Created on: 07.02.2013
 *      Author: daniel
 */

#ifndef AMBIENT_OCCLUSION_H_
#define AMBIENT_OCCLUSION_H_

#include <ogle/states/state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/shader-state.h>

class SSAO : public State
{
public:
  SSAO();

  void set_norWorldTexture(const ref_ptr<Texture>&);
  void set_depthTexture(const ref_ptr<Texture>&);

  void createShader(ShaderConfig &cfg);

  const ref_ptr<ShaderInput1f>& aoSampleRad() const;
  const ref_ptr<ShaderInput1f>& aoBias() const;
  const ref_ptr<ShaderInput1f>& aoConstAttenuation() const;
  const ref_ptr<ShaderInput1f>& aoLinearAttenuation() const;

protected:
  ref_ptr<ShaderState> aoShader_;

  ref_ptr<TextureState> norWorldTexture_;
  ref_ptr<TextureState> depthTexture_;

  ref_ptr<ShaderInput1f> aoSampleRad_;
  ref_ptr<ShaderInput1f> aoBias_;
  ref_ptr<ShaderInput1f> aoConstAttenuation_;
  ref_ptr<ShaderInput1f> aoLinearAttenuation_;
};

#endif /* AMBIENT_OCCLUSION_H_ */
