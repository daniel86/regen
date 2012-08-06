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

FireTexture::FireTexture() : Texture2D()
{
  pixelType_ = GL_UNSIGNED_BYTE;
  border_ = 0;
}

FireTexture::~FireTexture()
{
}

void FireTexture::set_spectrum(
    double t1, double t2, int n,
    GLenum mipmapFlag, bool useMipmap)
{
  unsigned char* data = (unsigned char*) malloc(n*4);

  spectrum(t1, t2, n, data);

  internalFormat_ = GL_RGBA;
  format_ = GL_RGBA;
  width_ = n;
  height_ = 1;
  data_ = data;

  bind();
  texImage();
  //if(useMipmap) {
  //  setupMipmaps(mipmapFlag);
  //} else {
    set_filter(GL_LINEAR, GL_LINEAR);
  //}

  set_wrapping(GL_CLAMP);

  free(data);
  data_ = NULL;
}
