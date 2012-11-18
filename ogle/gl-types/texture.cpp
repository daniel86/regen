/*
 * texture.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include "texture.h"
#include <ogle/utility/string-util.h>

GLenum CubeMapTexture::cubeSideToGLSide_[] = {
    GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
    GL_TEXTURE_CUBE_MAP_POSITIVE_X,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
    GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
    GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
};

Texture::Texture(
    GLuint numTextures,
    GLenum target,
    GLenum format,
    GLenum internalFormat,
    GLenum pixelType,
    GLint border,
    GLuint width, GLuint height)
: RectBufferObject(glGenTextures, glDeleteTextures, numTextures),
  targetType_(target),
  format_(format),
  pixelType_(pixelType),
  border_(border),
  internalFormat_(internalFormat),
  data_(NULL),
  isInTSpace_(false),
  useMipmaps_(false),
  numSamples_(1),
  dim_(2)
{
  set_size(width, height);
  data_ = NULL;
}

void Texture::set_internalFormat(GLenum internalFormat)
{
  internalFormat_ = internalFormat;
}
GLenum Texture::internalFormat() const
{
  return internalFormat_;
}
void Texture::set_format(GLenum format)
{
  format_ = format;
}
GLenum Texture::format() const
{
  return format_;
}

GLfloat Texture::texelSizeX()
{
  return 1.0f / ((float)width_);
}
GLfloat Texture::texelSizeY()
{
  return 1.0f / ((float)height_);
}

const GLboolean Texture::useMipmaps() const
{
  return useMipmaps_;
}

void Texture::set_data(GLvoid *data)
{
  data_ = data;
}
GLvoid* Texture::data() const
{
  return data_;
}

GLenum Texture::targetType() const
{
  return targetType_;
}
void Texture::set_targetType(GLenum targetType)
{
  targetType_ = targetType;
}

void Texture::set_pixelType(GLuint pixelType)
{
  pixelType_ = pixelType;
}
GLuint Texture::pixelType() const
{
  return pixelType_;
}

GLsizei Texture::numSamples() const
{
  return numSamples_;
}
void Texture::set_numSamples(GLsizei v)
{
  numSamples_ = v;
}

///////////////

Texture1D::Texture1D(GLuint numTextures)
: Texture(numTextures)
{
  dim_ = 1;
  targetType_ = GL_TEXTURE_1D;
}
void Texture1D::texImage() const
{
  glTexImage1D(
      targetType_,
      0, // mipmap level
      internalFormat_,
      width_,
      border_,
      format_,
      pixelType_,
      data_);
}
void Texture1D::texSubImage() const
{
  glTexSubImage1D(
      targetType_,
      0,
      0,
      width_,
      format_,
      pixelType_,
      data_);
}
string Texture1D::samplerType() const
{
  return "sampler1D";
}

Texture2D::Texture2D(GLuint numTextures)
: Texture(numTextures)
{
  dim_ = 2;
  targetType_ = GL_TEXTURE_2D;
}
void Texture2D::texImage() const
{
  glTexImage2D(targetType_,
               0, // mipmap level
               internalFormat_,
               width_,
               height_,
               border_,
               format_,
               pixelType_,
               data_);
}
void Texture2D::texSubImage() const
{
  glTexSubImage2D(targetType_,
      0,
      0,0,
      width_, height_,
      format_,
      pixelType_,
      data_);
}
string Texture2D::samplerType() const
{
  return "sampler2D";
}

TextureRectangle::TextureRectangle(GLuint numTextures)
: Texture2D(numTextures)
{
  targetType_ = GL_TEXTURE_RECTANGLE;
}
string TextureRectangle::samplerType() const
{
  return "sampler2DRect";
}

DepthTexture2D::DepthTexture2D(GLuint numTextures)
: Texture2D(numTextures)
{
  format_ = GL_DEPTH_COMPONENT;
  internalFormat_ = GL_DEPTH_COMPONENT;
  pixelType_ = GL_UNSIGNED_BYTE;
}

Texture2DMultisample::Texture2DMultisample(
    GLsizei numSamples,
    GLuint numTextures,
    GLboolean fixedSampleLaocations)
: Texture2D(numTextures)
{
  targetType_ = GL_TEXTURE_2D_MULTISAMPLE;
  fixedsamplelocations_ = fixedSampleLaocations;
  set_numSamples(numSamples);
}
void Texture2DMultisample::texImage() const
{
  glTexImage2DMultisample(targetType_,
      numSamples(),
      internalFormat_,
      width_,
      height_,
      fixedsamplelocations_);
}
string Texture2DMultisample::samplerType() const
{
  return "sampler2DMS";
}

DepthTexture2DMultisample::DepthTexture2DMultisample(
    GLsizei numSamples,
    GLboolean fixedSampleLaocations)
: DepthTexture2D()
{
  targetType_ = GL_TEXTURE_2D_MULTISAMPLE;
  fixedsamplelocations_ = fixedSampleLaocations;
  set_numSamples(numSamples);
}
void DepthTexture2DMultisample::texImage() const
{
  glTexImage2DMultisample(targetType_,
      numSamples(),
      internalFormat_,
      width_,
      height_,
      fixedsamplelocations_);
}

CubeMapTexture::CubeMapTexture(GLuint numTextures)
: Texture2D(numTextures)
{
  targetType_ = GL_TEXTURE_CUBE_MAP;
  dim_ = 3;
}
void CubeMapTexture::set_data(CubeSide side, void *data)
{
  cubeData_[side] = data;
}
void CubeMapTexture::texImage() const
{
  cubeTexImage(LEFT);
  cubeTexImage(RIGHT);
  cubeTexImage(TOP);
  cubeTexImage(BOTTOM);
  cubeTexImage(FRONT);
  cubeTexImage(BACK);
}
void CubeMapTexture::cubeTexImage(CubeSide side) const {
  glTexImage2D(cubeSideToGLSide_[side],
               0, // mipmap level
               internalFormat_,
               width_,
               height_,
               border_,
               format_,
               pixelType_,
               cubeData_[side]);
}
string CubeMapTexture::samplerType() const
{
  return "samplerCube";
}

NoiseTexture2D::NoiseTexture2D(GLuint width, GLuint height)
: Texture2D()
{
  char* pixels = new char[width * height];

  pixelType_ = GL_UNSIGNED_BYTE;
  internalFormat_ = GL_R8;
  format_ = GL_LUMINANCE;
  width_ = width;
  height_ = height;
  data_ = pixels;

  char* pDest = pixels;
  for (GLint i = 0; i < width * height; i++) {
      *pDest++ = rand() % 256;
  }

  bind();
  set_filter(GL_NEAREST, GL_NEAREST);
  set_wrapping(GL_REPEAT);
  texImage();

  delete [] pixels;
}
