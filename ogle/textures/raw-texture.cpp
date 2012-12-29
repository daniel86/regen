/*
 * raw-texture.cpp
 *
 *  Created on: 29.12.2012
 *      Author: daniel
 */

#include <cstdlib>
#include <iostream>
#include <fstream>

#include "raw-texture.h"

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

  texImage();

  delete [] pixels;
}

