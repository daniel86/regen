/*
 * texture-loader.cpp
 *
 *  Created on: 21.12.2012
 *      Author: daniel
 */

#include <GL/glew.h>
#include <GL/gl.h>
#include <IL/il.h>
#include <IL/ilu.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <ogle/utility/string-util.h>

#include "texture-loader.h"

static GLenum ilToGLFormat()
{
  switch(ilGetInteger(IL_IMAGE_FORMAT))
  {
  case IL_ALPHA:
    return GL_ALPHA;
  case IL_RGB:
    return GL_RGB;
  case IL_RGBA:
    return GL_RGBA;
  case IL_BGR:
    return GL_BGR;
  case IL_BGRA:
    return GL_BGRA;
  case IL_LUMINANCE:
    return GL_LUMINANCE;
  case IL_LUMINANCE_ALPHA:
    return GL_LUMINANCE_ALPHA;
  case IL_COLOR_INDEX:
    return GL_COLOR_INDEX;
  default:
    return ilGetInteger(IL_IMAGE_FORMAT);
  }
}

static void scaleImage(GLuint w, GLuint h, GLuint d)
{
  GLuint width_ = ilGetInteger(IL_IMAGE_WIDTH);
  // scale image to desired size
  if(w>0 && h>0) {
    if(width_ > w) {
      // use bilinear filtering for down scaling
      iluImageParameter(ILU_FILTER, ILU_BILINEAR);
    } else {
      // use triangle filtering for up scaling
      iluImageParameter(ILU_FILTER, ILU_SCALE_TRIANGLE);
    }
    iluScale(w,h,d);
  }
}

static void convertImage(GLenum convertTarget)
{
  if(convertTarget!=GL_NONE && ilConvertImage(
      ilGetInteger(IL_IMAGE_FORMAT),
      convertTarget) == IL_FALSE)
  {
    throw ImageError("ilConvertImage failed");
  }
}

static GLuint loadImage(const string &file)
{
  static GLboolean devilInitialized_ = GL_FALSE;
  if(!devilInitialized_) {
    ilInit();
    devilInitialized_ = GL_TRUE;
  }

  if(access(file.c_str(), F_OK) != 0)
  {
    throw FileNotFoundException(FORMAT_STRING(
        "Unable to open image file at '" << file << "'."));
  }

  GLuint ilID;
  ilGenImages(1, &ilID);
  ilBindImage(ilID);
  if(ilLoadImage(file.c_str()) == IL_FALSE) {
    throw ImageError("ilLoadImage failed");
  }

  return ilID;
}

ref_ptr<Texture> TextureLoader::load(
    const string &file,
    GLenum mipmapFlag,
    GLenum forcedFormat,
    const Vec3ui &forcedSize)
{
  GLuint ilID = loadImage(file);
  convertImage(IL_UNSIGNED_BYTE);
  scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
  GLint depth = ilGetInteger(IL_IMAGE_DEPTH);

  ref_ptr<Texture> tex;
  if(depth>1) {
    ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::manage(new Texture3D);
    tex3D->set_numTextures(depth);
    tex = ref_ptr<Texture>::cast(tex3D);
  }
  else {
    tex = ref_ptr<Texture>::manage(new Texture2D);
  }
  tex->bind();
  tex->set_size(
      ilGetInteger(IL_IMAGE_WIDTH),
      ilGetInteger(IL_IMAGE_HEIGHT));
  tex->set_pixelType(GL_UNSIGNED_BYTE);
  tex->set_format(ilToGLFormat());
  tex->set_internalFormat(forcedFormat==GL_NONE ? tex->format() : forcedFormat);
  tex->set_data((GLubyte*) ilGetData());
  tex->texImage();
  if(mipmapFlag != GL_NONE) {
    tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    tex->setupMipmaps(mipmapFlag);
  }
  else {
    tex->set_filter(GL_LINEAR, GL_LINEAR);
  }

  ilDeleteImages(1, &ilID);

  return tex;
}

