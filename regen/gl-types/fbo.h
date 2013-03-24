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

#include <regen/gl-types/texture.h>
#include <regen/gl-types/buffer-object.h>
#include <regen/gl-types/rbo.h>
#include <regen/gl-types/shader-input.h>
#include <regen/utility/ref-ptr.h>

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

  /**
   * Resizes all textures attached to this FBO.
   */
  void resize(GLuint width, GLuint height, GLuint depth);

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
   * @return the attached depth texture.
   */
  const ref_ptr<Texture>& depthTexture() const;
  /**
   * @return the attached stencil texture.
   */
  const ref_ptr<Texture>& stencilTexture() const;
  /**
   * @return the attached depth-stencil texture.
   */
  const ref_ptr<Texture>& depthStencilTexture() const;

  /**
   * List of attached textures.
   */
  vector< ref_ptr<Texture> >& colorBuffer();
  /**
   * List of attached textures.
   */
  vector< GLenum >& colorBuffers();
  /**
   * Returns texture associated to GL_COLOR_ATTACHMENT0.
   */
  const ref_ptr<Texture>& firstColorBuffer() const;

  /**
   * Add n RBO's to the FBO.
   */
  ref_ptr<RenderBufferObject> addRenderBuffer(GLuint count);
  /**
   * Add a RBO to the FBO.
   */
  GLenum addRenderBuffer(const ref_ptr<RenderBufferObject> &rbo);

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
   * Add a Texture to the FBO.
   */
  GLenum addTexture(const ref_ptr<Texture> &tex);

  /**
   * Sets depth attachment.
   */
  void set_depthAttachment(const ref_ptr<Texture> &tex);
  /**
   * Sets depth attachment.
   */
  void set_depthAttachment(const ref_ptr<RenderBufferObject> &rbo);
  /**
   * Sets stencil attachment.
   */
  void set_stencilTexture(const ref_ptr<Texture> &tex);
  /**
   * Sets stencil attachment.
   */
  void set_stencilTexture(const ref_ptr<RenderBufferObject> &rbo);
  /**
   * Sets depthStencil attachment.
   */
  void set_depthStencilTexture(const ref_ptr<Texture> &tex);
  /**
   * Sets depthStencil attachment.
   */
  void set_depthStencilTexture(const ref_ptr<RenderBufferObject> &rbo);

  /**
   * Sets the viewport to the FBO size.
   */
  inline void set_viewport() const
  { glViewport(0, 0, width_, height_); }

  /**
   * Enables all added color attachments.
   */
  inline void drawBuffers() const
  { drawBuffers(colorBuffers_.size(), &colorBuffers_[0]); }
  /**
   * Enables multiple color attachments.
   */
  inline void drawBuffers(vector<GLuint> &buffers) const
  { drawBuffers(buffers.size(), &buffers[0]); }
  /**
   * Enables multiple color attachments.
   */
  inline void drawBuffers(GLuint numBuffers, const GLuint *buffers) const
  { glDrawBuffers(numBuffers, buffers); }
  /**
   * Enables a single specified draw buffer.
   */
  inline void drawBuffer(GLuint colorBuffer) const
  { glDrawBuffer(colorBuffer); }
  /**
   * Disable drawing (useful if you only want depth values)
   */
  inline void drawBufferNone() const
  { glDrawBuffer(GL_NONE); }
  /**
   * Enables the back buffer of windowing systems fbo.
   */
  inline void drawBufferDefault() const
  { glDrawBuffer(GL_BACK); }

  /**
   * Bind the default framebuffer to a framebuffer target
   */
  inline void bindDefault(GLenum target=GL_FRAMEBUFFER) const
  { glBindFramebuffer(target, 0); }
  /**
   * Bind a framebuffer to a framebuffer target
   */
  inline void bind(GLenum target=GL_FRAMEBUFFER) const
  { glBindFramebuffer(target, ids_[bufferIndex_]); }
  /**
   * Bind a framebuffer to a framebuffer target
   * and set the viewport.
   */
  inline void activate() const {
    bind();
    set_viewport();
  }

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

protected:
  vector<GLenum> colorBuffers_;
  GLuint depth_;

  GLenum depthAttachmentTarget_;
  GLenum depthAttachmentFormat_;
  GLenum depthAttachmentType_;
  GLenum colorAttachmentFormat_;

  ref_ptr<Texture> depthTexture_;
  ref_ptr<Texture> stencilTexture_;
  ref_ptr<Texture> depthStencilTexture_;
  vector< ref_ptr<Texture> > colorBuffer_;
  vector< ref_ptr<RenderBufferObject> > renderBuffer_;

  ref_ptr<ShaderInput2f> viewport_;
  ref_ptr<ShaderInput2f> inverseViewport_;

  inline void attachTexture(const ref_ptr<Texture> &tex, GLenum target) const
  { glFramebufferTextureEXT(GL_FRAMEBUFFER, target, tex->id(), 0); }
  inline void attachRenderBuffer(const ref_ptr<RenderBufferObject> &rbo, GLenum target) const
  { glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, rbo->targetType(), rbo->id()); }
};
} // namespace

#endif /* GL_FBO_H_ */
