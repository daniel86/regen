/*
 * fire-texture.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _FIRE_TEXTURE_H_
#define _FIRE_TEXTURE_H_

#include <ogle/gl-types/texture.h>

class FireTexture : public Texture2D {
public:
  /**
   * Default constructor, does not load image data.
   */
  FireTexture();
  ~FireTexture();

  void set_spectrum(
      double t1, double t2, int n,
      GLenum mimpmapFlag=GL_DONT_CARE,
      bool useMipmap=true);
private:
  FireTexture(const FireTexture&);
};

#endif /* _FIRE_TEXTURE_H_ */
