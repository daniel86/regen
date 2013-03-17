/*
 * fbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "fbo.h"
using namespace ogle;

FrameBufferObject::FrameBufferObject(
    GLuint width, GLuint height, GLuint depth,
    GLenum depthTarget, GLenum depthFormat, GLenum depthType)
: RectBufferObject(glGenFramebuffers, glDeleteFramebuffers),
  depthAttachmentTarget_(depthTarget),
  depthAttachmentFormat_(depthFormat),
  depthAttachmentType_(depthType)
{
  set_size(width,height);
  depth_ = depth;
  bind();
  if(depthAttachmentFormat_!=GL_NONE) {
    createDepthTexture(depthAttachmentTarget_,
        depthAttachmentFormat_, depthAttachmentType_);
  }

  viewport_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("viewport"));
  viewport_->setUniformData( Vec2f( (GLfloat)width, (GLfloat)height) );

  inverseViewport_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("inverseViewport"));
  inverseViewport_->setUniformData( Vec2f( 1.0/(GLfloat)width, 1.0/(GLfloat)height) );
}

void FrameBufferObject::createDepthTexture(GLenum target, GLenum format, GLenum type)
{
  depthAttachmentTarget_ = target;
  depthAttachmentFormat_ = format;
  depthAttachmentType_ = type;
  if(target == GL_TEXTURE_CUBE_MAP) {
    depthTexture_ = ref_ptr<Texture>::manage(new CubeMapDepthTexture);
  }
  else if(depth_>1) {
    depthTexture_ = ref_ptr<Texture>::manage(new DepthTexture3D);
    ((Texture3D*)depthTexture_.get())->set_depth(depth_);
  }
  else {
    depthTexture_ = ref_ptr<Texture>::manage(new DepthTexture2D);
  }
  depthTexture_-> set_targetType(target);
  depthTexture_->set_size(width_, height_);
  depthTexture_->set_internalFormat(format);
  depthTexture_->set_pixelType(type);
  depthTexture_->bind();
  depthTexture_->set_wrapping(GL_REPEAT);
  depthTexture_->set_filter(GL_LINEAR, GL_LINEAR);
  depthTexture_->set_compare(GL_NONE, GL_EQUAL);
  depthTexture_->texImage();
  set_depthAttachment(*depthTexture_.get());
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

void FrameBufferObject::set_depthAttachment(const Texture &tex) const
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

ref_ptr<Texture> FrameBufferObject::addTexture(
    GLuint count,
    GLenum targetType,
    GLenum format,
    GLenum internalFormat,
    GLenum pixelType)
{
  ref_ptr<Texture> tex;
  switch(targetType) {
  case GL_TEXTURE_RECTANGLE:
    tex = ref_ptr<Texture>::manage(new TextureRectangle(count));
    break;

  case GL_TEXTURE_2D_ARRAY:
    tex = ref_ptr<Texture>::manage(new Texture2DArray(count));
    ((Texture3D*)tex.get())->set_depth(depth_);
    break;

  case GL_TEXTURE_CUBE_MAP:
    tex = ref_ptr<Texture>::manage(new TextureCube(count));
    break;

  case GL_TEXTURE_3D:
    tex = ref_ptr<Texture>::manage(new Texture3D(count));
    ((Texture3D*)tex.get())->set_depth(depth_);
    break;

  default: // GL_TEXTURE_2D:
    tex = ref_ptr<Texture>::manage(new Texture2D(count));
    break;

  }
  tex->set_size(width_, height_);
  tex->set_format(format);
  tex->set_internalFormat(internalFormat);
  tex->set_pixelType(pixelType);
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
    GLuint width, GLuint height, GLuint depth)
{
  set_size(width, height);
  depth_ = depth;

  viewport_->setUniformData( Vec2f( (GLfloat)width, (GLfloat)height) );
  inverseViewport_->setUniformData( Vec2f( 1.0/(GLfloat)width, 1.0/(GLfloat)height) );
  bind();

  // resize depth attachment
  if(depthTexture_.get()!=NULL) {
    depthTexture_->set_size(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(depthTexture_.get());
    if(tex3D!=NULL) {
      tex3D->set_depth(depth);
    }
    depthTexture_->bind();
    depthTexture_->texImage();
  }

  // resize color attachments
  for(vector< ref_ptr<Texture> >::iterator
      it=colorBuffer_.begin(); it!=colorBuffer_.end(); ++it)
  {
    ref_ptr<Texture> &tex = *it;
    tex->set_size(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(tex.get());
    if(tex3D!=NULL) {
      tex3D->set_depth(depth);
    }
    for(GLuint i=0; i<tex->numBuffers(); ++i)
    {
      tex->bind();
      tex->texImage();
      tex->nextBuffer();
    }
  }

  // resize rbo attachments
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

const ref_ptr<ShaderInput2f>& FrameBufferObject::viewport() const
{ return viewport_; }
const ref_ptr<ShaderInput2f>& FrameBufferObject::inverseViewport() const
{ return inverseViewport_; }

GLuint FrameBufferObject::depth() const
{ return depth_; }
