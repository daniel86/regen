/*
 * volume-texture.cpp
 *
 *  Created on: 10.03.2012
 *      Author: daniel
 */

#include <cstdlib>
#include <iostream>
#include <fstream>

#include "volume-texture.h"

#include <ogle/utility/perlin.h>
#include <ogle/utility/string-util.h>

inline float rnd(float min, float max) {
  return min + (max - min) * ((float)rand())/((float)RAND_MAX);
}

RAWTexture3D::RAWTexture3D()
: Texture3D()
{
}

void RAWTexture3D::loadRAWFile(const RAWTextureFile &raw)
throw (FileNotFoundException)
{
  ifstream f(raw.path.c_str(),
      ios::in
      |ios::binary
      |ios::ate // start at end position
      );
  if (!f.is_open()) {
    throw FileNotFoundException(FORMAT_STRING(
        "Unable to open data set file at '" << raw.path << "'."));
  }

  GLuint size = raw.width*raw.height*raw.depth*raw.numComponents;
  char* pixels = new char[size];

  size_t length = (size_t) f.tellg();
  f.seekg (0, ios::beg);
  f.read(pixels, size);
  f.close();

  if(raw.numComponents == 1) {
    format_ = GL_LUMINANCE;
    if(raw.bytesPerComponent == 8) {
      internalFormat_ = GL_R8;
    } else if(raw.bytesPerComponent == 16) {
      internalFormat_ = GL_R16;
    }
  } else if(raw.numComponents == 2) {
    format_ = GL_RG;
    if(raw.bytesPerComponent == 8) {
      internalFormat_ = GL_RG8;
    } else if(raw.bytesPerComponent == 16) {
      internalFormat_ = GL_RG16;
    }
  } else if(raw.numComponents == 3) {
    format_ = GL_RGB;
    if(raw.bytesPerComponent == 8) {
      internalFormat_ = GL_RGB8;
    } else if(raw.bytesPerComponent == 16) {
      internalFormat_ = GL_RGB16;
    }
  } else if(raw.numComponents == 4) {
    format_ = GL_RGBA;
    if(raw.bytesPerComponent == 8) {
      internalFormat_ = GL_RGBA8;
    } else if(raw.bytesPerComponent == 16) {
      internalFormat_ = GL_RGBA16;
    }
  }
  pixelType_ = GL_UNSIGNED_BYTE;
  width_ = raw.width;
  height_ = raw.height;
  numTextures_ = raw.depth;
  data_ = pixels;

  bind();
  set_filter(GL_LINEAR, GL_LINEAR);
  set_wrappingU(GL_CLAMP_TO_EDGE);
  set_wrappingV(GL_CLAMP_TO_EDGE);
  set_wrappingW(GL_CLAMP_TO_EDGE);
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
