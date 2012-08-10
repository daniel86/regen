/*
 * fire-texture.cpp
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>

#include <stdlib.h>

#include "fire-texture.h"
#include <ogle/utility/spectrum.h>

FireTexture::FireTexture()
: Texture1D()
{
  pixelType_ = GL_UNSIGNED_BYTE;
  border_ = 0;
}

void FireTexture::set_spectrum(
    GLdouble t1,
    GLdouble t2,
    GLint numTexels,
    GLenum mipmapFlag,
    GLboolean useMipmap)
{
  unsigned char* data = (unsigned char*) malloc(numTexels*4);

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
