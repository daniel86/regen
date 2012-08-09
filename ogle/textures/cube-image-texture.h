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
class CubeImageTexture : public CubeMapTexture
{
public:
  CubeImageTexture();

  CubeImageTexture(
      const string &filePath,
      const string &fileExtension="png")
  throw (ImageError, FileNotFoundException);

  void set_filePath(
      const string &filePath,
      const string &fileExtension="png",
      GLenum mimpmapFlag=GL_DONT_CARE)
  throw (ImageError, FileNotFoundException);

protected:
  static GLboolean devilInitialized_;
private:
  CubeImageTexture(const CubeImageTexture&);
};

#endif /* _CUBE_IMAGE_TEXTURE_H_ */
