/*
 * gl-image-texture.cpp
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>
#include <IL/il.h>

#include <unistd.h>

#include "cube-image-texture.h"
#include <ogle/gl-types/fbo.h>
#include <ogle/utility/string-util.h>

GLboolean CubeImageTexture::devilInitialized_ = false;

CubeImageTexture::CubeImageTexture()
: CubeMapTexture()
{
  pixelType_ = GL_UNSIGNED_BYTE;
  border_ = 0;

  // Initialize I(mage)L(oading)
  if(!devilInitialized_) {
    ilInit();
    devilInitialized_ = true;
  }
}

CubeImageTexture::CubeImageTexture(
    const string &filePath,
    const string &fileExtension)
throw (ImageError, FileNotFoundException)
: CubeMapTexture()
{
  pixelType_ = GL_UNSIGNED_BYTE;
  border_ = 0;

  // Initialize I(mage)L(oading)
  if(!devilInitialized_) {
    ilInit();
    devilInitialized_ = true;
  }

  set_filePath(filePath, fileExtension);
}

void CubeImageTexture::set_filePath(
    const string &filePath,
    const string &fileExtension,
    GLenum mimpmapFlag)
throw (ImageError, FileNotFoundException)
{
  if(access(filePath.c_str(), F_OK) != 0)
  {
    throw FileNotFoundException(FORMAT_STRING(
        "Unable to open image file path at '" << filePath << "'."));
  }

  GLuint ilID=0;
  ilGenImages(6, &ilID);

  for(CubeSide side=(CubeSide)0;
      side<=(CubeSide)5;
      side=(CubeSide)((int)side +1))
  {
    string sideImage;
    switch(side) {
    case FRONT: sideImage = "front"; break;
    case BACK: sideImage = "back"; break;
    case LEFT: sideImage = "left"; break;
    case RIGHT: sideImage = "right"; break;
    case TOP: sideImage = "top"; break;
    case BOTTOM: sideImage = "bottom"; break;
    }

    ilBindImage(ilID + (int)side);

    string imagePath = filePath + "/" + sideImage + "." + fileExtension;

    if(access(imagePath.c_str(), F_OK) != 0)
    {
      throw FileNotFoundException(FORMAT_STRING(
          "Unable to open image file at '" << imagePath << "'."));
    }
    if(ilLoadImage(imagePath.c_str()) == IL_FALSE)
    {
      throw ImageError("ilLoadImage failed");
    }
    if(ilConvertImage(IL_RGB, IL_UNSIGNED_BYTE) == IL_FALSE)
    {
      throw ImageError("ilConvertImage failed");
    }
    internalFormat_ = ilGetInteger(IL_IMAGE_BPP);
    width_ = ilGetInteger(IL_IMAGE_WIDTH);
    height_ = ilGetInteger(IL_IMAGE_HEIGHT);
    format_ = ilGetInteger(IL_IMAGE_FORMAT);

    set_data(side, (GLubyte*) ilGetData());
  }
  data_ = NULL;

  bind();
  texImage();
  if(useMipmaps()) {
    setupMipmaps(mimpmapFlag);
  }

  ilDeleteImages(6, &ilID);
}
