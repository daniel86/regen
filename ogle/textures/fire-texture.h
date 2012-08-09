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
class FireTexture : public Texture1D
{
public:
  FireTexture();

  void set_spectrum(
      GLdouble t1,
      GLdouble t2,
      GLint numTexels,
      GLenum mimpmapFlag=GL_DONT_CARE,
      GLboolean useMipmap=true);
};

#endif /* _FIRE_TEXTURE_H_ */
