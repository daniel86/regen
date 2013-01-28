/*
 * rain-particles.h
 *
 *  Created on: 27.01.2013
 *      Author: daniel
 */

#ifndef RAIN_PARTICLES_H_
#define RAIN_PARTICLES_H_

#include <ogle/meshes/particles.h>

class RainParticles : public ParticleState
{
public:
  RainParticles(GLuint numRainDrops);

  void createShader(ShaderConfig &shaderCfg);

  /**
   * Influences size of individual rain drop strands.
   */
  const ref_ptr<ShaderInput2f>& strandSize() const;
  /**
   * Influences area (height+radius) relative to camera where rain drops
   * are emitted.
   */
  const ref_ptr<ShaderInput3f>& cloudPosition() const;
  const ref_ptr<ShaderInput2f>& dropMass() const;
  const ref_ptr<ShaderInput1i>& maxNumParticleEmits() const;

protected:
  ref_ptr<ShaderInput2f> dropMass_;
  ref_ptr<ShaderInput3f> cloudPosition_;
  ref_ptr<ShaderInput1f> cloudRadius_;
  ref_ptr<ShaderInput1i> maxNumParticleEmits_;

  ref_ptr<ShaderInput2f> strandSize_;

  ref_ptr<ShaderInput3f> posInput_;
  ref_ptr<ShaderInput3f> velocityInput_;
  ref_ptr<ShaderInput1f> massInput_;
  ref_ptr<ShaderInput1f> lifetimeInput_;
};

#endif /* RAIN_PARTICLES_H_ */
