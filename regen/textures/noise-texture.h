/*
 * noise-texture.h
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#ifndef NOISE_TEXTURE_H_
#define NOISE_TEXTURE_H_

#include <regen/gl-types/texture.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
  namespace textures {
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
     * 2D Image of Perlin noise.
     */
    ref_ptr<Texture2D> perlin2D(
        GLuint width, GLuint height,
        const PerlinNoiseConfig &cfg,
        GLint randomSeed=0,
        GLboolean isSeamless=GL_FALSE);

    /**
     * 3D Image of Perlin noise.
     */
    ref_ptr<Texture3D> perlin3D(
        GLuint width, GLuint height, GLuint depth,
        const PerlinNoiseConfig &cfg,
        GLint randomSeed=0,
        GLboolean isSeamless=GL_FALSE);

    /**
     * Generates 2D texture map consisting of clouds of varying density.
     */
    ref_ptr<Texture2D> clouds2D(
        GLuint width, GLuint height,
        GLint randomSeed, GLboolean isSeamless);

    /**
     * Generates 2D texture map consisting of stained oak-like wood.
     */
    ref_ptr<Texture2D> wood(
        GLuint width, GLuint height,
        GLint randomSeed, GLboolean isSeamless);

    /**
     * Generates 2D texture map consisting of granite.
     */
    ref_ptr<Texture2D> granite(
        GLuint width, GLuint height,
        GLint randomSeed, GLboolean isSeamless);
  };
} // namespace

#endif /* NOISE_TEXTURE_H_ */