ref_ptr<TextureCube> TextureLoader::loadCube(
    const string &file,
    GLboolean flipBackFace,
    GLenum mipmapFlag,
    GLenum forcedFormat,
    const Vec3ui &forcedSize)
{
  GLuint ilID = loadImage(file);
  scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);

  GLint faceWidth, faceHeight;
  GLint width = ilGetInteger(IL_IMAGE_WIDTH);
  GLint height = ilGetInteger(IL_IMAGE_HEIGHT);
  GLint faces[12];
  if(width > height) {
    faceWidth = width/4;
    faceHeight = height/3;
    GLint faces_[12] = {
                      -1,    TextureCube::TOP,                -1,                -1,
      TextureCube::RIGHT,  TextureCube::FRONT, TextureCube::LEFT, TextureCube::BACK,
                      -1, TextureCube::BOTTOM,                -1,                -1
    };
    for(ILint i=0; i<12; ++i) faces[i]=faces_[i];
  } else {
    faceWidth = width/3;
    faceHeight = height/4;
    GLint faces_[12] = {
                      -1,    TextureCube::TOP,                -1,
      TextureCube::RIGHT,  TextureCube::FRONT, TextureCube::LEFT,
                      -1, TextureCube::BOTTOM,                -1,
                      -1,   TextureCube::BACK,                -1
    };
    for(ILint i=0; i<12; ++i) faces[i]=faces_[i];
  }
  const ILint numRows = height/faceHeight;
  const ILint numCols = width/faceWidth;
  const ILint bpp = ilGetInteger(IL_IMAGE_BPP);
  const ILint faceBytes = bpp*faceWidth*faceHeight;
  const ILint rowBytes = faceBytes*numCols;

  ref_ptr<TextureCube> tex = ref_ptr<TextureCube>::manage(new TextureCube);
  tex->bind();
  tex->set_size(faceWidth, faceHeight);
  tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
  tex->set_format(ilToGLFormat());
  tex->set_internalFormat(forcedFormat==GL_NONE ? tex->format() : forcedFormat);

  GLbyte *imageData = (GLbyte*) ilGetData();
  ILint index = 0;
  for(ILint row=0; row<numRows; ++row)
  {
    GLbyte *colData = imageData;
    for(ILint col=0; col<numCols; ++col)
    {
      ILint mappedFace = faces[index];
      if(mappedFace!=-1) {
        tex->set_data((TextureCube::CubeSide)mappedFace, colData);
      }
      index += 1;
      colData += bpp*faceWidth;
    }
    imageData += rowBytes;
  }
  glPixelStorei(GL_UNPACK_ROW_LENGTH,faceWidth*numCols);
  tex->cubeTexImage(TextureCube::LEFT);
  tex->cubeTexImage(TextureCube::RIGHT);
  tex->cubeTexImage(TextureCube::TOP);
  tex->cubeTexImage(TextureCube::BOTTOM);
  tex->cubeTexImage(TextureCube::FRONT);
  if(flipBackFace)
  {
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
    GLbyte *flippedFace = new GLbyte[faceBytes];
    GLbyte *faceData = (GLbyte*)tex->cubeData()[TextureCube::BACK];
    GLbyte *dst = flippedFace;

    for(ILint row=faceWidth-1; row>=0; --row)
    {
      GLbyte *rowData = faceData + row*faceWidth*numCols*bpp;
      for(ILint col=faceHeight-1; col>=0; --col)
      {
        GLbyte *pixelData = rowData + col*bpp;
        memcpy(dst, pixelData, bpp);
        dst += bpp;
      }
    }

    tex->set_data(TextureCube::BACK, flippedFace);
    tex->cubeTexImage(TextureCube::BACK);
    delete[] flippedFace;
  }
  else
  {
    tex->cubeTexImage(TextureCube::BACK);
    glPixelStorei(GL_UNPACK_ROW_LENGTH,0);
  }
  if(mipmapFlag != GL_NONE) {
    tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    tex->setupMipmaps(mipmapFlag);
  }
  else {
    tex->set_filter(GL_LINEAR, GL_LINEAR);
  }

  ilDeleteImages(1, &ilID);

  return tex;
}
