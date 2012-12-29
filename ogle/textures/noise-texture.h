/*
 * noise-texture.h
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#ifndef NOISE_TEXTURE_H_
#define NOISE_TEXTURE_H_

#include <ogle/gl-types/texture.h>

class NoiseTexture2D : public Texture2D
{
public:
  NoiseTexture2D(GLuint width, GLuint height);
};

class PyroclasticVolume : public Texture3D
{
public:
  // create a volume texture with n^3 texels and base radius r
  PyroclasticVolume(int n, float r);
};

#endif /* NOISE_TEXTURE_H_ */
