/*
 * texture.cpp
 *
 *  Created on: 23.03.2011
 *      Author: daniel
 */

#include <sstream>
#include <stdio.h>
#include <stdlib.h>

#include <regen/utility/string-util.h>
#include <regen/gl-types/gl-util.h>
#include <regen/gl-types/gl-enum.h>
#include <regen/gl-types/render-state.h>

using namespace regen;
#include "texture.h"

Texture::Texture(GLuint numTextures)
: GLRectangle(glGenTextures, glDeleteTextures, numTextures),
  ShaderInput1i("textureChannel"),
  dim_(2),
  format_(GL_RGBA),
  internalFormat_(GL_RGBA8),
  pixelType_(GL_BYTE),
  border_(0),
  data_(NULL),
  isInTSpace_(false),
  numSamples_(1)
{
  set_rectangleSize(2, 2);
  data_ = NULL;
  samplerType_ = "sampler2D";
  texBind_.target_ = GL_TEXTURE_2D;
  setUniformData(-1);
}

GLint Texture::channel() const
{ return getVertex1i(0); }

const string& Texture::samplerType() const
{ return samplerType_; }
void Texture::set_samplerType(const string &samplerType)
{ samplerType_ = samplerType; }

GLuint Texture::numComponents() const
{ return dim_; }

void Texture::set_internalFormat(GLenum internalFormat)
{ internalFormat_ = internalFormat; }
GLenum Texture::internalFormat() const
{ return internalFormat_; }

void Texture::set_format(GLenum format)
{ format_ = format; }
GLenum Texture::format() const
{ return format_; }

GLfloat Texture::texelSizeX() const
{ return 1.0f / ((float)width_); }
GLfloat Texture::texelSizeY() const
{ return 1.0f / ((float)height_); }

void Texture::set_data(GLvoid *data)
{ data_ = data; }
GLvoid* Texture::data() const
{ return data_; }

GLenum Texture::targetType() const
{ return texBind_.target_; }
void Texture::set_targetType(GLenum targetType)
{ texBind_.target_ = targetType; }

void Texture::set_pixelType(GLuint pixelType)
{ pixelType_ = pixelType; }
GLuint Texture::pixelType() const
{ return pixelType_; }

GLsizei Texture::numSamples() const
{ return numSamples_; }
void Texture::set_numSamples(GLsizei v)
{ numSamples_ = v; }

const TextureBind& Texture::textureBind()
{ texBind_.id_ = id(); return texBind_; }

void Texture::set_filter(GLenum mag, GLenum min) const {
  glTexParameteri(texBind_.target_, GL_TEXTURE_MAG_FILTER, mag);
  glTexParameteri(texBind_.target_, GL_TEXTURE_MIN_FILTER, min);
}

void Texture::set_minLoD(GLfloat min) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_MIN_LOD, min);
}
void Texture::set_maxLoD(GLfloat max) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_MAX_LOD, max);
}

void Texture::set_maxLevel(GLint maxLevel) const {
  glTexParameteri(texBind_.target_, GL_TEXTURE_MAX_LEVEL, maxLevel);
}

void Texture::set_swizzleR(GLenum swizzleMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_SWIZZLE_R, swizzleMode);
}
void Texture::set_swizzleG(GLenum swizzleMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_SWIZZLE_G, swizzleMode);
}
void Texture::set_swizzleB(GLenum swizzleMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_SWIZZLE_B, swizzleMode);
}
void Texture::set_swizzleA(GLenum swizzleMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_SWIZZLE_A, swizzleMode);
}

void Texture::set_wrapping(GLenum wrapMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_WRAP_S, wrapMode);
  glTexParameterf(texBind_.target_, GL_TEXTURE_WRAP_T, wrapMode);
  glTexParameterf(texBind_.target_, GL_TEXTURE_WRAP_R, wrapMode);
}
void Texture::set_wrappingU(GLenum wrapMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_WRAP_S, wrapMode);
}
void Texture::set_wrappingV(GLenum wrapMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_WRAP_T, wrapMode);
}
void Texture::set_wrappingW(GLenum wrapMode) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_WRAP_R, wrapMode);
}

void Texture::set_compare(GLenum mode, GLenum func) const {
  glTexParameteri(texBind_.target_, GL_TEXTURE_COMPARE_MODE, mode);
  glTexParameteri(texBind_.target_, GL_TEXTURE_COMPARE_FUNC, func);
}

void Texture::set_aniso(GLfloat v) const {
  glTexParameterf(texBind_.target_, GL_TEXTURE_MAX_ANISOTROPY_EXT, v);
}

void Texture::set_envMode(GLenum envMode) const {
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, envMode);
}

void Texture::setupMipmaps(GLenum mode) const {
  // glGenerateMipmap was introduced in opengl3.0
  // before glBuildMipmaps or GL_GENERATE_MIPMAP was used, but we do not need them ;)
  glGenerateMipmap(texBind_.target_);
}

void Texture::begin(RenderState *rs, GLint x)
{
  set_active(GL_TRUE);
  setVertex1i(0,x);
  rs->activeTexture().push(GL_TEXTURE0+x);
  rs->textures().push(x, textureBind());
}
void Texture::end(RenderState *rs, GLint x)
{
  rs->textures().pop(x);
  rs->activeTexture().pop();
  setVertex1i(0,-1);
  set_active(GL_FALSE);
}

