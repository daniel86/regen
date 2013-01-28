/*
 * snow-particles.h
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#ifndef SNOW_PARTICLES_H_
#define SNOW_PARTICLES_H_

#include <ogle/meshes/particles.h>

class SnowParticles : public ParticleState
{
public:
  SnowParticles(GLuint numSnowFlakes);

  void createShader(ShaderConfig &shaderCfg);

  void set_snowFlakeTexture(const ref_ptr<Texture> &tex);
  void set_depthTexture(const ref_ptr<Texture> &tex);

  /**
   * Influences size of individual snow flakes.
   */
  const ref_ptr<ShaderInput2f>& snowFlakeSize() const;
  /**
   * Influences area (height+radius) relative to camera where rain drops
   * are emitted.
   */
  const ref_ptr<ShaderInput3f>& cloudPosition() const;
  const ref_ptr<ShaderInput2f>& snowFlakeMass() const;
  const ref_ptr<ShaderInput1i>& maxNumParticleEmits() const;

protected:
  ref_ptr<ShaderInput2f> flakeMass_;
  ref_ptr<ShaderInput3f> cloudPosition_;
  ref_ptr<ShaderInput1f> cloudRadius_;
  ref_ptr<ShaderInput1i> maxNumParticleEmits_;

  ref_ptr<ShaderInput2f> snowFlakeSize_;
  ref_ptr<TextureState> snowFlakeTexture_;
  ref_ptr<TextureState> depthTexture_;

  ref_ptr<ShaderInput3f> posInput_;
  ref_ptr<ShaderInput3f> velocityInput_;
  ref_ptr<ShaderInput1f> massInput_;
  ref_ptr<ShaderInput1f> lifetimeInput_;
};

#endif /* SNOW_PARTICLES_H_ */
