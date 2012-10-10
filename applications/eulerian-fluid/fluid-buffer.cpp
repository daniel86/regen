/*
 * fluid-buffer.cpp
 *
 *  Created on: 09.10.2012
 *      Author: daniel
 */

#include "fluid-buffer.h"
#include <ogle/gl-types/volume-texture.h>

FluidBuffer::FluidBuffer(
    const string &name,
    ref_ptr<Texture> &fluidTexture)
: FrameBufferObject(fluidTexture->width(), fluidTexture->height()),
  name_(name),
  fluidTexture_(fluidTexture)
{
  bind();
  addColorAttachment( *fluidTexture_.get() );
}
FluidBuffer::FluidBuffer(
    const string &name,
    Vec3i size,
    GLuint numComponents,
    GLuint numTextures,
    PixelType pixelType)
: FrameBufferObject(size.x, size.y),
  name_(name)
{
  bind();

  fluidTexture_ = createTexture(
      size,
      numComponents,
      numTextures,
      pixelType);
  colorAttachmentFormat_ = fluidTexture_->internalFormat();
  for(GLint i=0; i<numTextures; ++i) {
    fluidTexture_->bind();
    addColorAttachment( *fluidTexture_.get() );
    fluidTexture_->nextBuffer();
  }

  clear();
}

ref_ptr<Texture> FluidBuffer::createTexture(
    Vec3i size,
    GLint numComponents,
    GLint numTexs,
    PixelType pixelType)
{
  ref_ptr<Texture> tex;

  if(size.z < 2) {
    tex = ref_ptr<Texture>::manage( new Texture2D(numTexs) );
  } else {
    ref_ptr<Texture3D> tex3D = ref_ptr<Texture3D>::manage( new Texture3D(numTexs) );
    tex3D->set_numTextures(size.z);
    tex = ref_ptr<Texture>::cast(tex3D);
  }
  tex->set_size(size.x, size.y);

  switch (numComponents) {
  case 1:
    tex->set_format(GL_RED);
    break;
  case 2:
    tex->set_format(GL_RG);
    break;
  case 3:
    tex->set_format(GL_RGB);
    break;
  case 4:
    tex->set_format(GL_RGBA);
    break;
  }

  switch(pixelType) {
  case F16:
    tex->set_pixelType(GL_HALF_FLOAT);
    switch (numComponents) {
    case 1:
      tex->set_internalFormat(GL_R16F);
      break;
    case 2:
      tex->set_internalFormat(GL_RG16F);
      break;
    case 3:
      tex->set_internalFormat(GL_RGB16F);
      break;
    case 4:
      tex->set_internalFormat(GL_RGBA16F);
      break;
    }
    break;
  case F32:
    tex->set_pixelType(GL_FLOAT);
    switch (numComponents) {
    case 1:
      tex->set_internalFormat(GL_R32F);
      break;
    case 2:
      tex->set_internalFormat(GL_RG32F);
      break;
    case 3:
      tex->set_internalFormat(GL_RGB32F);
      break;
    case 4:
      tex->set_internalFormat(GL_RGBA32F);
      break;
    }
    break;
  case BYTE:
  default:
      tex->set_pixelType(GL_UNSIGNED_BYTE);
      switch (numComponents) {
      case 1:
        tex->set_internalFormat(GL_RED);
        break;
      case 2:
        tex->set_internalFormat(GL_RG);
        break;
      case 3:
        tex->set_internalFormat(GL_RGB);
        break;
      case 4:
        tex->set_internalFormat(GL_RGBA);
        break;
      }
      break;
  }

  for(int i=0; i<numTexs; ++i) {
    tex->bind();
    GLenum mode = GL_CLAMP_TO_EDGE;
    tex->set_wrappingU(mode);
    tex->set_wrappingV(mode);
    tex->set_wrappingW(mode);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    tex->nextBuffer();
  }

  return tex;
}

Texture* FluidBuffer::fluidTexture()
{
  return fluidTexture_.get();
}

void FluidBuffer::clear()
{
  bind();
  GLenum buffers[fluidTexture_->numBuffers()];
  for(register int i=0; i<fluidTexture_->numBuffers(); ++i) {
    buffers[i] = GL_COLOR_ATTACHMENT0+i;
  }
  glDrawBuffers(fluidTexture_->numBuffers(), buffers);
  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void FluidBuffer::swap()
{
  fluidTexture_->nextBuffer();
}
