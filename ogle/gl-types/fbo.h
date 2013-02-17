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
      GLenum depthAttachmentFormat=GL_NONE);
  virtual ~FrameBufferObject() {}

  void createDepthTexture(GLenum format);
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
   * Add n TextureRectangle's to the FBO.
   */
  ref_ptr<Texture> addRectangleTexture(GLuint count,
      GLenum format=GL_RGBA, GLenum internalFormat=GL_RGBA);
  /**
   * Add n Texture's to the FBO.
   */
  ref_ptr<Texture> addTexture(GLuint count,
      GLenum format=GL_RGBA, GLenum internalFormat=GL_RGBA);

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
  void resize(GLuint width, GLuint height);

  /**
   * Sets depth attachment.
   */
  void set_depthAttachment(const Texture2D &tex) const;
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
   * Enables multiple color attachments.
   */
  inline void drawBufferMRT(vector<GLuint> &buffers) const {
    glDrawBuffers(buffers.size(), &buffers[0]);
  }
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

  inline void attachTexture(const Texture &tex, GLenum target) const {
    glFramebufferTextureEXT(GL_FRAMEBUFFER, target, tex.id(), 0);
  }
  inline void attachRenderBuffer(const RenderBufferObject &rbo, GLenum target) const {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, rbo.targetType(), rbo.id());
  }

protected:
  vector<GLuint> colorBuffers_;

  GLenum depthAttachmentFormat_;
  GLenum colorAttachmentFormat_;

  ref_ptr<Texture> depthTexture_;
  vector< ref_ptr<Texture> > colorBuffer_;
  vector< ref_ptr<RenderBufferObject> > renderBuffer_;
};

//////////
/////////


#include <ogle/algebra/vector.h>
#include <ogle/gl-types/texture.h>
#include <ogle/gl-types/shader-input.h>

class SimpleRenderTarget : public FrameBufferObject
{
public:
  enum PixelType {
    BYTE, F16, F32
  };

  /**
   * Create a texture.
   */
  static ref_ptr<Texture> createTexture(
      Vec3i size,
      GLint numComponents,
      GLint numTexs,
      PixelType pixelType);

  /**
   * Constructor that generates a texture based on
   * given parameters.
   */
  SimpleRenderTarget(
      const string &name,
      Vec3i size,
      GLuint numComponents,
      GLuint numTexs,
      PixelType pixelType);
  /**
   * Constructor that takes a previously allocated texture.
   */
  SimpleRenderTarget(
      const string &name,
      ref_ptr<Texture> &texture);

  const string& name();

  const ref_ptr<ShaderInputf>& inverseSize();

  /**
   * Texture attached to this buffer.
   */
  ref_ptr<Texture>& texture();

  /**
   * Clears all attached textures to zero.
   */
  void clear(const Vec4f &clearColor, GLint numBuffers);
  /**
   * Swap the active texture if there are multiple
   * attached textures.
   */
  void swap();

protected:
  string name_;
  ref_ptr<Texture> texture_;
  ref_ptr<ShaderInputf> inverseSize_;
  Vec3i size_;

  void initUniforms();
};

#endif /* GL_FBO_H_ */
