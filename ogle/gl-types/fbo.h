/*
 * fbo.h
 *
 *  Created on: 04.02.2011
 *      Author: daniel
 */

#ifndef _FBO_H_
#define _FBO_H_

#include <vector>
#include <list>

#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/buffer-object.h>
#include <ogle/gl-types/rbo.h>
#include <ogle/utility/ref-ptr.h>

/**
 * Framebuffer Objects are a mechanism for rendering to images
 * other than the default OpenGL Default Framebuffer.
 * They are OpenGL Objects that allow you to render directly
 * to textures, as well as blitting from one framebuffer to another.
 */
class FrameBufferObject : public RectBufferObject
{
public:
  /**
   * Default constructor.
   * Specifies the dimension and formats that
   * will be used for textures attached to the FBO.
   * Note that dimensions must be the same
   * for all attached textured and formats of
   * all attached draw buffer must be equal.
   */
  FrameBufferObject(
      GLuint width, GLuint height,
      GLenum colorAttachmentFormat=GL_RGBA,
      GLenum depthAttachmentFormat=GL_NONE);

  /**
   * Add n RBO's to the FBO.
   */
  ref_ptr<RenderBufferObject> addRenderBuffer(GLuint count);
  /**
   * Add n TextureRectangle's to the FBO.
   */
  ref_ptr<Texture> addRectangleTexture(GLuint count);
  /**
   * Add n Texture's to the FBO.
   */
  ref_ptr<Texture> addTexture(GLuint count);

  /**
   * Resizes all textures generated for this FBO.
   */
  void resize(GLuint width, GLuint height);

  /**
   * Sets the viewport to the FBO size.
   */
  inline void set_viewport() {
    glViewport(0, 0, width_, height_);
  }

  /**
   * Sets depth attachment.
   */
  static inline void set_depthAttachment(const Texture2D &tex) {
    attachTexture(tex, GL_DEPTH_ATTACHMENT);
  }
  /**
   * Sets depth attachment.
   */
  static inline void set_depthAttachment(const RenderBufferObject &rbo) {
    renderbuffer(rbo, GL_DEPTH_ATTACHMENT);
  }
  /**
   * Sets stencil attachment.
   */
  static inline void set_stencilTexture(const Texture &tex) {
    attachTexture(tex, GL_STENCIL_ATTACHMENT);
  }
  /**
   * Sets stencil attachment.
   */
  static inline void set_stencilTexture(const RenderBufferObject &rbo) {
    renderbuffer(rbo, GL_STENCIL_ATTACHMENT);
  }
  /**
   * Sets depthStencil attachment.
   */
  static inline void set_depthStencilTexture(const Texture &tex) {
    attachTexture(tex, GL_DEPTH_STENCIL_ATTACHMENT);
  }
  /**
   * Sets depthStencil attachment.
   */
  static inline void set_depthStencilTexture(const RenderBufferObject &rbo) {
    renderbuffer(rbo, GL_DEPTH_STENCIL_ATTACHMENT);
  }
  /**
   * Adds color attachment.
   */
  inline GLenum addColorAttachment(const Texture &tex) {
    GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.size();
    attachTexture(tex, attachment);
    colorBuffers_.push_back(attachment);
    return attachment;
  }
  /**
   * Adds color attachment.
   */
  inline GLenum addColorAttachment(const RenderBufferObject &rbo) {
    GLenum attachment = GL_COLOR_ATTACHMENT0 + colorBuffers_.size();
    renderbuffer(rbo, attachment);
    colorBuffers_.push_back(attachment);
    return attachment;
  }

  /**
   * Use texture2D attachment.
   */
  static inline void attachTexture(const Texture &tex, GLenum target) {
    glFramebufferTextureEXT(
        GL_FRAMEBUFFER,
        target,
        tex.id(),
        0);
  }
  /**
   * Use renderbuffer attachment.
   */
  static inline void renderbuffer(const RenderBufferObject &rbo, GLenum target) {
    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        target,
        rbo.targetType(),
        rbo.id());
  }

  /**
   * Enables multiple color attachments.
   */
  inline virtual void drawBufferMRT(vector<GLuint> &buffers) {
    glDrawBuffers(buffers.size(), &buffers[0]);
  }
  /**
   * Disable drawing (useful if you only want depth values)
   */
  static inline void drawBufferNone() {
    glDrawBuffer(GL_NONE);
  }
  /**
   * Enables a single specified draw buffer.
   */
  static inline void drawBuffer(GLuint colorBuffer) {
    glDrawBuffer(colorBuffer);
  }
  /**
   * Enables the back buffer of windowing systems fbo.
   */
  static inline void drawBufferDefault() {
    glDrawBuffer(GL_BACK);
  }

  /**
   * Blit fbo to another fbo without any offset.
   * If sizes not match filtering is used and src stretched
   * to dst size.
   * This is not a simple copy of pixels, for example the blit can
   * resolve/downsample multisampled attachments.
   */
  static inline void blitCopy(FrameBufferObject &src, FrameBufferObject &dst,
      GLenum readAttachment=GL_COLOR_ATTACHMENT0,
      GLenum writeAttachment=GL_COLOR_ATTACHMENT0,
      GLbitfield mask=GL_COLOR_BUFFER_BIT,
      GLenum filter=GL_NEAREST)
  {
    src.bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(readAttachment);
    dst.bind(GL_DRAW_FRAMEBUFFER);
    glDrawBuffer(writeAttachment);
    glBlitFramebuffer(
        0, 0, src.width(), src.height(),
        0, 0, dst.width(), dst.height(),
        mask, filter);
  }
  /**
   * Blit fbo attachment onto screen.
   */
  static inline void blitCopyToScreen(FrameBufferObject &src,
      GLuint screenWidth, GLuint screenHeight,
      GLenum readAttachment=GL_COLOR_ATTACHMENT0,
      GLbitfield mask=GL_COLOR_BUFFER_BIT,
      GLenum filter=GL_NEAREST,
      GLenum screenBuffer=GL_BACK)
  {
    src.bind(GL_READ_FRAMEBUFFER);
    glReadBuffer(readAttachment);
    bindDefault(GL_DRAW_FRAMEBUFFER);
    glDrawBuffer(screenBuffer);
    glBlitFramebuffer(
        0, 0, src.width(), src.height(),
        0, 0, screenWidth, screenHeight,
        mask, filter);
  }

  /**
   * Bind the default framebuffer to a framebuffer target
   */
  static inline void bindDefault(GLenum target=GL_FRAMEBUFFER) {
    glBindFramebuffer( target, 0 );
  }

  /**
   * Bind a framebuffer to a framebuffer target
   */
  inline void bind(GLenum target) const {
    glBindFramebuffer( target, ids_[bufferIndex_] );
  }
  /**
   * Bind a framebuffer to the GL_FRAMEBUFFER target
   */
  inline void bind() const {
    glBindFramebuffer( GL_FRAMEBUFFER, ids_[bufferIndex_] );
  }

protected:
  vector<GLuint> colorBuffers_;

  GLenum depthAttachmentFormat_;
  GLenum colorAttachmentFormat_;

  ref_ptr<DepthTexture2D> depthTexture_;
  list< ref_ptr<Texture> > colorBuffer_;
  list< ref_ptr<RenderBufferObject> > renderBuffer_;
};


#endif /* GL_FBO_H_ */
