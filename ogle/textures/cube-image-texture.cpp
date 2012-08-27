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
#include <stdio.h>
#include <string.h>

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
    GLenum internalFormat,
    GLboolean flipBackFace)
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

  set_filePath(filePath, internalFormat, flipBackFace);
}

void CubeImageTexture::set_filePath(
    const string &filePath,
    GLenum internalFormat,
    GLboolean flipBackFace)
throw (ImageError, FileNotFoundException)
{
  if(access(filePath.c_str(), F_OK) != 0)
  {
    throw FileNotFoundException(FORMAT_STRING(
        "Unable to open image file path at '" << filePath << "'."));
  }

  GLuint ilID=0;
  ilGenImages(1, &ilID);

  ilBindImage(ilID);
  if(ilLoadImage(filePath.c_str()) == IL_FALSE)
  {
    throw ImageError("ilLoadImage failed");
  }

  ILint width = ilGetInteger(IL_IMAGE_WIDTH);
  ILint height = ilGetInteger(IL_IMAGE_HEIGHT);
  ILint faces[12];
  if(width > height) {
    width_ = width/4;
    height_ = height/3;
    ILint faces_[12] = {
           -1,    TOP,   -1,   -1,
        RIGHT,  FRONT, LEFT, BACK,
           -1, BOTTOM,   -1,   -1
    };
    for(ILint i=0; i<12; ++i) faces[i]=faces_[i];
  } else {
    width_ = width/3;
    height_ = height/4;
    ILint faces_[12] = {
          -1,    TOP,    -1,
       RIGHT,  FRONT,  LEFT,
          -1, BOTTOM,    -1,
          -1,   BACK,    -1
    };
    for(ILint i=0; i<12; ++i) faces[i]=faces_[i];
  }
  const ILint numRows = height/height_;
  const ILint numCols = width/width_;
  const ILint bpp = ilGetInteger(IL_IMAGE_BPP);
  const ILint faceBytes = bpp*width_*height_;
  const ILint rowBytes = faceBytes*numCols;

  format_ = ilGetInteger(IL_IMAGE_FORMAT);
  pixelType_ = ilGetInteger(IL_IMAGE_TYPE);
  if(internalFormat==GL_NONE) {
    internalFormat_ = format_;
  } else {
    internalFormat_ = internalFormat;
  }

  GLbyte *imageData = (GLbyte*) ilGetData();

  ILint index = 0;
  for(ILint row=0; row<numRows; ++row)
  {
    GLbyte *colData = imageData;
    for(ILint col=0; col<numCols; ++col)
    {
      ILint mappedFace = faces[index];
      if(mappedFace!=-1) {
        set_data((CubeSide)mappedFace, colData);
      }
      index += 1;
      colData += bpp*width_;
    }
    imageData += rowBytes;
  }
  data_ = NULL;

  bind();

  set_filter(GL_LINEAR, GL_LINEAR);
  glPixelStorei(GL_UNPACK_ROW_LENGTH,width_*numCols);
  cubeTexImage(LEFT);
  cubeTexImage(RIGHT);
  cubeTexImage(TOP);
  cubeTexImage(BOTTOM);
  cubeTexImage(FRONT);

  if(flipBackFace)
  {
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    GLbyte *flippedFace = new GLbyte[faceBytes];
    GLbyte *faceData = (GLbyte*)cubeData_[BACK];
    GLbyte *dst = flippedFace;

    for(ILint row=width_-1; row>=0; --row)
    {
      GLbyte *rowData = faceData + row*width_*numCols*bpp;
      for(ILint col=height_-1; col>=0; --col)
      {
        GLbyte *pixelData = rowData + col*bpp;
        memcpy(dst, pixelData, bpp);
        dst += bpp;
      }
    }

    set_data(BACK, flippedFace);
    cubeTexImage(BACK);
    delete[] flippedFace;
  }
  else
  {
    cubeTexImage(BACK);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  }

  ilDeleteImages(1, &ilID);
}
