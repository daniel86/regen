/*
 * cube-image-texture.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _CUBE_IMAGE_TEXTURE_H_
#define _CUBE_IMAGE_TEXTURE_H_

#include <ogle/gl-types/texture.h>
#include <ogle/textures/image-texture.h>

/**
 * Texture with exactly 6 distinct sets of 2D images,
 * all of the same size. They act as 6 faces of a cube.
 */
class CubeImageTexture : public TextureCube
{
public:
  CubeImageTexture();

  CubeImageTexture(
      const string &filePath,
      GLenum internalFormat=GL_NONE,
      GLboolean flipBackFace=GL_FALSE)
  throw (ImageError, FileNotFoundException);

  void set_filePath(
      const string &filePath,
      GLenum internalFormat=GL_NONE,
      GLboolean flipBackFace=GL_FALSE)
  throw (ImageError, FileNotFoundException);

protected:
  static GLboolean devilInitialized_;
private:
  CubeImageTexture(const CubeImageTexture&);
};

#endif /* _CUBE_IMAGE_TEXTURE_H_ */
