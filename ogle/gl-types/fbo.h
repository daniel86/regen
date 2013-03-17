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
#include <ogle/gl-types/shader-input.h>
#include <ogle/utility/ref-ptr.h>

namespace ogle {
/**
 * \brief Framebuffer Objects are a mechanism for rendering to images
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
      GLuint width, GLuint height, GLuint depth,
      GLenum depthTarget, GLenum depthFormat, GLenum depthType);
  virtual ~FrameBufferObject() {}

  /**
   * @return the FBO viewport.
   */
  const ref_ptr<ShaderInput2f>& viewport() const;
  /**
   * @return the target texel size.
   */
  const ref_ptr<ShaderInput2f>& inverseViewport() const;

  /**
   * @return depth of attachment textures.
   */
  GLuint depth() const;

  /**
   * Creates depth attachment.
   * @param target the depth target.
   * @param format the depth format.
   * @param type the depth type.
   */
  void createDepthTexture(GLenum target, GLenum format, GLenum type);
  /**
   * Returns GL_NONE if no depth buffer used else the depth
   * buffer format is returned (GL_DEPTH_COMPONENT_*).
   */
  GLenum depthAttachmentFormat() const;
  /**
   * The depth texture used or a NULL reference if no
   * depth buffer is used.
   */
  const ref_ptr<Texture>& depthTexture() const;

  /**
   * List of attached textures.
   */
  vector< ref_ptr<Texture> >& colorBuffer();
  /**
   * Returns texture associated to GL_COLOR_ATTACHMENT0.
   */
  const ref_ptr<Texture>& firstColorBuffer() const;

  /**
   * Add n RBO's to the FBO.
   */
  ref_ptr<RenderBufferObject> addRenderBuffer(GLuint count);
  /**
   * Add n Texture's to the FBO.
   */
  ref_ptr<Texture> addTexture(
      GLuint count,
      GLenum targetType,
      GLenum format,
      GLenum internalFormat,
      GLenum pixelType);

  /**
   * Adds color attachment.
   */
  GLenum addColorAttachment(const Texture &tex);
  /**
   * Adds color attachment.
   */
  GLenum addColorAttachment(const RenderBufferObject &rbo);

  /**
   * Resizes all textures generated for this FBO.
   */
  void resize(GLuint width, GLuint height, GLuint depth);

  /**
   * Sets depth attachment.
   */
  void set_depthAttachment(const Texture &tex) const;
  /**
   * Sets depth attachment.
   */
  void set_depthAttachment(const RenderBufferObject &rbo) const;
  /**
   * Sets stencil attachment.
   */
  void set_stencilTexture(const Texture &tex) const;
  /**
   * Sets stencil attachment.
   */
  void set_stencilTexture(const RenderBufferObject &rbo) const;
  /**
   * Sets depthStencil attachment.
   */
  void set_depthStencilTexture(const Texture &tex) const;
  /**
   * Sets depthStencil attachment.
   */
  void set_depthStencilTexture(const RenderBufferObject &rbo) const;

  /**
   * Blit fbo to another fbo without any offset.
   * If sizes not match filtering is used and src stretched
   * to dst size.
   * This is not a simple copy of pixels, for example the blit can
   * resolve/downsample multisampled attachments.
   */
  void blitCopy(FrameBufferObject &dst,
      GLenum readAttachment,
      GLenum writeAttachment,
      GLbitfield mask=GL_COLOR_BUFFER_BIT,
      GLenum filter=GL_NEAREST) const;
  /**
   * Blit fbo attachment onto screen.
   */
  void blitCopyToScreen(
      GLuint screenWidth, GLuint screenHeight,
      GLenum readAttachment,
      GLbitfield mask=GL_COLOR_BUFFER_BIT,
      GLenum filter=GL_NEAREST,
      GLenum screenBuffer=GL_BACK) const;

  /**
   * Sets the viewport to the FBO size.
   */
  inline void set_viewport() const {
    glViewport(0, 0, width_, height_);
  }

  /**
   * Enables all added color attachments.
   */
  inline void drawBuffers() const {
    glDrawBuffers(colorBuffers_.size(), &colorBuffers_[0]);
  }
  /**
   * Enables multiple color attachments.
   */
  inline void drawBufferMRT(vector<GLuint> &buffers) const {
    glDrawBuffers(buffers.size(), &buffers[0]);
  }
  /**
   * Enables multiple color attachments.
   */
  inline void drawBufferMRT(GLuint numBuffers, const GLuint *buffers) const {
    glDrawBuffers(numBuffers, buffers);
  }
  /**
   * Disable drawing (useful if you only want depth values)
   */
  inline void drawBufferNone() const {
    glDrawBuffer(GL_NONE);
  }
  /**
   * Enables a single specified draw buffer.
   */
  inline void drawBuffer(GLuint colorBuffer) const {
    glDrawBuffer(colorBuffer);
  }
  /**
   * Enables the back buffer of windowing systems fbo.
   */
  inline void drawBufferDefault() const {
    glDrawBuffer(GL_BACK);
  }

  /**
   * Bind the default framebuffer to a framebuffer target
   */
  inline void bindDefault(GLenum target=GL_FRAMEBUFFER) const {
    glBindFramebuffer(target, 0);
  }
  /**
   * Bind a framebuffer to a framebuffer target
   * and set the viewport.
   */
  inline void activate() const {
    glBindFramebuffer(GL_FRAMEBUFFER, ids_[bufferIndex_]);
    glViewport(0, 0, width_, height_);
  }
  /**
   * Bind a framebuffer to a framebuffer target
   */
  inline void bind(GLenum target) const {
    glBindFramebuffer(target, ids_[bufferIndex_]);
  }
  /**
   * Bind a framebuffer to the GL_FRAMEBUFFER target
   */
  inline void bind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, ids_[bufferIndex_]);
  }

  /**
   * Attaches a Texture.
   * @param tex the texture.
   * @param target the texture target.
   */
  inline void attachTexture(const Texture &tex, GLenum target) const {
    glFramebufferTextureEXT(GL_FRAMEBUFFER, target, tex.id(), 0);
  }
  /**
   * Attaches a RenderBufferObject.
   * @param rbo the texture.
   * @param target the texture target.
   */
  inline void attachRenderBuffer(const RenderBufferObject &rbo, GLenum target) const {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, rbo.targetType(), rbo.id());
  }

protected:
  vector<GLuint> colorBuffers_;
  GLuint depth_;

  GLenum depthAttachmentTarget_;
  GLenum depthAttachmentFormat_;
  GLenum depthAttachmentType_;
  GLenum colorAttachmentFormat_;

  ref_ptr<Texture> depthTexture_;
  vector< ref_ptr<Texture> > colorBuffer_;
  vector< ref_ptr<RenderBufferObject> > renderBuffer_;

  ref_ptr<ShaderInput2f> viewport_;
  ref_ptr<ShaderInput2f> inverseViewport_;
};
} // namespace

#endif /* GL_FBO_H_ */
