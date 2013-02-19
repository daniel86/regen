/*
 * fbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "fbo.h"

FrameBufferObject::FrameBufferObject(
    GLuint width, GLuint height,
    GLenum depthAttachmentFormat)
: RectBufferObject(glGenFramebuffers, glDeleteFramebuffers),
  depthAttachmentFormat_(depthAttachmentFormat)
{
  set_size(width,height);
  bind();
  if(depthAttachmentFormat_!=GL_NONE) {
    createDepthTexture(depthAttachmentFormat_);
  }

  viewportUniform_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("viewport"));
  viewportUniform_->setUniformData( Vec2f(
      (GLfloat)width, (GLfloat)height) );
}

const ref_ptr<ShaderInput2f>& FrameBufferObject::viewport()
{
  return viewportUniform_;
}

void FrameBufferObject::createDepthTexture(GLenum format)
{
  depthTexture_ = ref_ptr<Texture>::manage(new DepthTexture2D);
  depthTexture_->set_size(width_, height_);
  depthTexture_->set_internalFormat(format);
  depthTexture_->bind();
  depthTexture_->set_wrapping(GL_REPEAT);
  depthTexture_->set_filter(GL_LINEAR, GL_LINEAR);
  depthTexture_->set_compare(GL_NONE, GL_EQUAL);
  depthTexture_->texImage();
  set_depthAttachment(*((DepthTexture2D*)depthTexture_.get()));
}

const ref_ptr<Texture>& FrameBufferObject::depthTexture() const
{
  return depthTexture_;
}
GLenum FrameBufferObject::depthAttachmentFormat() const
{
  return depthAttachmentFormat_;
}
vector< ref_ptr<Texture> >& FrameBufferObject::colorBuffer()
{
  return colorBuffer_;
}
const ref_ptr<Texture>& FrameBufferObject::firstColorBuffer() const
{
  return colorBuffer_.front();
}

void FrameBufferObject::set_depthAttachment(const Texture2D &tex) const
{
  attachTexture(tex, GL_DEPTH_ATTACHMENT);
}
void FrameBufferObject::set_depthAttachment(const RenderBufferObject &rbo) const
{
  attachRenderBuffer(rbo, GL_DEPTH_ATTACHMENT);
}
void FrameBufferObject::set_stencilTexture(const Texture &tex) const
{
  attachTexture(tex, GL_STENCIL_ATTACHMENT);
}
void FrameBufferObject::set_stencilTexture(const RenderBufferObject &rbo) const
{
  attachRenderBuffer(rbo, GL_STENCIL_ATTACHMENT);
}
void FrameBufferObject::set_depthStencilTexture(const Texture &tex) const
{
  attachTexture(tex, GL_DEPTH_STENCIL_ATTACHMENT);
}
void FrameBufferObject::set_depthStencilTexture(const RenderBufferObject &rbo) const
{
  attachRenderBuffer(rbo, GL_DEPTH_STENCIL_ATTACHMENT);
}

ref_ptr<Texture> FrameBufferObject::addRectangleTexture(GLuint count,
    GLenum format, GLenum internalFormat)
{
  ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new TextureRectangle(count));
  tex->set_size(width_, height_);
  tex->set_format(format);
  tex->set_internalFormat(internalFormat);
  for(GLuint j=0; j<count; ++j) {
    tex->bind();
    tex->set_wrapping(GL_REPEAT);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    addColorAttachment(*tex.get());
    tex->nextBuffer();
  }
  colorBuffer_.push_back(tex);
  return tex;
}

ref_ptr<Texture> FrameBufferObject::addTexture(GLuint count,
    GLenum format, GLenum internalFormat)
{
  ref_ptr<Texture> tex = ref_ptr<Texture>::manage(new Texture2D(count));
  tex->set_size(width_, height_);
  tex->set_format(format);
  tex->set_internalFormat(internalFormat);
  for(GLuint j=0; j<count; ++j) {
    tex->bind();
    tex->set_wrapping(GL_CLAMP_TO_EDGE);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    addColorAttachment(*tex.get());
    tex->nextBuffer();
  }
  colorBuffer_.push_back(tex);
  return tex;
}

ref_ptr<RenderBufferObject> FrameBufferObject::addRenderBuffer(GLuint count)
{
  ref_ptr<RenderBufferObject> rbo = ref_ptr<RenderBufferObject>::manage(new RenderBufferObject(count));
  rbo->set_size(width_, height_);
  for(GLuint j=0; j<count; ++j) {
    rbo->bind();
    rbo->storage();
    addColorAttachment(*rbo.get());
    rbo->nextBuffer();
  }
  renderBuffer_.push_back(rbo);
  return rbo;
}

GLenum FrameBufferObject::addColorAttachment(const Texture &tex) {
  GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.size();
  attachTexture(tex, attachment);
  colorBuffers_.push_back(attachment);
  return attachment;
}

GLenum FrameBufferObject::addColorAttachment(const RenderBufferObject &rbo) {
  GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.size();
  attachRenderBuffer(rbo, attachment);
  colorBuffers_.push_back(attachment);
  return attachment;
}

void FrameBufferObject::blitCopy(
    FrameBufferObject &dst,
    GLenum readAttachment,
    GLenum writeAttachment,
    GLbitfield mask,
    GLenum filter) const
{
  bind(GL_READ_FRAMEBUFFER);
  glReadBuffer(readAttachment);
  dst.bind(GL_DRAW_FRAMEBUFFER);
  glDrawBuffer(writeAttachment);
  glBlitFramebuffer(
      0, 0, width(), height(),
      0, 0, dst.width(), dst.height(),
      mask, filter);
}

void FrameBufferObject::blitCopyToScreen(
    GLuint screenWidth, GLuint screenHeight,
    GLenum readAttachment,
    GLbitfield mask,
    GLenum filter,
    GLenum screenBuffer) const
{
  bind(GL_READ_FRAMEBUFFER);
  glReadBuffer(readAttachment);
  bindDefault(GL_DRAW_FRAMEBUFFER);
  glDrawBuffer(screenBuffer);
  glBlitFramebuffer(
      0, 0, width(), height(),
      0, 0, screenWidth, screenHeight,
      mask, filter);
}

void FrameBufferObject::resize(
    GLuint width, GLuint height)
{
  set_size(width, height);
  viewportUniform_->setUniformData( Vec2f(
      (GLfloat)width, (GLfloat)height) );
  bind();
  if(depthTexture_.get()!=NULL) {
    depthTexture_->set_size(width_, height_);
    depthTexture_->bind();
    depthTexture_->texImage();
  }
  for(vector< ref_ptr<Texture> >::iterator
      it=colorBuffer_.begin(); it!=colorBuffer_.end(); ++it)
  {
    ref_ptr<Texture> &tex = *it;
    tex->set_size(width_, height_);
    for(GLuint i=0; i<tex->numBuffers(); ++i)
    {
      tex->bind();
      tex->texImage();
      tex->nextBuffer();
    }
  }
  for(vector< ref_ptr<RenderBufferObject> >::iterator
      it=renderBuffer_.begin(); it!=renderBuffer_.end(); ++it)
  {
    ref_ptr<RenderBufferObject> &rbo = *it;
    rbo->set_size(width_, height_);
    for(GLuint i=0; i<rbo->numBuffers(); ++i)
    {
      rbo->bind();
      rbo->storage();
      rbo->nextBuffer();
    }
  }
}

/////////
/////////

SimpleRenderTarget::SimpleRenderTarget(
    const string &name,
    ref_ptr<Texture> &fluidTexture)
: FrameBufferObject(fluidTexture->width(), fluidTexture->height()),
  name_(name),
  texture_(fluidTexture)
{
  bind();
  Texture *tex = texture_.get();
  addColorAttachment( *tex );
  // remember texture size
  size_ = Vec3i(tex->width(), tex->height(), 1);
  Texture3D *tex3D = dynamic_cast<Texture3D*>(tex);
  if(tex3D!=NULL) {
    size_.z = tex3D->depth();
  }
  initUniforms();
}
SimpleRenderTarget::SimpleRenderTarget(
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

  texture_ = createTexture(
      size,
      numComponents,
      numTextures,
      pixelType);
  colorAttachmentFormat_ = texture_->internalFormat();
  for(GLuint i=0u; i<numTextures; ++i) {
    texture_->bind();
    addColorAttachment( *texture_.get() );
    texture_->nextBuffer();
  }

  clear(Vec4f(0.0f), numTextures);
  initUniforms();
}

void SimpleRenderTarget::initUniforms()
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

const ref_ptr<ShaderInputf>& SimpleRenderTarget::inverseSize()
{
  return inverseSize_;
}
const string& SimpleRenderTarget::name()
{
  return name_;
}

ref_ptr<Texture> SimpleRenderTarget::createTexture(
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
    tex3D->set_depth(size.z);
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

ref_ptr<Texture>& SimpleRenderTarget::texture()
{
  return texture_;
}

void SimpleRenderTarget::clear(const Vec4f &clearColor, GLint numBuffers)
{
  if(numBuffers==1) {
    drawBuffer(GL_COLOR_ATTACHMENT0);
  } else {
    static const GLuint mrtBuffers[] = {GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};
    drawBufferMRT(2u, mrtBuffers);
  }
  glClearColor(clearColor.x, clearColor.y, clearColor.z, clearColor.w);
  glClear(GL_COLOR_BUFFER_BIT);
}

void SimpleRenderTarget::swap()
{
  texture_->nextBuffer();
}
