/*
 * fire-texture.cpp
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <stdlib.h>

#include "spectral-texture.h"
#include <ogle/utility/spectrum.h>

SpectralTexture::SpectralTexture()
: Texture1D(),
  t1_(0.0),
  t2_(0.0)
{
  pixelType_ = GL_UNSIGNED_BYTE;
  border_ = 0;
}

GLdouble SpectralTexture::t1() const
{
  return t1_;
}
GLdouble SpectralTexture::t2() const
{
  return t2_;
}

void SpectralTexture::set_spectrum(
    GLdouble t1,
    GLdouble t2,
    GLint numTexels,
    GLenum mipmapFlag,
    GLboolean useMipmap)
{
  unsigned char* data = (unsigned char*) malloc(numTexels*4);

  t1_ = t1;
  t2_ = t2;
  spectrum(t1, t2, numTexels, data);

  internalFormat_ = GL_RGBA;
  format_ = GL_RGBA;
  width_ = numTexels;
  height_ = 1;
  data_ = data;

  bind();
  texImage();
  set_filter(GL_LINEAR, GL_LINEAR);
  set_wrapping(GL_CLAMP);

  free(data);
  data_ = NULL;
}