///////////////

Texture1D::Texture1D(GLuint numTextures)
: Texture(numTextures)
{
  dim_ = 1;
  texBind_.target_ = GL_TEXTURE_1D;
  samplerType_ = "sampler1D";
}
void Texture1D::texImage() const
{
  glTexImage1D(
      texBind_.target_,
      0, // mipmap level
      internalFormat_,
      width_,
      border_,
      format_,
      pixelType_,
      data_);
}

Texture2D::Texture2D(GLuint numTextures)
: Texture(numTextures)
{
  dim_ = 2;
  texBind_.target_ = GL_TEXTURE_2D;
  samplerType_ = "sampler2D";
}
void Texture2D::texImage() const
{
  glTexImage2D(texBind_.target_,
               0, // mipmap level
               internalFormat_,
               width_,
               height_,
               border_,
               format_,
               pixelType_,
               data_);
}

TextureRectangle::TextureRectangle(GLuint numTextures)
: Texture2D(numTextures)
{
  texBind_.target_ = GL_TEXTURE_RECTANGLE;
  samplerType_ = "sampler2DRect";
}

Texture2DDepth::Texture2DDepth(GLuint numTextures)
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
  texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE;
  fixedsamplelocations_ = fixedSampleLaocations;
  samplerType_ = "sampler2DMS";
  set_numSamples(numSamples);
}
void Texture2DMultisample::texImage() const
{
  glTexImage2DMultisample(texBind_.target_,
      numSamples(),
      internalFormat_,
      width_,
      height_,
      fixedsamplelocations_);
}

Texture2DMultisampleDepth::Texture2DMultisampleDepth(
    GLsizei numSamples,
    GLboolean fixedSampleLaocations)
: Texture2DDepth()
{
  texBind_.target_ = GL_TEXTURE_2D_MULTISAMPLE;
  fixedsamplelocations_ = fixedSampleLaocations;
  set_numSamples(numSamples);
}
void Texture2DMultisampleDepth::texImage() const
{
  glTexImage2DMultisample(texBind_.target_,
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
  texBind_.target_ = GL_TEXTURE_CUBE_MAP;
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
  glTexImage2D(glenum::cubeMapLayer(side),
               0, // mipmap level
               internalFormat_,
               width_,
               height_,
               border_,
               format_,
               pixelType_,
               cubeData_[side]);
}

TextureCubeDepth::TextureCubeDepth(GLuint numTextures)
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
  texBind_.target_ = GL_TEXTURE_3D;
  samplerType_ = "sampler3D";
}
void Texture3D::set_depth(GLuint numTextures) {
  numTextures_ = numTextures;
}
GLuint Texture3D::depth() {
  return numTextures_;
}
void Texture3D::texImage() const {
  glTexImage3D(texBind_.target_,
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
      texBind_.target_,
      0, 0, 0, // offset
      layer,
      width_,
      height_,
      1,
      format_,
      pixelType_,
      subData);
}

Texture3DDepth::Texture3DDepth(GLuint numTextures) : Texture3D(numTextures)
{
  format_  = GL_DEPTH_COMPONENT;
  internalFormat_ = GL_DEPTH_COMPONENT;
}
Texture2DArray::Texture2DArray(GLuint numTextures) : Texture3D(numTextures)
{
  samplerType_ = "sampler2DArray";
  texBind_.target_ = GL_TEXTURE_2D_ARRAY;
}

///////////////
///////////////
///////////////

TextureBuffer::TextureBuffer(GLenum texelFormat)
: Texture()
{
  texBind_.target_ = GL_TEXTURE_BUFFER;
  samplerType_ = "samplerBuffer";
  texelFormat_ = texelFormat;
}

void TextureBuffer::attach(const ref_ptr<VBO> &vbo, VBOReference &ref)
{
  attachedVBO_ = vbo;
  attachedVBORef_ = ref;
#ifdef GL_ARB_texture_buffer_range
  glTexBufferRange(
      texBind_.target_,
      texelFormat_,
      ref->bufferID(),
      ref->address(),
      ref->allocatedSize());
#else
  glTexBuffer(targetType_, texelFormat_, ref->bufferID());
#endif
  GL_ERROR_LOG();
}
void TextureBuffer::attach(GLuint storage)
{
  attachedVBO_ = ref_ptr<VBO>();
  attachedVBORef_ = VBOReference();
  glTexBuffer(texBind_.target_, texelFormat_, storage);
  GL_ERROR_LOG();
}
void TextureBuffer::attach(GLuint storage, GLuint offset, GLuint size)
{
  attachedVBO_ = ref_ptr<VBO>();
  attachedVBORef_ = VBOReference();
#ifdef GL_ARB_texture_buffer_range
  glTexBufferRange(texBind_.target_, texelFormat_, storage, offset, size);
#else
  glTexBuffer(targetType_, texelFormat_, storage);
#endif
  GL_ERROR_LOG();
}

void TextureBuffer::texImage() const
{
}
