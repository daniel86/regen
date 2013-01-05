/*
 * noise-texture.cpp
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include "noise-texture.h"
#include <ogle/external/perlin.h>

NoiseTexture2D::NoiseTexture2D(GLuint width, GLuint height)
: Texture2D()
{
  char* pixels = new char[width * height];

  pixelType_ = GL_UNSIGNED_BYTE;
  internalFormat_ = GL_R8;
  format_ = GL_LUMINANCE;
  width_ = width;
  height_ = height;
  data_ = pixels;

  char* pDest = pixels;
  for (GLuint i=0u; i < width * height; i++) {
      *pDest++ = rand() % 256;
  }

  bind();
  set_filter(GL_NEAREST, GL_NEAREST);
  set_wrapping(GL_REPEAT);
  texImage();

  delete [] pixels;
}

//////////

PyroclasticVolume::PyroclasticVolume(int n, float r)
: Texture3D()
{
  GLuint size = n*n*n;
  char* pixels = new char[size];

  float frequency = 3.0f / n;
  float center = n / 2.0f + 0.5f;

  char *ptr = pixels;
  for(int x=0; x < n; x++)
  {
    for (int y=0; y < n; ++y)
    {
      for (int z=0; z < n; ++z)
      {
        float dx = center-x;
        float dy = center-y;
        float dz = center-z;

        float off = fabsf(perlinNoise3D(
            x*frequency, y*frequency, z*frequency, 5, 6, 3));

        float d = sqrtf(dx*dx+dy*dy+dz*dz)/(n);
        *ptr++ = ((d-off) < r)?255:0;
      }
    }
  }

  pixelType_ = GL_UNSIGNED_BYTE;
  format_ = GL_LUMINANCE;
  internalFormat_ = GL_LUMINANCE;
  width_ = n;
  height_ = n;
  numTextures_ = n;
  data_ = pixels;

  bind();
  set_filter(GL_LINEAR, GL_LINEAR);
  set_wrappingU(GL_CLAMP_TO_BORDER);
  set_wrappingV(GL_CLAMP_TO_BORDER);
  set_wrappingW(GL_CLAMP_TO_BORDER);
  texImage();

  delete [] pixels;
}
