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

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
#include <boost/foreach.hpp>

#include <stdio.h>
#include <string.h>
#include <fstream>

#include <regen/utility/string-util.h>
#include <regen/utility/logging.h>
#include <regen/external/spectrum.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/render-state.h>

#include "texture-loader.h"
using namespace regen;

static GLenum regenImageFormat()
{
  GLenum format = ilGetInteger(IL_IMAGE_FORMAT);
  switch(format) {
  case GL_LUMINANCE: return GL_RED;
  case GL_LUMINANCE_ALPHA: return GL_RG;
  default: return format;
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

static void convertImage(GLenum format, GLenum type)
{
  GLenum srcFormat = regenImageFormat();
  GLenum srcType = ilGetInteger(IL_IMAGE_TYPE);
  GLenum dstFormat = (format==GL_NONE ? srcFormat : format);
  GLenum dstType = (type==GL_NONE ? srcType : type);
  if(srcFormat!=dstFormat || srcType!=dstType) {
    if(!ilConvertImage(dstFormat, dstType) == IL_FALSE)
    {
      throw TextureLoader::Error("ilConvertImage failed");
    }
  }
}

static GLuint loadImage(const string &file)
{
  static GLboolean devilInitialized_ = GL_FALSE;
  if(!devilInitialized_) {
    ilInit();
    devilInitialized_ = GL_TRUE;
  }

  if (!boost::filesystem::exists(file)) {
    throw TextureLoader::Error(FORMAT_STRING(
        "Unable to open image file at '" << file << "'."));
  }

  GLuint ilID;
  ilGenImages(1, &ilID);
  ilBindImage(ilID);
  if(ilLoadImage(file.c_str()) == IL_FALSE) {
    throw TextureLoader::Error("ilLoadImage failed");
  }

  DEBUG_LOG("Texture '" << file << "'" <<
      " format=" << regenImageFormat() <<
      " type=" << ilGetInteger(IL_IMAGE_TYPE) <<
      " bpp=" << ilGetInteger(IL_IMAGE_BPP) <<
      " channels=" << ilGetInteger(IL_IMAGE_CHANNELS) <<
      " width=" << ilGetInteger(IL_IMAGE_WIDTH) <<
      " height=" << ilGetInteger(IL_IMAGE_HEIGHT));

  return ilID;
}

ref_ptr<Texture> TextureLoader::load(
    const string &file,
    GLenum mipmapFlag,
    GLenum forcedInternalFormat,
    GLenum forcedFormat,
    GLenum forcedType,
    const Vec3ui &forcedSize)
{
  GLuint ilID = loadImage(file);
  scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
  convertImage(forcedFormat, forcedType);
  GLint depth = ilGetInteger(IL_IMAGE_DEPTH);

  ref_ptr<Texture> tex;
  if(depth>1) {
    ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::manage(new Texture3D);
    tex3D->set_depth(depth);
    tex = tex3D;
  }
  else {
    tex = ref_ptr<Texture>::manage(new Texture2D);
  }
  tex->startConfig();
  tex->set_size(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
  tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
  tex->set_format(regenImageFormat());
  tex->set_internalFormat(
      forcedInternalFormat==GL_NONE ? tex->format() : forcedInternalFormat);
  tex->set_data((GLubyte*) ilGetData());
  tex->texImage();
  tex->set_data(NULL);
  if(mipmapFlag != GL_NONE) {
    tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    tex->setupMipmaps(mipmapFlag);
  }
  else {
    tex->set_filter(GL_LINEAR, GL_LINEAR);
  }
  tex->set_wrapping(GL_REPEAT);
  tex->stopConfig();

  ilDeleteImages(1, &ilID);

  return tex;
}

ref_ptr<Texture2DArray> TextureLoader::loadArray(
    const string &textureDirectory,
    const string &textureNamePattern,
    GLenum mipmapFlag,
    GLenum forcedInternalFormat,
    GLenum forcedFormat,
    GLenum forcedType,
    const Vec3ui &forcedSize)
{
  GLuint numTextures=0;
  list<string> textureFiles;

  boost::filesystem::path texturedir(textureDirectory);
  boost::regex pattern(textureNamePattern);

  set<string> accumulator;
  boost::filesystem::directory_iterator it(texturedir), eod;
  BOOST_FOREACH(const boost::filesystem::path &filePath, make_pair(it, eod))
  {
    string name = filePath.filename().string();
    if(boost::regex_match(name, pattern))
    {
      accumulator.insert(filePath.string());
      numTextures += 1;
    }
  }

  ref_ptr<Texture2DArray> tex = ref_ptr<Texture2DArray>::manage(new Texture2DArray);
  tex->set_depth(numTextures);
  tex->startConfig();

  GLint arrayIndex = 0;
  for(set<string>::iterator
      it=accumulator.begin(); it!=accumulator.end(); ++it)
  {
    const string &textureFile = *it;
    GLuint ilID = loadImage(textureFile);
    scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
    convertImage(forcedFormat, forcedType);

    if(arrayIndex==0) {
      tex->set_size(ilGetInteger(IL_IMAGE_WIDTH), ilGetInteger(IL_IMAGE_HEIGHT));
      tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
      tex->set_format(regenImageFormat());
      tex->set_internalFormat(
          forcedInternalFormat==GL_NONE ? tex->format() : forcedInternalFormat);
      tex->set_data(NULL);
      tex->texImage();
    }

    tex->texSubImage(arrayIndex, (GLubyte*) ilGetData());
    ilDeleteImages(1, &ilID);
  }

  if(mipmapFlag != GL_NONE) {
    tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    tex->setupMipmaps(mipmapFlag);
  }
  else {
    tex->set_filter(GL_LINEAR, GL_LINEAR);
  }
  tex->set_wrapping(GL_REPEAT);

  tex->stopConfig();

  return tex;
}

ref_ptr<TextureCube> TextureLoader::loadCube(
    const string &file,
    GLboolean flipBackFace,
    GLenum mipmapFlag,
    GLenum forcedInternalFormat,
    GLenum forcedFormat,
    GLenum forcedType,
    const Vec3ui &forcedSize)
{
  GLuint ilID = loadImage(file);
  scaleImage(forcedSize.x, forcedSize.y, forcedSize.z);
  convertImage(forcedFormat, forcedType);

  GLint faceWidth, faceHeight;
  GLint width = ilGetInteger(IL_IMAGE_WIDTH);
  GLint height = ilGetInteger(IL_IMAGE_HEIGHT);
  GLint faces[12];
  // guess layout
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
  tex->startConfig();
  tex->set_size(faceWidth, faceHeight);
  tex->set_pixelType(ilGetInteger(IL_IMAGE_TYPE));
  tex->set_format(regenImageFormat());
  tex->set_internalFormat(
      forcedInternalFormat==GL_NONE ? tex->format() : forcedInternalFormat);

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
    glPixelStorei(GL_UNPACK_ROW_LENGTH,4);
  }
  if(mipmapFlag != GL_NONE) {
    tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    tex->setupMipmaps(mipmapFlag);
  }
  else {
    tex->set_filter(GL_LINEAR, GL_LINEAR);
  }

  tex->stopConfig();

  ilDeleteImages(1, &ilID);

  return tex;
}

ref_ptr<Texture> TextureLoader::loadRAW(
    const string &path,
    const Vec3ui &size,
    GLuint numComponents,
    GLuint bytesPerComponent)

{
  ifstream f(path.c_str(),
      ios::in
      |ios::binary
      |ios::ate // start at end position
      );
  if (!f.is_open()) {
    throw Error(FORMAT_STRING(
        "Unable to open data set file at '" << path << "'."));
  }

  int numBytes = size.x*size.y*size.z*numComponents;
  char* pixels = new char[numBytes];
  f.seekg (0, ios::beg);
  f.read(pixels, numBytes);
  f.close();

  GLenum format_ = GLEnum::textureFormat(numComponents);
  GLenum internalFormat_ = GLEnum::textureInternalFormat(GL_UNSIGNED_BYTE, numComponents, bytesPerComponent);

  ref_ptr<Texture> tex;
  if(size.z>1) {
    ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::manage(new Texture3D);
    tex3D->set_depth(size.z);
    tex = tex3D;
  }
  else {
    tex = ref_ptr<Texture>::manage(new Texture2D);
  }

  tex->startConfig();
  tex->set_size(size.x, size.y);
  tex->set_pixelType(GL_UNSIGNED_BYTE);
  tex->set_format(format_);
  tex->set_internalFormat(internalFormat_);
  tex->set_data((GLubyte*)pixels);
  tex->texImage();
  tex->stopConfig();

  delete [] pixels;

  return tex;
}

ref_ptr<Texture> TextureLoader::loadSpectrum(
    GLdouble t1,
    GLdouble t2,
    GLint numTexels,
    GLenum mipmapFlag)
{
  unsigned char* data = new unsigned char[numTexels*4];
  spectrum(t1, t2, numTexels, data);

  ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new Texture1D);
  tex->startConfig();
  tex->set_size(numTexels, 1);
  tex->set_pixelType(GL_UNSIGNED_BYTE);
  tex->set_format(GL_RGBA);
  tex->set_internalFormat(GL_RGBA);
  tex->set_data((GLubyte*)data);
  tex->texImage();
  tex->set_wrapping(GL_CLAMP);
  if(mipmapFlag==GL_NONE) {
    tex->set_filter(GL_LINEAR, GL_LINEAR);
  } else {
    tex->set_filter(GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
    tex->setupMipmaps(mipmapFlag);
  }
  tex->set_data(NULL);
  delete []data;
  tex->stopConfig();

  return tex;
}

