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

#include <regen/gl-types/render-state.h>
#include <regen/gl-types/texture.h>
#include <regen/gl-types/buffer-object.h>
#include <regen/gl-types/rbo.h>
#include <regen/gl-types/shader-input.h>
#include <regen/utility/ref-ptr.h>

namespace regen {
/**
 * \brief Specifies a list of color buffers to be drawn into.
 */
struct DrawBuffers {
  /**
   * Draw into multiple buffer attachments.
   * @param buffers symbolic constants specifying the buffers
   *                into which fragment colors or data values will be written.
   */
  DrawBuffers(const vector<GLenum> &buffers)
  : buffers_(buffers) {}
  /**
   * Draw into single buffer attachment.
   * @param buffer symbolic constant specifying the buffer
   *               into which fragment colors or data values will be written.
   */
  DrawBuffers(const GLenum buffer)
  { buffers_.push_back(buffer); }
  DrawBuffers() {}
  /**
   * An array of symbolic constants specifying the buffers
   * into which fragment colors or data values will be written.
   */
  vector<GLenum> buffers_;
  /**
   * @param b another value.
   * @return false if values are component-wise equal
   */
  inline bool operator!=(const DrawBuffers &b) const
  {
    if(buffers_.size()!=b.buffers_.size()) return true;
    for(unsigned int i=0; i<buffers_.size(); ++i) {
      if(buffers_[i]!=b.buffers_[i]) return true;
    }
    return false;
  }

  /**
   * Indicates that no drawing can be
   * performed to color buffers on this framebuffer.
   */
  static DrawBuffers& none()
  { static DrawBuffers v(GL_NONE); return v; }
  /** Draw into scene front buffer. */
  static DrawBuffers& front()
  { static DrawBuffers v(GL_FRONT); return v; }
  /** Draw into scene back buffer. */
  static DrawBuffers& back()
  { static DrawBuffers v(GL_BACK); return v; }
  /** Draw into first color attachment. */
  static DrawBuffers& attachment0()
  { static DrawBuffers v(GL_COLOR_ATTACHMENT0); return v; }
};
} // namespace

namespace regen {
/**
 * \brief Framebuffer Objects are a mechanism for rendering to images
 * other than the default OpenGL Default Framebuffer.
 * They are OpenGL Objects that allow you to render directly
 * to textures, as well as blitting from one framebuffer to another.
 */
class FrameBufferObject : public RectBufferObject
{
public:
  struct Screen {
    Screen();
    // state stacks
    ValueStack<DrawBuffers> drawBuffers_;
    ValueStackAtomic<GLenum> readBuffer_;
  };

  static inline Screen& screen()
  { static Screen s; return s; }

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
   * Specifies a list of color buffers to be drawn into.
   */
  inline ValueStack<DrawBuffers>& drawBuffers()
  { return drawBuffers_; }
  /**
   * Select a color buffer source for pixels.
   */
  inline ValueStackAtomic<GLenum>& readBuffer()
  { return readBuffer_; }

  /**
   * Resizes all textures attached to this FBO.
   */
  void resize(GLuint width, GLuint height, GLuint depth);

  /**
   * @return the FBO viewport.
   */
  const ref_ptr<ShaderInput2f>& viewport() const;
  /**
   * @return convenience vector that can be passed to glViewport.
   */
  const Vec4ui& glViewport() const;
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
   * @return all added color attachments.
   */
  const DrawBuffers& colorBuffers();
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
   * Blit fbo to another fbo without any offset.
   * If sizes not match filtering is used and src stretched
   * to dst size.
   * This is not a simple copy of pixels, for example the blit can
   * resolve/downsample multisampled attachments.
   */
  void blitCopy(
      FrameBufferObject &dst,
      GLenum readAttachment,
      GLenum writeAttachment,
      GLbitfield mask=GL_COLOR_BUFFER_BIT,
      GLenum filter=GL_NEAREST);
  /**
   * Blit fbo attachment into screen back buffer.
   */
  void blitCopyToScreen(
      GLuint screenWidth, GLuint screenHeight,
      GLenum readAttachment,
      GLbitfield mask=GL_COLOR_BUFFER_BIT,
      GLenum filter=GL_NEAREST);

protected:
  // state stacks
  ValueStack<DrawBuffers> drawBuffers_;
  ValueStackAtomic<GLenum> readBuffer_;

  DrawBuffers colorBuffers_;
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
  Vec4ui glViewport_;

  inline void attachTexture(const ref_ptr<Texture> &tex, GLenum target) const
  { glFramebufferTextureEXT(GL_FRAMEBUFFER, target, tex->id(), 0); }
  inline void attachRenderBuffer(const ref_ptr<RenderBufferObject> &rbo, GLenum target) const
  { glFramebufferRenderbuffer(GL_FRAMEBUFFER, target, rbo->targetType(), rbo->id()); }
};
} // namespace

#endif /* GL_FBO_H_ */
