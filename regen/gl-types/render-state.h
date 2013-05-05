/*
 * render-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef RENDER_STATE_H_
#define RENDER_STATE_H_

#include <vector>
using namespace std;

#include <regen/gl-types/state-stacks.h>

namespace regen {

  typedef Vec4ui Viewport;
  typedef Vec4ui Scissor;
  typedef Vec4i BlendFunction;
  typedef Vec2i BlendEquation;
  typedef Vec3i StencilOp;
  typedef Vec2d DepthRange;
  typedef Vec4b ColorMask;
  typedef Vec4f ClearColor;
  /**
   * \brief Set front and back function and reference value for stencil testing.
   */
  struct StencilFunc {
    /** specifies the test function. Accepted values are
     * GL_NEVER,GL_LESS,GL_LEQUAL,GL_GREATER,GL_GEQUAL,GL_EQUAL,
     * GL_NOTEQUAL,GL_ALWAYS. The initial value is GL_ALWAYS. */
    GLenum func_;
    /** specifies the reference value for the stencil test.
     * The initial value is 0. */
    GLint ref_;
    /** specifies a mask that is ANDed with both the reference value
     * and the stored stencil value when the test is done.
     * The initial value is all 1's. */
    GLuint mask_;
    /**
     * @param b another value.
     * @return false if values are component-wise equal
     */
    inline bool operator!=(const StencilFunc &b) const
    { return func_!=b.func_ || ref_!=b.ref_ || mask_!=b.mask_; }
  };
  /**
   * \brief Specifies the default outer or inner tessellation levels
   * to be used when no tessellation control shader is present.
   */
  struct PatchLevels {
    /**
     * Specifies the default outer or inner tessellation levels
     * to be used when no tessellation control shader is present.
     * @param inner inner level.
     * @param outer outer level.
     */
    PatchLevels(const Vec4f &inner, const Vec4f &outer)
    : inner_(inner), outer_(outer) {}
    PatchLevels()
    : inner_(Vec4f(0.0)), outer_(Vec4f(0.0)) {}
    /** inner level */
    Vec4f inner_;
    /** outer level */
    Vec4f outer_;
    /**
     * @param b another value.
     * @return false if values are component-wise equal
     */
    inline bool operator!=(const PatchLevels &b) const
    { return inner_!=b.inner_ || outer_!=b.outer_; }
  };
  /**
   * \brief Bind a named texture to a texturing target
   */
  struct TextureBind {
    /**
     * @param target Specifies the target to which the texture is bound.
     * @param id Specifies the name of a texture.
     */
    TextureBind(GLenum target,GLuint id)
    : target_(target), id_(id) {}
    TextureBind()
    : target_(GL_TEXTURE_2D), id_(0) {}
    /** Specifies the target to which the texture is bound. */
    GLenum target_;
    /** Specifies the name of a texture. */
    GLuint id_;
    /**
     * @param b another value.
     * @return false if values are component-wise equal
     */
    inline bool operator!=(const TextureBind &b) const
    { return id_!=b.id_ || target_!=b.target_; }
  };
  /**
   * \brief Bind a range within a buffer object to an indexed buffer target.
   */
  struct BufferRange {
    /**
     * @param buffer The name of a buffer object to bind to the specified binding point.
     * @param offset The starting offset in basic machine units into the buffer object buffer.
     * @param size The amount of data in machine units that can be read from the buffet object while used as an indexed target.
     */
    BufferRange(GLuint buffer, GLintptr offset, GLsizeiptr size)
    : buffer_(buffer), offset_(offset), size_(size) {}
    BufferRange()
    : buffer_(0), offset_(0), size_(0) {}
    /** The name of a buffer object to bind to the specified binding point. */
    GLuint buffer_;
    /** The starting offset in basic machine units into the buffer object buffer. */
    GLintptr offset_;
    /** The amount of data in machine units that can be read from the buffet object while used as an indexed target. */
    GLsizeiptr size_;
    /**
     * @param b another value.
     * @return false if values are component-wise equal
     */
    inline bool operator!=(const BufferRange &b) const
    { return buffer_!=b.buffer_ || offset_!=b.offset_ || size_!=b.size_; }
  };

  /**
   * \brief Handles server-side GL states.
   *
   * Each state is implemented using a stack with push()
   * and pop() defined.
   */
  class RenderState
  {
  public:
    /**
     * \brief States that can be affected by glEnable/glDisable
     */
    enum Toggle {
      /**
       * If enabled, blend the computed fragment color values with the values in the color buffers.
       */
      BLEND=0,
      /**
       * If enabled, apply the currently selected logical operation to the
       * computed fragment color and color buffer values.
       */
      COLOR_LOGIC_OP,
      /**
       * If enabled,cull polygons based on their winding in window coordinates.
       */
      CULL_FACE,
      /**
       * If enabled, debug messages are produced by a debug context. When disabled,
       * the debug message log is silenced. Note that in a non-debug context, very
       * few, if any messages might be produced, even when GL_DEBUG_OUTPUT is enabled.
       */
      DEBUG_OUTPUT,
      /**
       * If enabled, debug messages are produced synchronously by a debug context. If disabled,
       * debug messages may be produced asynchronously. In particular, they may be delayed relative
       * to the execution of GL commands, and the debug callback function may be called from
       * a thread other than that in which the commands are executed.
       */
      DEBUG_OUTPUT_SYNCHRONOUS,
      /**
       * If enabled, the -wc<=zc<=wc plane equation is ignored
       * by view volume clipping (effectively, there is no near or far plane clipping).
       */
      DEPTH_CLAMP,
      /**
       * If enabled, do depth comparisons and update the depth buffer.
       * Note that even if the depth buffer exists and the depth mask is non-zero,
       * the depth buffer is not updated if the depth test is disabled.
       */
      DEPTH_TEST,
      /**
       * If enabled, dither color components or indices before they are written to the color buffer.
       */
      DITHER,
      /**
       * If enabled and the value of GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING for the
       * framebuffer attachment corresponding to the destination buffer is GL_SRGB,
       * the R, G, and B destination color values (after conversion from fixed-point to floating-point)
       * are considered to be encoded for the sRGB color space and hence are linearized prior to
       * their use in blending.
       */
      FRAMEBUFFER_SRGB,
      /**
       * If enabled, draw lines with correct filtering. Otherwise, draw aliased lines.
       */
      LINE_SMOOTH,
      /**
       * If enabled, use multiple fragment samples in computing the final color of a pixel.
       */
      MULTISAMPLE,
      /**
       * If enabled, and if the polygon is rendered in
       * GL_FILL mode, an offset is added to depth values of a polygon's
       * fragments before the depth comparison is performed.
       */
      POLYGON_OFFSET_FILL,
      /**
       * If enabled, and if the polygon is rendered in
       * GL_LINE mode, an offset is added to depth values of a polygon's
       * fragments before the depth comparison is performed.
       */
      POLYGON_OFFSET_LINE,
      /**
       * If enabled, an offset is added to depth values of a polygon's fragments
       * before the depth comparison is performed, if the polygon is rendered in
       * GL_POINT mode.
       */
      POLYGON_OFFSET_POINT,
      /**
       * If enabled, draw polygons with proper filtering.
       * Otherwise, draw aliased polygons. For correct antialiased polygons,
       * an alpha buffer is needed and the polygons must be sorted front to
       * back.
       */
      POLYGON_SMOOTH,
      /**
       * Enables primitive restarting.  If enabled, any one of the draw commands
       * which transfers a set of generic attribute array elements to the GL will restart
       * the primitive when the index of the vertex is equal to the primitive restart index.
       */
      PRIMITIVE_RESTART,
      /**
       * Enables primitive restarting with a fixed index. If enabled, any one of the
       * draw commands which transfers a set of generic attribute array elements to the GL will
       * restart the primitive when the index of the vertex is equal to the fixed primitive
       * index for the specified index type.
       */
      PRIMITIVE_RESTART_FIXED_INDEX,
      /**
       * Close shader pipeline before rastarizing.
       */
      RASTARIZER_DISCARD,
      /**
       * If enabled, compute a temporary coverage value where each bit is determined by the
       * alpha value at the corresponding sample location.  The temporary coverage
       * value is then ANDed with the fragment coverage value.
       */
      SAMPLE_ALPHA_TO_COVERAGE,
      /**
       * If enabled, each sample alpha value is replaced by the maximum representable alpha value.
       */
      SAMPLE_ALPHA_TO_ONE,
      /**
       * If enabled,
       * the fragment's coverage is ANDed with the temporary coverage value.  If
       * GL_SAMPLE_COVERAGE_INVERT is set to GL_TRUE, invert the coverage value.
       */
      SAMPLE_COVERAGE,
      /**
       * If enabled, the active fragment shader is run once for each covered sample, or at
       * fraction of this rate as determined by the current value of GL_MIN_SAMPLE_SHADING_VALUE.
       */
      SAMPLE_SHADING,
      /**
       * If enabled, the sample coverage mask generated for a fragment during rasterization
       * will be ANDed with the value of GL_SAMPLE_MASK_VALUE before
       * shading occurs.
       */
      SAMPLE_MASK,
      /**
       * If enabled, discard fragments that are outside the scissor rectangle.
       */
      SCISSOR_TEST,
      /**
       * If enabled, do stencil testing and update the stencil buffer.
       */
      STENCIL_TEST,
      /**
       * If enabled, cubemap textures are sampled such that when linearly sampling from the border
       * between two adjacent faces, texels from both faces are used to generate the final sample
       * value. When disabled, texels from only a single face are used to construct the final
       * sample value.
       */
      TEXTURE_CUBE_MAP_SEAMLESS,
      /**
       * If enabled and a vertex or geometry shader is active,
       * then the derived point size is taken from the (potentially clipped) shader builtin
       * gl_PointSize and clamped to the implementation-dependent point size range.
       */
      PROGRAM_POINT_SIZE,
      /**
       * If enabled, clip geometry against user-defined half space i.
       */
      CLIP_DISTANCE0,
      CLIP_DISTANCE1,
      CLIP_DISTANCE2,
      CLIP_DISTANCE3,
      TOGGLE_STATE_LAST
    };
    /**
     * @param t Toggle value
     * @return GL enumeration.
     */
    static GLenum toggleToID(Toggle t);

    /**
     * @return singleton RenderState.
     */
    static RenderState* get();

    /**
     * Returns true if a transform feedback operation was started.
     */
    inline GLboolean isTransformFeedbackAcive() const
    { return feedbackCount_>0; }
    /**
     * Start transform feedback operation.
     * Silently do nothing is transform feedback already is active.
     */
    inline void beginTransformFeedback(GLenum v) {
      if(feedbackCount_==0) {
        glBeginTransformFeedback(v);
      }
      feedbackCount_ += 1;
    }
    /**
     * End transform feedback operation.
     * Silently do nothing is transform feedback already is active.
     */
    inline void endTransformFeedback() {
      feedbackCount_ -= 1;
      if(feedbackCount_==0) {
        glEndTransformFeedback();
      }
    }

    /**
     * Enable or disable server-side GL capabilities.
     */
    inline IndexedValueStack<GLboolean>& toggles()
    { return toggles_; }

    /**
     * bind a named buffer object to GL_ARRAY_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& arrayBuffer()
    { return arrayBuffer_; }
    /**
     * bind a named buffer object to GL_ELEMENT_ARRAY_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& elementArrayBuffer()
    { return elementArrayBuffer_; }
    /**
     * bind a named buffer object to GL_PIXEL_PACK_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& pixelPackBuffer()
    { return pixelPackBuffer_; }
    /**
     * bind a named buffer object to GL_PIXEL_UNPACK_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& pixelUnpackBuffer()
    { return pixelUnpackBuffer_; }
    /**
     * bind a named buffer object to GL_DISPATCH_INDIRECT_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& dispatchIndirectBuffer()
    { return dispatchIndirectBuffer_; }
    /**
     * bind a named buffer object to GL_DRAW_INDIRECT_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& drawIndirectBuffer()
    { return drawIndirectBuffer_; }
    /**
     * bind a named buffer object to GL_TEXTURE_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& textureBuffer()
    { return textureBuffer_; }
    /**
     * bind a named buffer object to GL_COPY_READ_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& copyReadBuffer()
    { return copyReadBuffer_; }
    /**
     * bind a named buffer object to GL_COPY_WRITE_BUFFER target.
     */
    inline ParameterStackAtomic<GLuint>& copyWriteBuffer()
    { return copyWriteBuffer_; }
    /**
     * Vertex Array Objects (VAO) are OpenGL Objects that store the
     * set of bindings between Vertex Attributes and the user's source vertex data.
     */
    inline ValueStackAtomic<GLuint>& vao()
    { return vao_; }

    /**
     * bind a named buffer object to GL_UNIFORM_BUFFER target.
     */
    inline IndexedValueStack<BufferRange>& uniformBufferRange()
    { return uniformBufferRange_; }
    /**
     * bind a named buffer object to GL_TRANSFORM_FEEDBACK_BUFFER target.
     */
    inline IndexedValueStack<BufferRange>& feedbackBufferRange()
    { return feedbackBufferRange_; }
    /**
     * bind a named buffer object to GL_ATOMIC_COUNTER_BUFFER target.
     */
    inline IndexedValueStack<BufferRange>& atomicCounterBufferRange()
    { return atomicCounterBufferRange_; }
    /**
     * bind a named buffer object to GL_SHADER_STORAGE_BUFFER target.
     */
    inline IndexedValueStack<BufferRange>& shaderStorageBufferRange()
    { return shaderStorageBufferRange_; }

    /**
     * Bind a framebuffer to the framebuffer read target.
     */
    inline ParameterStackAtomic<GLuint>& readFrameBuffer()
    { return readFrameBuffer_; }
    /**
     * Bind a framebuffer to the framebuffer draw target.
     */
    inline ParameterStackAtomic<GLuint>& drawFrameBuffer()
    { return drawFrameBuffer_; }
    /**
     * Set the framebuffer viewport.
     */
    inline ValueStack<Viewport>& viewport()
    { return viewport_; }

    /**
     * The texture stack.
     */
    inline IndexedValueStack<TextureBind>& textures()
    { return textures_; }
    /**
     * Selects active texture unit.
     */
    inline ValueStackAtomic<GLenum>& activeTexture()
    { return activeTexture_; }

    /**
     * Reserves next texture channel.
     */
    inline GLuint reserveTextureChannel()
    { return min(textureCounter_++,maxTextureUnits_-1); }
    /**
     * Releases last reserved texture channel.
     */
    inline void releaseTextureChannel()
    { --textureCounter_; }

    /**
     * Installs a program object as part of current rendering state.
     */
    inline ValueStackAtomic<GLuint>& shader()
    { return shader_; }

    /**
     * Define the scissor box for a specific viewport.
     * 'index' specifies the index of the viewport whose scissor box to modify.
     * 'left', 'bottom' specify the coordinate of the bottom left corner
     * of the scissor box, in pixels.
     * 'width', 'height' specifies the dimensions of the scissor box, in pixels.
     */
    inline IndexedValueStack<Scissor>& scissor()
    { return scissor_; }

    /**
     * Specifies whether front- or back-facing facets are candidates for culling.
     * Symbolic constants GL_FRONT,GL_BACK, GL_FRONT_AND_BACK are accepted.
     * The initial value is GL_BACK.
     */
    inline ValueStackAtomic<GLenum>& cullFace()
    { return cullFace_; }
    /**
     * Specifies the orientation of front-facing polygons.
     * GL_CW and GL_CCW are accepted. The initial value is GL_CCW.
     */
    inline ValueStackAtomic<GLenum>& frontFace()
    { return frontFace_; }

    /**
     * Specifies whether the depth buffer is enabled for writing.
     * If flag is GL_FALSE, depth buffer writing is disabled.
     * Otherwise, it is enabled. Initially, depth buffer writing is enabled.
     */
    inline ValueStackAtomic<GLboolean>& depthMask()
    { return depthMask_; }
    /**
     * Specifies the depth comparison function. Symbolic constants
     * GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,GL_GREATER,
     * GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS are accepted.
     * The initial value is GL_LESS.
     */
    inline ValueStackAtomic<GLenum>& depthFunc()
    { return depthFunc_; }
    /**
     * The clear depth value.
     */
    inline ValueStackAtomic<GLclampd>& depthClear()
    { return depthClear_; }
    /**
     * Specify mapping of depth values from normalized device coordinates
     * to window coordinates.
     * 'nearVal' specifies the mapping of the near clipping plane to window coordinates.
     * The initial value is 0.
     * 'farVal' specifies the mapping of the far clipping plane to window coordinates.
     * The initial value is 1.
     */
    inline IndexedValueStack<DepthRange>& depthRange()
    { return depthRange_; }

    /**
     * Set the blend color.
     * Initially the GL_BLEND_COLOR is set to (0,0,0,0).
     */
    inline ValueStack<Vec4f>& blendColor()
    { return blendColor_; }
    /**
     * Specify the equation used for both the RGB blend equation and the
     * Alpha blend equation.
     * 'buf' specifies the index of the draw buffer for which to set the blend equation.
     * 'mode' specifies how source and destination colors are combined.
     * It must be GL_FUNC_ADD,GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT,GL_MIN,GL_MAX.
     * Initially, both the RGB blend equation and the alpha blend equation
     * are set to GL_FUNC_ADD.
     */
    inline IndexedValueStack<BlendEquation>& blendEquation()
    { return blendEquation_; }
    /**
     * Specify pixel arithmetic.
     * 'buf' specifies the index of the draw buffer for which to set the blend function.
     * v.xz-'sfactor' specifies how the red, green, blue, and alpha source blending
     * factors are computed. The initial value is GL_ONE.
     * v.yw-'dfactor' specifies how the red, green, blue, and alpha destination
     * blending factors are computed.
     * The following symbolic constants are accepted:
     * GL_ZERO,GL_ONE,GL_SRC_COLOR,GL_ONE_MINUS_SRC_COLOR,GL_DST_COLOR,
     * GL_ONE_MINUS_DST_COLOR,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,
     * GL_DST_ALPHA,GL_ONE_MINUS_DST_ALPHA.GL_CONSTANT_COLOR,
     * GL_ONE_MINUS_CONSTANT_COLOR,GL_CONSTANT_ALPHA,
     * GL_ONE_MINUS_CONSTANT_ALPHA. The initial value is GL_ZERO.
     */
    inline IndexedValueStack<BlendFunction>& blendFunction()
    { return blendFunc_; }

    /**
     * Set front and back function and reference value for stencil testing.
     * v.'func' specifies the test function. Accepted values are
     * GL_NEVER,GL_LESS,GL_LEQUAL,GL_GREATER,GL_GEQUAL,GL_EQUAL,
     * GL_NOTEQUAL,GL_ALWAYS. The initial value is GL_ALWAYS.
     * v.'ref' specifies the reference value for the stencil test.
     * The initial value is 0.
     * v.'mask' specifies a mask that is ANDed with both the reference value
     * and the stored stencil value when the test is done.
     * The initial value is all 1's.
     */
    inline ValueStack<StencilFunc>& stencilFunc()
    { return stencilFunc_; }
    /**
     * Specifies a bit mask to enable and disable writing of individual bits
     * in the stencil planes. Initially, the mask is all 1's.
     */
    inline ValueStackAtomic<GLuint>& stencilMask()
    { return stencilMask_; }
    /**
     * Set front and back stencil test actions.
     * v.x-'sfail' specifies the action to take when the stencil test fails.
     * Accepted values are GL_KEEP,GL_ZERO,GL_REPLACE,GL_INCR,GL_INCR_WRAP,
     * GL_DECR,GL_DECR_WRAP,GL_INVERT. The initial value is GL_KEEP.
     * v.y-'dpfail' specifies the stencil action when the stencil test passes,
     * but the depth test fails. dpfail accepts the same symbolic constants
     * as sfail. The initial value is GL_KEEP.
     * v.z-'dppass' specifies the stencil action when both the stencil test
     * and the depth test pass, or when the stencil test passes and
     * either there is no depth buffer or depth testing is not enabled.
     * dppass accepts the same symbolic constants as sfail.
     * The initial value is GL_KEEP.
     */
    inline ValueStack<StencilOp>& stencilOp()
    { return stencilOp_; }

    /**
     * Specifies how polygons will be rasterized.
     * Accepted values are GL_POINT,GL_LINE,GL_FILL.
     * The initial value is GL_FILL for both front- and back-facing polygons.
     */
    inline ParameterStackAtomic<GLenum>& polygonMode()
    { return polygonMode_; }
    /**
     * Set the scale and units used to calculate depth values.
     * v.x-'factor' specifies a scale factor that is used to create a variable
     * depth offset for each polygon. The initial value is 0.
     * v.y-'units' is multiplied by an implementation-specific value to
     * create a constant depth offset. The initial value is 0.
     */
    inline ValueStack<Vec2f>& polygonOffset()
    { return polygonOffset_; }

    /**
     * Specify the diameter of rasterized points.
     * The initial value is 1.
     */
    inline ValueStackAtomic<GLfloat>& pointSize()
    { return pointSize_; }

    /**
     * Specifies the threshold value to which point sizes are clamped
     * if they exceed the specified value. The default value is 1.0.
     */
    inline ParameterStackAtomic<GLfloat>& pointFadeThreshold()
    { return pointFadeThreshold_; }
    /**
     * Specify the point sprite texture coordinate origin,
     * either GL_LOWER_LEFT or GL_UPPER_LEFT.
     * The default value is GL_UPPER_LEFT.
     */
    inline ParameterStackAtomic<GLint>& pointSpriteOrigin()
    { return pointSpriteOrigin_; }

    /**
     * Specifies the number of vertices that
     * will be used to make up a single patch primitive.
     */
    inline ParameterStackAtomic<GLint>& patchVertices()
    { return patchVertices_; }
    /**
     * Specifies the default outer or inner tessellation levels
     * to be used when no tessellation control shader is present.
     */
    inline ValueStack<PatchLevels>& patchLevel()
    { return patchLevel_; }

    /**
     * Enable and disable writing of frame buffer color components.
     */
    inline IndexedValueStack<ColorMask>& colorMask()
    { return colorMask_; }
    /**
     * @return specify clear values for the color buffers.
     */
    inline ValueStack<ClearColor>& clearColor()
    { return clearColor_; }

    /**
     * Specify the width of rasterized lines.
     * The initial value is 1.
     */
    inline ValueStackAtomic<GLfloat>& lineWidth()
    { return lineWidth_; }

    /**
     * Specifies minimum rate at which sample shaing takes place.
     */
    inline ValueStackAtomic<GLfloat>& sampleShading()
    { return minSampleShading_; }

    /**
     * Specify a logical pixel operation for rendering.
     * 'opcode' must be one of GL_CLEAR,GL_SET,GL_COPY,GL_COPY_INVERTED,
     * GL_NOOP,GL_INVERT,GL_AND,GL_NAND,GL_OR,GL_NOR,GL_XOR,GL_EQUIV,GL_AND_REVERSE,
     * GL_AND_INVERTED,GL_OR_REVERSE,GL_OR_INVERTED. The initial value is GL_COPY.
     */
    inline ValueStackAtomic<GLenum>& logicOp()
    { return logicOp_; }

  protected:
    GLint maxDrawBuffers_;
    GLint maxTextureUnits_;
    GLint maxViewports_;
    GLint maxAttributes_;
    GLint maxFeedbackBuffers_;
    GLint maxUniformBuffers_;
    GLint maxAtomicCounterBuffers_;
    GLint maxShaderStorageBuffers_;
    GLint feedbackCount_;

    IndexedValueStack<GLboolean> toggles_;

    ParameterStackAtomic<GLuint> arrayBuffer_;
    ParameterStackAtomic<GLuint> elementArrayBuffer_;
    ParameterStackAtomic<GLuint> pixelPackBuffer_;
    ParameterStackAtomic<GLuint> pixelUnpackBuffer_;
    ParameterStackAtomic<GLuint> dispatchIndirectBuffer_;
    ParameterStackAtomic<GLuint> drawIndirectBuffer_;
    ParameterStackAtomic<GLuint> textureBuffer_;
    ParameterStackAtomic<GLuint> copyReadBuffer_;
    ParameterStackAtomic<GLuint> copyWriteBuffer_;
    ValueStackAtomic<GLuint> vao_;

    IndexedValueStack<BufferRange> uniformBufferRange_;
    IndexedValueStack<BufferRange> feedbackBufferRange_;
    IndexedValueStack<BufferRange> atomicCounterBufferRange_;
    IndexedValueStack<BufferRange> shaderStorageBufferRange_;

    ParameterStackAtomic<GLuint> readFrameBuffer_;
    ParameterStackAtomic<GLuint> drawFrameBuffer_;
    ValueStack<Viewport> viewport_;

    ValueStackAtomic<GLuint> shader_;

    ValueStackAtomic<GLenum> activeTexture_;
    IndexedValueStack<TextureBind> textures_;
    GLint textureCounter_;

    IndexedValueStack<Scissor> scissor_;
    ValueStackAtomic<GLenum> cullFace_;

    ValueStackAtomic<GLboolean> depthMask_;
    ValueStackAtomic<GLenum> depthFunc_;
    ValueStackAtomic<GLclampd> depthClear_;
    IndexedValueStack<DepthRange> depthRange_;

    ValueStack<Vec4f> blendColor_;
    IndexedValueStack<BlendEquation> blendEquation_;
    IndexedValueStack<BlendFunction> blendFunc_;

    ValueStackAtomic<GLuint> stencilMask_;
    ValueStack<StencilFunc> stencilFunc_;
    ValueStack<StencilOp> stencilOp_;

    ParameterStackAtomic<GLenum> polygonMode_;
    ValueStack<Vec2f> polygonOffset_;

    ValueStackAtomic<GLfloat> pointSize_;
    ParameterStackAtomic<GLfloat> pointFadeThreshold_;
    ParameterStackAtomic<GLint> pointSpriteOrigin_;

    ParameterStackAtomic<GLint> patchVertices_;
    ValueStack<PatchLevels> patchLevel_;

    IndexedValueStack<ColorMask> colorMask_;
    ValueStack<ClearColor> clearColor_;

    ValueStackAtomic<GLfloat> lineWidth_;
    ValueStackAtomic<GLfloat> minSampleShading_;
    ValueStackAtomic<GLenum> logicOp_;
    ValueStackAtomic<GLenum> frontFace_;

    RenderState();
  };
} // namespace

#endif /* RENDER_STATE_H_ */
