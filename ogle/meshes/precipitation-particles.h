/*
 * precipitation-particles.h
 *
 *  Created on: 30.01.2013
 *      Author: daniel
 */

#ifndef PRECIPITATION_PARTICLES_H_
#define PRECIPITATION_PARTICLES_H_

#include <ogle/meshes/particles.h>

namespace ogle {

class PrecipitationParticles : public ParticleState
{
public:
  /**
   * Defines how to interpret the cloud position input.
   */
  enum CloudPositionMode {
    ABSOLUTE, CAMERA_RELATIVE
  };

  PrecipitationParticles(GLuint numParticles, BlendMode blendMode);

  /**
   * Defines how to interpret the cloud position input.
   */
  void set_cloudPositionMode(CloudPositionMode v);
  /**
   * Defines how to interpret the cloud position input.
   */
  CloudPositionMode cloudPositionMode() const;

  void createShader(ShaderConfig &shaderCfg, const string &drawShader);

  void set_particleTexture(const ref_ptr<Texture> &tex);

  /**
   * base mass + mass variance for each particle
   */
  const ref_ptr<ShaderInput2f>& particleMass() const;
  /**
   * base size + size variance for each particle
   */
  const ref_ptr<ShaderInput2f>& particleSize() const;
  /**
   * particles with y pos below die
   */
  const ref_ptr<ShaderInput1f>& surfaceHeight() const;
  /**
   * emitter position
   */
  const ref_ptr<ShaderInput1f>& cloudRadius() const;
  /**
   * radius for emitting
   */
  const ref_ptr<ShaderInput3f>& cloudPosition() const;

protected:
  CloudPositionMode cloudPositionMode_;
  ref_ptr<ShaderInput3f> cloudPosition_;
  ref_ptr<ShaderInput1f> cloudRadius_;
  ref_ptr<ShaderInput1f> surfaceHeight_;

  ref_ptr<ShaderInput2f> particleMass_;
  ref_ptr<ShaderInput2f> particleSize_;

  ref_ptr<ShaderInput3f> posInput_;
  ref_ptr<ShaderInput3f> velocityInput_;
  ref_ptr<ShaderInput1f> massInput_;
  ref_ptr<ShaderInput1f> sizeInput_;

  ref_ptr<TextureState> particleTexture_;
};

class RainParticles : public PrecipitationParticles
{
public:
  RainParticles(GLuint numRainDrops, BlendMode blendMode=BLEND_MODE_ADD);

  void loadIntensityTexture(const string &texturePath);

  void createShader(ShaderConfig &shaderCfg);

  const ref_ptr<ShaderInput2f>& streakSize() const;

protected:
  ref_ptr<ShaderInput2f> streakSize_;
};

class SnowParticles : public PrecipitationParticles
{
public:
  SnowParticles(GLuint numSnowFlakes, BlendMode blendMode=BLEND_MODE_ADD);
  void createShader(ShaderConfig &shaderCfg);
};

} // end ogle namespace

#endif /* PRECIPITATION_PARTICLES_H_ */
