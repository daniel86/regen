/*
 * distance-fog.h
 *
 *  Created on: 07.02.2013
 *      Author: daniel
 */

#ifndef DISTANCE_FOG_H_
#define DISTANCE_FOG_H_

#include <ogle/states/state.h>
#include <ogle/states/shader-state.h>
#include <ogle/states/texture-state.h>

class DistanceFog : public State
{
public:
  DistanceFog();

  void createShader(ShaderConfig &cfg);

  void set_gDepthTexture(const ref_ptr<Texture> &t);
  void set_tBuffer(const ref_ptr<Texture> &color, const ref_ptr<Texture> &depth);
  void set_skyColor(const ref_ptr<TextureCube> &t);

  const ref_ptr<ShaderInput3f>& fogColor() const;
  const ref_ptr<ShaderInput1f>& fogStart() const;
  const ref_ptr<ShaderInput1f>& fogEnd() const;
  const ref_ptr<ShaderInput1f>& fogDensity() const;

protected:
  ref_ptr<ShaderState> fogShader_;

  ref_ptr<ShaderInput3f> fogColor_;
  ref_ptr<ShaderInput1f> fogStart_;
  ref_ptr<ShaderInput1f> fogEnd_;
  ref_ptr<ShaderInput1f> fogDensity_;

  ref_ptr<TextureState> skyColorTexture_;
  ref_ptr<TextureState> tDepthTexture_;
  ref_ptr<TextureState> tColorTexture_;
  ref_ptr<TextureState> gDepthTexture_;
};

#endif /* DISTANCE_FOG_H_ */
