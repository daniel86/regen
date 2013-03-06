/*
 * texture.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include <ogle/utility/string-util.h>
#include <ogle/gl-types/gl-enum.h>

using namespace ogle;
#include "texture.h"

Texture::Texture(GLuint numTextures)
: RectBufferObject(glGenTextures, glDeleteTextures, numTextures),
  dim_(2),
  targetType_(GL_TEXTURE_2D),
  format_(GL_RGBA),
  internalFormat_(GL_RGBA8),
  pixelType_(GL_BYTE),
  border_(0),
  data_(NULL),
  isInTSpace_(false),
  numSamples_(1)
{
  set_size(2, 2);
  data_ = NULL;
  samplerType_ = "sampler2D";
}

const string& Texture::samplerType() const
{
  return samplerType_;
}
void Texture::set_samplerType(const string &samplerType)
{
  samplerType_ = samplerType;
}

GLuint Texture::numComponents() const
{
  return dim_;
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

GLfloat Texture::texelSizeX() const
{
  return 1.0f / ((float)width_);
}
GLfloat Texture::texelSizeY() const
{
  return 1.0f / ((float)height_);
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

void Texture::set_filter(GLenum mag, GLenum min) const {
  glTexParameteri(targetType_, GL_TEXTURE_MAG_FILTER, mag);
  glTexParameteri(targetType_, GL_TEXTURE_MIN_FILTER, min);
}

void Texture::set_minLoD(GLfloat min) const {
  glTexParameterf(targetType_, GL_TEXTURE_MIN_LOD, min);
}
void Texture::set_maxLoD(GLfloat max) const {
  glTexParameterf(targetType_, GL_TEXTURE_MAX_LOD, max);
}

void Texture::set_maxLevel(GLint maxLevel) const {
  glTexParameteri(targetType_, GL_TEXTURE_MAX_LEVEL, maxLevel);
}

void Texture::set_swizzleR(GLenum swizzleMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_SWIZZLE_R, swizzleMode);
}
void Texture::set_swizzleG(GLenum swizzleMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_SWIZZLE_G, swizzleMode);
}
void Texture::set_swizzleB(GLenum swizzleMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_SWIZZLE_B, swizzleMode);
}
void Texture::set_swizzleA(GLenum swizzleMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_SWIZZLE_A, swizzleMode);
}

void Texture::set_wrapping(GLenum wrapMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_WRAP_S, wrapMode);
  glTexParameterf(targetType_, GL_TEXTURE_WRAP_T, wrapMode);
  glTexParameterf(targetType_, GL_TEXTURE_WRAP_R, wrapMode);
}
void Texture::set_wrappingU(GLenum wrapMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_WRAP_S, wrapMode);
}
void Texture::set_wrappingV(GLenum wrapMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_WRAP_T, wrapMode);
}
void Texture::set_wrappingW(GLenum wrapMode) const {
  glTexParameterf(targetType_, GL_TEXTURE_WRAP_R, wrapMode);
}

void Texture::set_compare(GLenum mode, GLenum func) const {
  glTexParameteri(targetType_, GL_TEXTURE_COMPARE_MODE, mode);
  glTexParameteri(targetType_, GL_TEXTURE_COMPARE_FUNC, func);
}

void Texture::set_aniso(GLfloat v) const {
  glTexParameterf(targetType_, GL_TEXTURE_MAX_ANISOTROPY_EXT, v);
}

void Texture::set_envMode(GLenum envMode) const {
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, envMode);
}

void Texture::setupMipmaps(GLenum mode) const {
  // glGenerateMipmap was introduced in opengl3.0
  // before glBuildMipmaps or GL_GENERATE_MIPMAP was used, but we do not need them ;)
  glGenerateMipmap(targetType_);
  glHint(GL_GENERATE_MIPMAP_HINT, mode);
}

///////////////

Texture1D::Texture1D(GLuint numTextures)
: Texture(numTextures)
{
  dim_ = 1;
  targetType_ = GL_TEXTURE_1D;
  samplerType_ = "sampler1D";
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

Texture2D::Texture2D(GLuint numTextures)
: Texture(numTextures)
{
  dim_ = 2;
  targetType_ = GL_TEXTURE_2D;
  samplerType_ = "sampler2D";
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

TextureRectangle::TextureRectangle(GLuint numTextures)
: Texture2D(numTextures)
{
  targetType_ = GL_TEXTURE_RECTANGLE;
  samplerType_ = "sampler2DRect";
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
  samplerType_ = "sampler2DMS";
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

TextureCube::TextureCube(GLuint numTextures)
: Texture2D(numTextures)
{
  samplerType_ = "samplerCube";
  targetType_ = GL_TEXTURE_CUBE_MAP;
  dim_ = 3;
  for(int i=0; i<6; ++i) { cubeData_[i] = NULL; }
}
void TextureCube::set_data(CubeSide side, void *data)
{
  cubeData_[side] = data;
}
void** TextureCube::cubeData()
{
  return cubeData_;
}

void TextureCube::texImage() const
{
  cubeTexImage(LEFT);
  cubeTexImage(RIGHT);
  cubeTexImage(TOP);
  cubeTexImage(BOTTOM);
  cubeTexImage(FRONT);
  cubeTexImage(BACK);
}
void TextureCube::cubeTexImage(CubeSide side) const {
  glTexImage2D(cubeMapLayerEnum(side),
               0, // mipmap level
               internalFormat_,
               width_,
               height_,
               border_,
               format_,
               pixelType_,
               cubeData_[side]);
}

CubeMapDepthTexture::CubeMapDepthTexture(GLuint numTextures)
: TextureCube(numTextures)
{
  format_ = GL_DEPTH_COMPONENT;
  internalFormat_ = GL_DEPTH_COMPONENT;
  pixelType_ = GL_UNSIGNED_BYTE;
}

Texture3D::Texture3D(GLuint numTextures)
: Texture(numTextures)
{
  dim_ = 3;
  targetType_ = GL_TEXTURE_3D;
  samplerType_ = "sampler3D";
}
void Texture3D::set_depth(GLuint numTextures) {
  numTextures_ = numTextures;
}
GLuint Texture3D::depth() {
  return numTextures_;
}
void Texture3D::texImage() const {
  glTexImage3D(targetType_,
      0, // mipmap level
      internalFormat_,
      width_,
      height_,
      numTextures_,
      border_,
      format_,
      pixelType_,
      data_);
}
void Texture3D::texSubImage(GLint layer, GLubyte *subData) const {
  glTexSubImage3D(
          targetType_,
          0, 0, 0, // offset
          layer,
          width_,
          height_,
          1,
          format_,
          pixelType_,
          subData);
}

DepthTexture3D::DepthTexture3D(GLuint numTextures) : Texture3D(numTextures)
{
  format_  = GL_DEPTH_COMPONENT;
  internalFormat_ = GL_DEPTH_COMPONENT;
}
Texture2DArray::Texture2DArray(GLuint numTextures) : Texture3D(numTextures)
{
  samplerType_ = "sampler2DArray";
  targetType_ = GL_TEXTURE_2D_ARRAY;
}
