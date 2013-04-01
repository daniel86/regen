/*
 * fbo.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/gl-types/render-state.h>
#include <regen/utility/gl-util.h>

#include "fbo.h"
using namespace regen;

static inline void __DrawBuffers(const DrawBuffers &v)
{ glDrawBuffers(v.buffers_.size(),&v.buffers_[0]); }

FrameBufferObject::FrameBufferObject(
    GLuint width, GLuint height, GLuint depth,
    GLenum depthTarget, GLenum depthFormat, GLenum depthType)
: RectBufferObject(glGenFramebuffers, glDeleteFramebuffers),
  drawBuffers_(__DrawBuffers),
  readBuffer_(glReadBuffer),
  depthAttachmentTarget_(depthTarget),
  depthAttachmentFormat_(depthFormat),
  depthAttachmentType_(depthType)
{
  set_size(width,height);
  depth_ = depth;

  if(depthAttachmentFormat_!=GL_NONE) {
    createDepthTexture(depthAttachmentTarget_,
        depthAttachmentFormat_, depthAttachmentType_);
  }

  viewport_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("viewport"));
  viewport_->setUniformData( Vec2f( (GLfloat)width, (GLfloat)height) );
  glViewport_ = Vec4ui(0,0,width,height);

  inverseViewport_ = ref_ptr<ShaderInput2f>::manage(new ShaderInput2f("inverseViewport"));
  inverseViewport_->setUniformData( Vec2f( 1.0/(GLfloat)width, 1.0/(GLfloat)height) );

  readBuffer_.push(GL_COLOR_ATTACHMENT0);
}

void FrameBufferObject::createDepthTexture(GLenum target, GLenum format, GLenum type)
{
  RenderState::get()->drawFrameBuffer().push(id());
  depthAttachmentTarget_ = target;
  depthAttachmentFormat_ = format;
  depthAttachmentType_ = type;

  ref_ptr<Texture> depth;
  if(target == GL_TEXTURE_CUBE_MAP) {
    depth = ref_ptr<Texture>::manage(new CubeMapDepthTexture);
  }
  else if(depth_>1) {
    depth = ref_ptr<Texture>::manage(new DepthTexture3D);
    ((Texture3D*)depth.get())->set_depth(depth_);
  }
  else {
    depth = ref_ptr<Texture>::manage(new DepthTexture2D);
  }
  depth-> set_targetType(target);
  depth->set_size(width_, height_);
  depth->set_internalFormat(format);
  depth->set_pixelType(type);

  RenderState::get()->textureChannel().push(GL_TEXTURE7);
  RenderState::get()->textureBind().push(7, TextureBind(depth->targetType(), depth->id()));
  depth->set_wrapping(GL_REPEAT);
  depth->set_filter(GL_LINEAR, GL_LINEAR);
  depth->set_compare(GL_NONE, GL_EQUAL);
  depth->texImage();
  RenderState::get()->textureBind().pop(7);
  RenderState::get()->textureChannel().pop();

  set_depthAttachment(depth);
  RenderState::get()->drawFrameBuffer().pop();
}

GLenum FrameBufferObject::depthAttachmentFormat() const
{
  return depthAttachmentFormat_;
}
vector< ref_ptr<Texture> >& FrameBufferObject::colorBuffer()
{
  return colorBuffer_;
}
const DrawBuffers& FrameBufferObject::colorBuffers()
{
  return colorBuffers_;
}
const ref_ptr<Texture>& FrameBufferObject::firstColorBuffer() const
{
  return colorBuffer_.front();
}

void FrameBufferObject::set_depthAttachment(const ref_ptr<Texture> &tex)
{
  attachTexture(tex, GL_DEPTH_ATTACHMENT);
  depthTexture_ = tex;
}
void FrameBufferObject::set_depthAttachment(const ref_ptr<RenderBufferObject> &rbo)
{
  attachRenderBuffer(rbo, GL_DEPTH_ATTACHMENT);
  depthTexture_ = ref_ptr<Texture>();
}
void FrameBufferObject::set_stencilTexture(const ref_ptr<Texture> &tex)
{
  attachTexture(tex, GL_STENCIL_ATTACHMENT);
  stencilTexture_ = tex;
}
void FrameBufferObject::set_stencilTexture(const ref_ptr<RenderBufferObject> &rbo)
{
  attachRenderBuffer(rbo, GL_STENCIL_ATTACHMENT);
  stencilTexture_ = ref_ptr<Texture>();
}
void FrameBufferObject::set_depthStencilTexture(const ref_ptr<Texture> &tex)
{
  attachTexture(tex, GL_DEPTH_STENCIL_ATTACHMENT);
  depthStencilTexture_ = tex;
}
void FrameBufferObject::set_depthStencilTexture(const ref_ptr<RenderBufferObject> &rbo)
{
  attachRenderBuffer(rbo, GL_DEPTH_STENCIL_ATTACHMENT);
  depthStencilTexture_ = ref_ptr<Texture>();
}

GLenum FrameBufferObject::addTexture(const ref_ptr<Texture> &tex)
{
  RenderState::get()->drawFrameBuffer().push(id());
  GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.buffers_.size();
  attachTexture(tex, attachment);
  colorBuffers_.buffers_.push_back(attachment);
  colorBuffer_.push_back(tex);
  RenderState::get()->drawFrameBuffer().pop();
  return attachment;
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
  RenderState::get()->textureChannel().push(GL_TEXTURE7);
  for(GLuint j=0; j<count; ++j) {
    RenderState::get()->textureBind().push(7, TextureBind(tex->targetType(), tex->id()));
    tex->set_wrapping(GL_CLAMP_TO_EDGE);
    tex->set_filter(GL_LINEAR, GL_LINEAR);
    tex->texImage();
    addTexture(tex);
    RenderState::get()->textureBind().pop(7);
    tex->nextBuffer();
  }
  RenderState::get()->textureChannel().pop();
  return tex;
}

GLenum FrameBufferObject::addRenderBuffer(const ref_ptr<RenderBufferObject> &rbo)
{
  RenderState::get()->drawFrameBuffer().push(id());
  GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.buffers_.size();
  attachRenderBuffer(rbo, attachment);
  colorBuffers_.buffers_.push_back(attachment);
  renderBuffer_.push_back(rbo);
  RenderState::get()->drawFrameBuffer().pop();
  return attachment;
}
ref_ptr<RenderBufferObject> FrameBufferObject::addRenderBuffer(GLuint count)
{
  ref_ptr<RenderBufferObject> rbo = ref_ptr<RenderBufferObject>::manage(new RenderBufferObject(count));
  rbo->set_size(width_, height_);
  for(GLuint j=0; j<count; ++j) {
    rbo->bind();
    rbo->storage();
    addRenderBuffer(rbo);
    rbo->nextBuffer();
  }
  return rbo;
}

void FrameBufferObject::blitCopy(
    FrameBufferObject &dst,
    GLenum readAttachment,
    GLenum writeAttachment,
    GLbitfield mask,
    GLenum filter)
{
  RenderState *rs = RenderState::get();
  // read from this
  rs->readFrameBuffer().push(id());
  readBuffer_.push(readAttachment);
  // write to dst
  rs->drawFrameBuffer().push(dst.id());
  dst.drawBuffers().push(DrawBuffers(writeAttachment));

  glBlitFramebuffer(
      0, 0, width(), height(),
      0, 0, dst.width(), dst.height(),
      mask, filter);

  dst.drawBuffers().pop();
  rs->drawFrameBuffer().pop();
  readBuffer_.pop();
  rs->readFrameBuffer().pop();
}

void FrameBufferObject::blitCopyToScreen(
    GLuint screenWidth, GLuint screenHeight,
    GLenum readAttachment,
    GLbitfield mask,
    GLenum filter)
{
  RenderState *rs = RenderState::get();
  // read from this
  rs->readFrameBuffer().push(id());
  readBuffer_.push(readAttachment);
  // write to screen front buffer
  rs->drawFrameBuffer().push(0);
  // XXX redundant
  glDrawBuffer(GL_FRONT);

  glBlitFramebuffer(
      0, 0, width(), height(),
      0, 0, screenWidth, screenHeight,
      mask, filter);

  rs->drawFrameBuffer().pop();
  readBuffer_.pop();
  rs->readFrameBuffer().pop();
}

void FrameBufferObject::resize(
    GLuint width, GLuint height, GLuint depth)
{
  set_size(width, height);
  depth_ = depth;

  viewport_->setUniformData( Vec2f( (GLfloat)width, (GLfloat)height) );
  inverseViewport_->setUniformData( Vec2f( 1.0/(GLfloat)width, 1.0/(GLfloat)height) );
  glViewport_ = Vec4ui(0,0,width,height);
  RenderState::get()->drawFrameBuffer().push(id());
  RenderState::get()->textureChannel().push(GL_TEXTURE7);

  // resize depth attachment
  if(depthTexture_.get()!=NULL) {
    depthTexture_->set_size(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(depthTexture_.get());
    if(tex3D!=NULL) {
      tex3D->set_depth(depth);
    }
    RenderState::get()->textureBind().push(7,
        TextureBind(depthTexture_->targetType(), depthTexture_->id()));
    depthTexture_->texImage();
    RenderState::get()->textureBind().pop(7);
  }

  // resize stencil attachment
  if(stencilTexture_.get()!=NULL) {
    stencilTexture_->set_size(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(stencilTexture_.get());
    if(tex3D!=NULL) {
      tex3D->set_depth(depth);
    }
    RenderState::get()->textureBind().push(7,
        TextureBind(stencilTexture_->targetType(), stencilTexture_->id()));
    stencilTexture_->texImage();
    RenderState::get()->textureBind().pop(7);
  }

  // resize depth stencil attachment
  if(depthStencilTexture_.get()!=NULL) {
    depthStencilTexture_->set_size(width_, height_);
    Texture3D *tex3D = dynamic_cast<Texture3D*>(depthStencilTexture_.get());
    if(tex3D!=NULL) {
      tex3D->set_depth(depth);
    }
    RenderState::get()->textureBind().push(7,
        TextureBind(depthStencilTexture_->targetType(), depthStencilTexture_->id()));
    depthStencilTexture_->texImage();
    RenderState::get()->textureBind().pop(7);
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
      RenderState::get()->textureBind().push(7,
          TextureBind(tex->targetType(), tex->id()));
      tex->texImage();
      RenderState::get()->textureBind().pop(7);
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

  RenderState::get()->textureChannel().pop();
  RenderState::get()->drawFrameBuffer().pop();
}

const ref_ptr<Texture>& FrameBufferObject::depthTexture() const
{ return depthTexture_; }
const ref_ptr<Texture>& FrameBufferObject::stencilTexture() const
{ return stencilTexture_; }
const ref_ptr<Texture>& FrameBufferObject::depthStencilTexture() const
{ return depthStencilTexture_; }

const ref_ptr<ShaderInput2f>& FrameBufferObject::viewport() const
{ return viewport_; }
const Vec4ui& FrameBufferObject::glViewport() const
{ return glViewport_; }
const ref_ptr<ShaderInput2f>& FrameBufferObject::inverseViewport() const
{ return inverseViewport_; }

GLuint FrameBufferObject::depth() const
{ return depth_; }
