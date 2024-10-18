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

#include <regen/gl-types/gl-object.h>
#include <regen/gl-types/render-state.h>
#include <regen/gl-types/texture.h>
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
    DrawBuffers(const std::vector<GLenum> &buffers)
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
    std::vector<GLenum> buffers_;
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
   * \brief OpenGL Objects that contain images.
   *
   * RenderBuffer's are created and used specifically with Framebuffer Objects.
   * RenderBuffer's are optimized for being used as render targets,
   * while Textures may not be.
   */
  class RenderBuffer : public GLRectangle
  {
  public:
    /**
     * @param numObjects number of GL buffers.
     */
    RenderBuffer(GLuint numObjects=1);

    /**
     * Binds this RenderBuffer.
     * Call end when you are done.
     */
    void begin(RenderState *rs);
    /**
     * Complete previous call to begin.
     */
    void end(RenderState *rs);

    /**
     * Specifies the internal format to use for the renderbuffer object's image.
     * Accepted values are GL_R*, GL_RG*, GL_RGB* GL_RGBA*, GL_DEPTH_COMPONENT*,
     * GL_SRGB*, GL_COMPRESSED_*.
     */
    void set_format(GLenum format);

    /**
     * Establish data storage, format and dimensions
     * of a renderbuffer object's image using multisampling.
     */
    void storageMS(GLuint numMultisamples) const;
    /**
     * Establish data storage, format and dimensions of a
     * renderbuffer object's image
     */
    void storage() const;

  protected:
    GLenum format_;
  };
} // namespace

namespace regen {
  /**
   * \brief Framebuffer Objects are a mechanism for rendering to images
   * other than the default OpenGL Default Framebuffer.
   * They are OpenGL Objects that allow you to render directly
   * to textures, as well as blitting from one framebuffer to another.
   */
  class FBO : public GLRectangle
  {
  public:
    /**
     * \brief Contains state stacks for the screen buffer.
     */
    struct Screen {
      Screen();
      /** the active draw buffers. */
      ValueStackAtomic<GLenum> drawBuffer_;
      /** the active read buffer. */
      ValueStackAtomic<GLenum> readBuffer_;
    };

    /**
     * @return the screen buffer state set.
     */
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
    FBO(GLuint width, GLuint height, GLuint depth=1);

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
     * List of attached textures.
     */
    std::vector< ref_ptr<Texture> >& colorTextures();
    /**
     * List of attached RenderBuffer's.
     */
    std::vector< ref_ptr<RenderBuffer> >& renderBuffers();
    /**
     * Returns texture associated to GL_COLOR_ATTACHMENT0.
     */
    const ref_ptr<Texture>& firstColorTexture() const;
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
     * @return all added color attachments.
     */
    const DrawBuffers& colorBuffers();

    /**
     * Add n RenderBuffer's to the FBO.
     */
    ref_ptr<RenderBuffer> addRenderBuffer(GLuint count);
    /**
     * Add a RenderBuffer to the FBO.
     */
    GLenum addRenderBuffer(const ref_ptr<RenderBuffer> &rbo);

    /**
     * Create a texture using given parameters.
     * @param width the texture width.
     * @param height the texture height.
     * @param depth the texture depth.
     * @param count number of texture objects to generate.
     * @param targetType the texture type (GL_TEXTURE_2D,...).
     * @param format the texel format (GL_RGBA,...).
     * @param internalFormat internal texel format (GL_RGBA,...).
     * @param pixelType texel base type (GL_FLOAT,..).
     * @return the texture created.
     */
    static ref_ptr<Texture> createTexture(
        GLuint width,
        GLuint height,
        GLuint depth,
        GLuint count,
        GLenum targetType,
        GLenum format,
        GLint internalFormat,
        GLenum pixelType);
    /**
     * Add n Texture's to the FBO.
     */
    ref_ptr<Texture> addTexture(
        GLuint count,
        GLenum targetType,
        GLenum format,
        GLint internalFormat,
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
    void set_depthAttachment(const ref_ptr<RenderBuffer> &rbo);
    /**
     * Sets stencil attachment.
     */
    void set_stencilTexture(const ref_ptr<Texture> &tex);
    /**
     * Sets stencil attachment.
     */
    void set_stencilTexture(const ref_ptr<RenderBuffer> &rbo);
    /**
     * Sets depthStencil attachment.
     */
    void set_depthStencilTexture(const ref_ptr<Texture> &tex);
    /**
     * Sets depthStencil attachment.
     */
    void set_depthStencilTexture(const ref_ptr<RenderBuffer> &rbo);

    /**
     * Blit fbo to another fbo without any offset.
     * If sizes not match filtering is used and src stretched
     * to dst size.
     * This is not a simple copy of pixels, for example the blit can
     * resolve/downsample multisampled attachments.
     */
    void blitCopy(
        FBO &dst,
        GLenum readAttachment,
        GLenum writeAttachment,
        GLbitfield mask=GL_COLOR_BUFFER_BIT,
        GLenum filter=GL_NEAREST,
        GLboolean keepRatio=GL_FALSE);
    /**
     * Blit fbo attachment into screen back buffer.
     */
    void blitCopyToScreen(
        GLuint screenWidth, GLuint screenHeight,
        GLenum readAttachment,
        GLbitfield mask=GL_COLOR_BUFFER_BIT,
        GLenum filter=GL_NEAREST,
        GLboolean keepRatio=GL_FALSE);

  protected:
    // state stacks
    ValueStack<DrawBuffers> drawBuffers_;
    ValueStackAtomic<GLenum> readBuffer_;

    DrawBuffers colorBuffers_;
    GLuint depth_;

    GLenum depthAttachmentTarget_;
    GLenum depthAttachmentFormat_;
    GLenum depthAttachmentType_;

    std::vector< ref_ptr<Texture> > colorTextures_;
    std::vector< ref_ptr<RenderBuffer> > renderBuffers_;
    ref_ptr<Texture> depthTexture_;
    ref_ptr<Texture> stencilTexture_;
    ref_ptr<Texture> depthStencilTexture_;

    ref_ptr<ShaderInput2f> viewport_;
    ref_ptr<ShaderInput2f> inverseViewport_;
    Vec4ui glViewport_;
  };
} // namespace

#endif /* GL_FBO_H_ */
