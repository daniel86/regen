/*
 * fire-texture.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _FIRE_TEXTURE_H_
#define _FIRE_TEXTURE_H_

#include <ogle/gl-types/texture.h>

/**
 * 1 dimensional texture that loads texels from spectrum.
 */
class SpectralTexture : public Texture1D
{
public:
  SpectralTexture();

  GLdouble t1() const;
  GLdouble t2() const;
  void set_spectrum(
      GLdouble t1,
      GLdouble t2,
      GLint numTexels,
      GLenum mimpmapFlag=GL_DONT_CARE,
      GLboolean useMipmap=true);
protected:
  GLdouble t1_;
  GLdouble t2_;
};

#endif /* _FIRE_TEXTURE_H_ */
