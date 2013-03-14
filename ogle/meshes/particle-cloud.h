/*
 * particle-cloud.h
 *
 *  Created on: 30.01.2013
 *      Author: daniel
 */

#ifndef PRECIPITATION_PARTICLES_H_
#define PRECIPITATION_PARTICLES_H_

#include <ogle/meshes/particles.h>

namespace ogle {
/**
 * \brief A cloud particle emitter.
 */
class ParticleCloud : public ParticleState
{
public:
  /**
   * Defines how to interpret the cloud position input.
   */
  enum PositionMode {
    ABSOLUTE,
    CAMERA_RELATIVE
  };

  /**
   * @param numParticles particle count.
   * @param blendMode particle blend mode.
   */
  ParticleCloud(GLuint numParticles, BlendMode blendMode);

  /**
   * @param v how to interpret the cloud position input.
   */
  void set_cloudPositionMode(PositionMode v);
  /**
   * @return how to interpret the cloud position input.
   */
  PositionMode cloudPositionMode() const;

  /**
   * Creates the particle update and draw shader.
   * @param shaderCfg the shader configuration
   * @param drawKey include key for draw shader
   */
  void createShader(ShaderState::Config &shaderCfg, const string &drawKey);

  /**
   * @param tex a texture that is applied to each particle.
   */
  void set_particleTexture(const ref_ptr<Texture> &tex);

  /**
   * @return base mass + mass variance for each particle
   */
  const ref_ptr<ShaderInput2f>& particleMass() const;
  /**
   * @return base size + size variance for each particle
   */
  const ref_ptr<ShaderInput2f>& particleSize() const;
  /**
   * @return particles with y pos below die
   */
  const ref_ptr<ShaderInput1f>& surfaceHeight() const;
  /**
   * @return emitter position
   */
  const ref_ptr<ShaderInput1f>& cloudRadius() const;
  /**
   * @return radius for emitting
   */
  const ref_ptr<ShaderInput3f>& cloudPosition() const;

protected:
  PositionMode cloudPositionMode_;
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

/**
 * \brief Rain streak particles emitted from a cloud.
 */
class ParticleRain : public ParticleCloud
{
public:
  /**
   * @param numRainDrops particle count.
   * @param blendMode particle blend mode.
   */
  ParticleRain(GLuint numRainDrops, BlendMode blendMode=BLEND_MODE_ADD);

  /**
   * @param texturePath rain drop texture file.
   */
  void loadIntensityTexture(const string &texturePath);
  /**
   * Creates the particle update and draw shader.
   * @param shaderCfg the shader configuration
   */
  void createShader(ShaderState::Config &shaderCfg);
  /**
   * @return base size + size variance for each rain streak
   */
  const ref_ptr<ShaderInput2f>& streakSize() const;

protected:
  ref_ptr<ShaderInput2f> streakSize_;
};

/**
 * \brief Snow flake particles emitted from a cloud.
 */
class ParticleSnow : public ParticleCloud
{
public:
  /**
   * @param numSnowFlakes particle count.
   * @param blendMode particle blend mode.
   */
  ParticleSnow(GLuint numSnowFlakes, BlendMode blendMode=BLEND_MODE_ADD);

  /**
   * Creates the particle update and draw shader.
   * @param shaderCfg the shader configuration
   */
  void createShader(ShaderState::Config &shaderCfg);
};
} // namespace

#endif /* PRECIPITATION_PARTICLES_H_ */
