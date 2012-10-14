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
  Texture *tex = fluidTexture_.get();
  addColorAttachment( *tex );
  // remember texture size
  size_ = Vec3i(tex->width(), tex->height(), 1);
  Texture3D *tex3D = dynamic_cast<Texture3D*>(tex);
  if(tex3D!=NULL) {
    size_.z = tex3D->numTextures();
  }
  initUniforms();
}
FluidBuffer::FluidBuffer(
    const string &name,
    Vec3i size,
    GLuint numComponents,
    GLuint numTextures,
    PixelType pixelType)
: FrameBufferObject(size.x, size.y),
  name_(name),
  size_(size)
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

  clear(Vec4f(0.0f));
  initUniforms();
}

void FluidBuffer::initUniforms()
{
  if(size_.z<1) { size_.z=1; }
  if(size_.z>1) {
    inverseSize_ = ref_ptr<ShaderInputf>::manage(new ShaderInput3f("inverseGridSize"));
    ShaderInput3f *in = (ShaderInput3f*) inverseSize_.get();
    in->setUniformData(Vec3f(1.0/size_.x,1.0/size_.y,1.0/size_.z));
  } else {
    inverseSize_ = ref_ptr<ShaderInputf>::manage(new ShaderInput2f("inverseGridSize"));
    ShaderInput2f *in = (ShaderInput2f*) inverseSize_.get();
    in->setUniformData(Vec2f(1.0/size_.x,1.0/size_.y));
  }
}

const ref_ptr<ShaderInputf>& FluidBuffer::inverseSize()
{
  return inverseSize_;
}
const string& FluidBuffer::name()
{
  return name_;
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

ref_ptr<Texture>& FluidBuffer::fluidTexture()
{
  return fluidTexture_;
}

void FluidBuffer::clear(const Vec4f &clearColor)
{
  if(fluidTexture_->numBuffers()==1) {
    drawBuffer(GL_COLOR_ATTACHMENT0);
  } else {
    static const GLuint mrtBuffers[] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};
    drawBufferMRT(2u, mrtBuffers);
  }
  glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT);
}

void FluidBuffer::swap()
{
  fluidTexture_->nextBuffer();
}
