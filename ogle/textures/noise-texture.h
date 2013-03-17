/*
 * noise-texture.h
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#ifndef NOISE_TEXTURE_H_
#define NOISE_TEXTURE_H_

#include <libnoise/noise.h>
#include <ogle/gl-types/texture.h>
#include <ogle/utility/ref-ptr.h>

namespace ogle {
/**
 * \brief Loads procedural textures using coherent noise.
 *
 * Coherent noise means that for any two points in the space,
 * the value of the noise function changes smoothly as you
 * move from one point to the other -- that is, there are no discontinuities.
 */
class NoiseTextureLoader
{
public:
  /**
   * Configures Perlin noise function for generating coherent noise.
   */
  struct PerlinNoiseConfig {
    /**
     * Sets the frequency of the first octave.
     */
    GLdouble baseFrequency;
    /**
     * The persistence value controls the roughness of the Perlin noise.
     * For best results, set the persistence to a number between 0.0 and 1.0.
     */
    GLdouble persistence;
    /**
     * The lacunarity is the frequency multiplier between successive octaves.
     * For best results, set the lacunarity to a number between 1.5 and 3.5.
     */
    GLdouble lacunarity;
    /**
     * The number of octaves controls the amount of detail in the Perlin noise.
     * The larger the number of octaves, the more time required to calculate the Perlin-noise value.
     */
    GLuint octaveCount;
    PerlinNoiseConfig();
  };

  /**
   * 2D Image of noise.
   */
  static ref_ptr<Texture2D> noise2D(
      noise::module::Module &noiseGen,
      GLuint width,
      GLuint height,
      GLboolean isSeamless);

  /**
   * 3D Image of noise.
   */
  static ref_ptr<Texture3D> noise3D(
      noise::module::Module &noiseGen,
      GLuint width,
      GLuint height,
      GLuint depth,
      GLboolean isSeamless);

  /**
   * 2D Image of Perlin noise.
   */
  static ref_ptr<Texture2D> perlin2D(
      GLuint width, GLuint height,
      const PerlinNoiseConfig &cfg,
      GLint randomSeed=0,
      GLboolean isSeamless=GL_FALSE);

  /**
   * 3D Image of Perlin noise.
   */
  static ref_ptr<Texture3D> perlin3D(
      GLuint width, GLuint height, GLuint depth,
      const PerlinNoiseConfig &cfg,
      GLint randomSeed=0,
      GLboolean isSeamless=GL_FALSE);

  /**
   * Generates 2D texture map consisting of clouds of varying density.
   */
  static ref_ptr<Texture2D> clouds2D(
      GLuint width, GLuint height,
      GLint randomSeed, GLboolean isSeamless);

  /**
   * Generates 2D texture map consisting of stained oak-like wood.
   */
  static ref_ptr<Texture2D> wood(
      GLuint width, GLuint height,
      GLint randomSeed, GLboolean isSeamless);

  /**
   * Generates 2D texture map consisting of granite.
   */
  static ref_ptr<Texture2D> granite(
      GLuint width, GLuint height,
      GLint randomSeed, GLboolean isSeamless);
};
} // end ogle namespace

#endif /* NOISE_TEXTURE_H_ */
