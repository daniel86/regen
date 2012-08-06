/*
 * gl-image-texture.cpp
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include <unistd.h>

#include "image-texture.h"

#include <ogle/gl-types/fbo.h>
#include <ogle/utility/string-util.h>

bool ImageTexture::devilInitialized_ = false;

ImageTexture::ImageTexture() : Texture()
{
  init();
}
ImageTexture::ImageTexture(const string &file, bool useMipmap)
throw (ImageError, FileNotFoundException)
: Texture()
{
  init();
  set_file(file, 0, 0, 0, useMipmap);
}
ImageTexture::ImageTexture(const string &file,
    int width, int height, int depth,
    bool useMipmap)
throw (ImageError, FileNotFoundException)
: Texture()
{
  init();
  set_file(file, width, height, depth, useMipmap);
}

ImageTexture::~ImageTexture()
{
}

void ImageTexture::init()
{
  targetType_ = GL_TEXTURE_2D;
  pixelType_ = GL_UNSIGNED_BYTE;
  border_ = 0;
  // Initialize I(mage)L(oading)
  if(!devilInitialized_) {
    ilInit();
    devilInitialized_ = true;
  }
}

void ImageTexture::set_file(const string &file,
    int width, int height, int depth,
    bool useMipmap, GLenum mipmapFlag)
throw (ImageError, FileNotFoundException)
{
  if(access(file.c_str(), F_OK) != 0) {
    throw FileNotFoundException(FORMAT_STRING(
        "Unable to open image file at '" << file << "'."));
  }

  GLuint ilID;
  ilGenImages(1, &ilID);

  ilBindImage(ilID);
  if(ilLoadImage(file.c_str()) == IL_FALSE) {
    throw ImageError("ilLoadImage failed");
  }

  if(ilConvertImage(
      ilGetInteger(IL_IMAGE_FORMAT),
      IL_UNSIGNED_BYTE) == IL_FALSE)
  {
    throw ImageError("ilConvertImage failed");
  }

  width_ = ilGetInteger(IL_IMAGE_WIDTH);

  // scale image to desired size
  if(width>0 && height>0) {
    if(width_ > width) {
      // use bilinear filtering for down scaling
      iluImageParameter(ILU_FILTER, ILU_BILINEAR);
    } else {
      // use triangle filtering for up scaling
      iluImageParameter(ILU_FILTER, ILU_SCALE_TRIANGLE);
    }
    iluScale(width, height, depth);
  }

  width_ = ilGetInteger(IL_IMAGE_WIDTH);
  height_ = ilGetInteger(IL_IMAGE_HEIGHT);
  depth_ = ilGetInteger(IL_IMAGE_DEPTH);
  switch(ilGetInteger(IL_IMAGE_FORMAT))
  {
  case IL_ALPHA:
    format_ = GL_ALPHA;
    internalFormat_ = GL_ALPHA;
    break;
  case IL_RGB:
    format_ = GL_RGB;
    internalFormat_ = GL_RGB;
    break;
  case IL_RGBA:
    format_ = GL_RGBA;
    internalFormat_ = GL_RGBA;
    break;
  case IL_BGR:
    format_ = GL_BGR;
    internalFormat_ = GL_BGR;
    break;
  case IL_BGRA:
    format_ = GL_BGRA;
    internalFormat_ = GL_BGRA;
    break;
  case IL_LUMINANCE:
    format_ = GL_LUMINANCE;
    internalFormat_ = GL_LUMINANCE;
    break;
  case IL_LUMINANCE_ALPHA:
    format_ = GL_LUMINANCE_ALPHA;
    internalFormat_ = GL_LUMINANCE_ALPHA;
    break;
  case IL_COLOR_INDEX:
    format_ = GL_COLOR_INDEX;
    internalFormat_ = GL_COLOR_INDEX;
    break;
  }
  internalFormat_ = ilGetInteger(IL_IMAGE_BPP);
  targetType_ = (depth_>1 ? GL_TEXTURE_3D : GL_TEXTURE_2D);
  samplerType_ = (depth_>1 ? "sampler3D" : "sampler2D");

  data_ = (GLubyte*) ilGetData();

  bind();
  texImage();
  if(useMipmap) {
    setupMipmaps(mipmapFlag);
  } else {
    set_filter(GL_LINEAR, GL_LINEAR);
  }

  ilDeleteImages(1, &ilID);
}


void ImageTexture::texSubImage(GLubyte *subData, int layer) const {
  if(targetType_ == GL_TEXTURE_2D) {
    glTexSubImage2D(targetType_,
        0,0,0,
        width_, height_,
        format_,
        pixelType_,
        data_);
  } else {
    glTexSubImage3D(
            targetType_,
            0, 0, 0, layer,
            width_,
            height_,
            1,
            format_,
            pixelType_,
            subData);
  }
}

void ImageTexture::texImage() const {
  if(targetType_ == GL_TEXTURE_2D) {
    glTexImage2D(targetType_,
                 0, // mipmap level
                 internalFormat_,
                 width_,
                 height_,
                 border_,
                 format_,
                 pixelType_,
                 data_);
  } else {
    glTexImage3D(targetType_,
        0, // mipmap level
        internalFormat_,
        width_,
        height_,
        depth_,
        border_,
        format_,
        pixelType_,
        data_);
  }
}

