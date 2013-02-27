/*
 * render-state.h
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#ifndef RENDER_STATE_H_
#define RENDER_STATE_H_

#include <ogle/states/state-stacks.h>
#include <ogle/gl-types/fbo.h>
#include <ogle/gl-types/shader.h>
#include <ogle/gl-types/texture.h>

class State;
class StateNode;
class TextureState;

typedef Vec4ui Viewport;
typedef Vec4ui Scissor;
typedef Vec4i BlendFunction;
typedef Vec2i BlendEquation;
typedef Vec3i StencilOp;
typedef Vec2d DepthRange;
typedef Vec4b ColorMask;
struct StencilFunc {
  GLenum func_;
  GLint ref_;
  GLuint mask_;
};
struct PatchLevels {
  PatchLevels(const Vec4f &inner, const Vec4f &outer)
  : inner_(inner), outer_(outer) {}
  Vec4f inner_;
  Vec4f outer_;
};

class RenderState
{
public:
  /**
   * States that can be affected by glEnable/glDisable
   */
  enum Toggle {
    // If enabled, blend the computed fragment color values with the values in the color buffers.
    BLEND=0,
    // If enabled, apply the currently selected logical operation to the
    // computed fragment color and color buffer values.
    COLOR_LOGIC_OP,
    // If enabled,cull polygons based on their winding in window coordinates.
    CULL_FACE,
    // If enabled, debug messages are produced by a debug context. When disabled,
    // the debug message log is silenced. Note that in a non-debug context, very
    // few, if any messages might be produced, even when GL_DEBUG_OUTPUT is enabled.
    DEBUG_OUTPUT,
    // If enabled, debug messages are produced synchronously by a debug context. If disabled,
    // debug messages may be produced asynchronously. In particular, they may be delayed relative
    // to the execution of GL commands, and the debug callback function may be called from
    // a thread other than that in which the commands are executed.
    DEBUG_OUTPUT_SYNCHRONOUS,
    // If enabled, the -wc<=zc<=wc plane equation is ignored
    // by view volume clipping (effectively, there is no near or far plane clipping).
    DEPTH_CLAMP,
    // If enabled, do depth comparisons and update the depth buffer.
    // Note that even if the depth buffer exists and the depth mask is non-zero,
    // the depth buffer is not updated if the depth test is disabled.
    DEPTH_TEST,
    // If enabled, dither color components or indices before they are written to the color buffer.
    DITHER,
    // If enabled and the value of GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING for the
    // framebuffer attachment corresponding to the destination buffer is GL_SRGB,
    // the R, G, and B destination color values (after conversion from fixed-point to floating-point)
    // are considered to be encoded for the sRGB color space and hence are linearized prior to
    // their use in blending.
    FRAMEBUFFER_SRGB,
    // If enabled, draw lines with correct filtering. Otherwise, draw aliased lines.
    LINE_SMOOTH,
    // If enabled, use multiple fragment samples in computing the final color of a pixel.
    MULTISAMPLE,
    // If enabled, and if the polygon is rendered in
    // GL_FILL mode, an offset is added to depth values of a polygon's
    // fragments before the depth comparison is performed.
    POLYGON_OFFSET_FILL,
    // If enabled, and if the polygon is rendered in
    // GL_LINE mode, an offset is added to depth values of a polygon's
    // fragments before the depth comparison is performed.
    POLYGON_OFFSET_LINE,
    // If enabled, an offset is added to depth values of a polygon's fragments
    // before the depth comparison is performed, if the polygon is rendered in
    // GL_POINT mode.
    POLYGON_OFFSET_POINT,
    // If enabled, draw polygons with proper filtering.
    // Otherwise, draw aliased polygons. For correct antialiased polygons,
    // an alpha buffer is needed and the polygons must be sorted front to
    // back.
    POLYGON_SMOOTH,
    // Enables primitive restarting.  If enabled, any one of the draw commands
    // which transfers a set of generic attribute array elements to the GL will restart
    // the primitive when the index of the vertex is equal to the primitive restart index.
    PRIMITIVE_RESTART,
    // Enables primitive restarting with a fixed index. If enabled, any one of the
    // draw commands which transfers a set of generic attribute array elements to the GL will
    // restart the primitive when the index of the vertex is equal to the fixed primitive
    // index for the specified index type.
    PRIMITIVE_RESTART_FIXED_INDEX,
    // If enabled, compute a temporary coverage value where each bit is determined by the
    // alpha value at the corresponding sample location.  The temporary coverage
    // value is then ANDed with the fragment coverage value.
    SAMPLE_ALPHA_TO_COVERAGE,
    // If enabled, each sample alpha value is replaced by the maximum representable alpha value.
    SAMPLE_ALPHA_TO_ONE,
    // If enabled,
    // the fragment's coverage is ANDed with the temporary coverage value.  If
    // GL_SAMPLE_COVERAGE_INVERT is set to GL_TRUE, invert the coverage
    // value.
    SAMPLE_COVERAGE,
    // If enabled, the active fragment shader is run once for each covered sample, or at
    // fraction of this rate as determined by the current value of GL_MIN_SAMPLE_SHADING_VALUE.
    SAMPLE_SHADING,
    // If enabled, the sample coverage mask generated for a fragment during rasterization
    // will be ANDed with the value of GL_SAMPLE_MASK_VALUE before
    // shading occurs.
    SAMPLE_MASK,
    // If enabled, discard fragments that are outside the scissor rectangle.
    SCISSOR_TEST,
    // If enabled, do stencil testing and update the stencil buffer.
    STENCIL_TEST,
    // If enabled, cubemap textures are sampled such that when linearly sampling from the border
    // between two adjacent faces, texels from both faces are used to generate the final sample
    // value. When disabled, texels from only a single face are used to construct the final
    // sample value.
    TEXTURE_CUBE_MAP_SEAMLESS,
    // If enabled and a vertex or geometry shader is active,
    // then the derived point size is taken from the (potentially clipped) shader builtin
    // gl_PointSize and clamped to the implementation-dependent point size range.
    PROGRAM_POINT_SIZE,
    // If enabled, clip geometry against user-defined half space i.
    CLIP_DISTANCE0,
    CLIP_DISTANCE1,
    CLIP_DISTANCE2,
    CLIP_DISTANCE3,
    TOGGLE_STATE_LAST
  };
  static GLenum toggleToID(Toggle t);

  RenderState();
  virtual ~RenderState();

  virtual GLboolean isNodeHidden(StateNode *node);
  virtual GLboolean isStateHidden(State *state);

  inline GLuint nextTexChannel() {
    return min(textureCounter_++,maxTextureUnits_-1);
  }
  inline void releaseTexChannel() {
    if(textureArray_[min(textureCounter_,maxTextureUnits_-1)].isEmpty())
      textureCounter_ -= 1;
  }

  GLboolean isTransformFeedbackAcive() const {
    return isFeedbackAcive_;
  }
  // glBeginTransformFeedback()
  void beginTransformFeedback(GLenum v) {
    glBeginTransformFeedback(v);
    isFeedbackAcive_ = GL_TRUE;
  }
  // glEndTransformFeedback()
  void endTransformFeedback() {
    glEndTransformFeedback();
    isFeedbackAcive_ = GL_FALSE;
  }

  /**
   * Enable or disable server-side GL capabilities.
   */
  inline void pushToggle(Toggle toggle, GLboolean v) {
    toggleStacks_[toggle].push(v);
    if(v) glEnable( toggleToID(toggle) );
    else glDisable( toggleToID(toggle) );
  }
  inline void popToggle(Toggle toggle) {
    Stack<GLboolean> &stack = toggleStacks_[toggle];
    stack.pop();
    if(stack.top()) glEnable( toggleToID(toggle) );
    else           glDisable( toggleToID(toggle) );
  }

  /**
   * Bind a framebuffer to a framebuffer target
   * and set the viewport.
   */
  inline void pushFBO(FrameBufferObject *v)
  { fbo_.push(v); }
  inline void popFBO()
  { fbo_.pop(); }

  /**
   * Installs a program object as part of current rendering state
   * and specifies the values of uniform variables and attributes
   * for the program object.
   */
  inline void pushShader(Shader *v)
  { shader_.push(v); }
  inline void popShader()
  { shader_.pop(); }
  inline Shader* shader()
  { return shader_.stack_.isEmpty() ? NULL : shader_.stack_.top(); }

  void pushShaderInput(ShaderInput *in);
  void popShaderInput(const string &name);

  // glBindTexture
  void pushTexture(TextureState *tex);
  void popTexture(GLuint channel);

  /**
   * Define the scissor box.
   * 'left', 'bottom' specify the coordinate of the bottom left corner
   * of the scissor box, in pixels.
   * 'width', 'height' specifies the dimensions of the scissor box, in pixels.
   */
  inline void pushScissor(const Scissor &v)
  { scissor_.push(v); }
  inline void popScissor()
  { scissor_.pop(); }
  /**
   * Define the scissor box for a specific viewport.
   * 'index' specifies the index of the viewport whose scissor box to modify.
   * 'left', 'bottom' specify the coordinate of the bottom left corner
   * of the scissor box, in pixels.
   * 'width', 'height' specifies the dimensions of the scissor box, in pixels.
   */
  inline void pushScissor(GLint index, const Scissor &v)
  { scissor_.push(index,v); }
  inline void popScissor(GLint index)
  { scissor_.pop(index); }

  /**
   * Specifies whether front- or back-facing facets are candidates for culling.
   * Symbolic constants GL_FRONT,GL_BACK, GL_FRONT_AND_BACK are accepted.
   * The initial value is GL_BACK.
   */
  inline void pushCullFace(GLenum v)
  { cullFace_.push(v); }
  inline void popCullFace()
  { cullFace_.pop(); }
  /**
   * Specifies the orientation of front-facing polygons.
   * GL_CW and GL_CCW are accepted. The initial value is GL_CCW.
   */
  inline void pushFrontFace(GLenum v)
  { frontFace_.push(v); }
  inline void popFrontFace()
  { frontFace_.pop(); }

  /**
   * Specifies whether the depth buffer is enabled for writing.
   * If flag is GL_FALSE, depth buffer writing is disabled.
   * Otherwise, it is enabled. Initially, depth buffer writing is enabled.
   */
  inline void pushDepthMask(GLboolean v)
  { depthMask_.push(v); }
  inline void popDepthMask()
  { depthMask_.pop(); }
  /**
   * Specifies the depth comparison function. Symbolic constants
   * GL_NEVER,GL_LESS,GL_EQUAL,GL_LEQUAL,GL_GREATER,
   * GL_NOTEQUAL,GL_GEQUAL,GL_ALWAYS are accepted.
   * The initial value is GL_LESS.
   */
  inline void pushDepthFunc(GLenum v)
  { depthFunc_.push(v); }
  inline void popDepthFunc()
  { depthFunc_.pop(); }
  inline void pushDepthClear(GLclampd v)
  { depthClear_.push(v); }
  inline void popDepthClear()
  { depthClear_.pop(); }
  /**
   * Specify mapping of depth values from normalized device coordinates to
   * window coordinates for a specified viewport.
   * 'index' specifies the index of the viewport whose depth range to update.
   * 'nearVal' specifies the mapping of the near clipping plane to window coordinates.
   * The initial value is 0.
   * 'farVal' specifies the mapping of the far clipping plane to window coordinates.
   * The initial value is 1.
   */
  inline void pushDepthRange(const DepthRange &v)
  { depthRange_.push(v); }
  inline void popDepthRange()
  { depthRange_.pop(); }
  /**
   * Specify mapping of depth values from normalized device coordinates
   * to window coordinates.
   * 'nearVal' specifies the mapping of the near clipping plane to window coordinates.
   * The initial value is 0.
   * 'farVal' specifies the mapping of the far clipping plane to window coordinates.
   * The initial value is 1.
   */
  inline void pushDepthRange(GLuint index, const DepthRange &v)
  { depthRange_.push(index,v); }
  inline void popDepthRange(GLuint index)
  { depthRange_.pop(index); }

  /**
   * Set the blend color.
   * Initially the GL_BLEND_COLOR is set to (0,0,0,0).
   */
  inline void pushBlendColor(const Vec4f &v)
  { blendColor_.push(v); }
  inline void popBlendColor()
  { blendColor_.pop(); }
  /**
   * Specify the equation used for both the RGB blend equation and the
   * Alpha blend equation.
   * 'mode' specifies how source and destination colors are combined.
   * It must be GL_FUNC_ADD, GL_FUNC_SUBTRACT, GL_FUNC_REVERSE_SUBTRACT, GL_MIN, GL_MAX.
   */
  inline void pushBlendEquation(const BlendEquation &v)
  { blendEquation_.push(v); }
  inline void popBlendEquation()
  { blendEquation_.pop(); }
  /**
   * Specify the equation used for both the RGB blend equation and the
   * Alpha blend equation.
   * 'buf' specifies the index of the draw buffer for which to set the blend equation.
   * 'mode' specifies how source and destination colors are combined.
   * It must be GL_FUNC_ADD,GL_FUNC_SUBTRACT,GL_FUNC_REVERSE_SUBTRACT,GL_MIN,GL_MAX.
   * Initially, both the RGB blend equation and the alpha blend equation
   * are set to GL_FUNC_ADD.
   */
  inline void pushBlendEquation(GLuint index, const BlendEquation &v)
  { blendEquation_.push(index,v); }
  inline void popBlendEquation(GLuint index)
  { blendEquation_.pop(index); }
  /**
   * Specify pixel arithmetic.
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
  inline void pushBlendFunction(const BlendFunction &v)
  { blendFunc_.push(v); }
  inline void popBlendFunction()
  { blendFunc_.pop(); }
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
  inline void pushBlendFunction(GLuint index, const BlendFunction &v)
  { blendFunc_.push(index,v); }
  inline void popBlendFunction(GLuint index)
  { blendFunc_.pop(index); }

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
  inline void pushStencilFunc(const StencilFunc &v)
  { stencilFunc_.push(v); }
  inline void popStencilFunc()
  { stencilFunc_.pop(); }
  /**
   * Specifies a bit mask to enable and disable writing of individual bits
   * in the stencil planes. Initially, the mask is all 1's.
   */
  inline void pushStencilMask(GLuint v)
  { stencilMask_.push(v); }
  inline void popStencilMask()
  { stencilMask_.pop(); }
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
  inline void pushStencilOp(const StencilOp &v)
  { stencilOp_.push(v); }
  inline void popStencilOp()
  { stencilOp_.pop(); }

  /**
   * Specifies how polygons will be rasterized.
   * Accepted values are GL_POINT,GL_LINE,GL_FILL.
   * The initial value is GL_FILL for both front- and back-facing polygons.
   */
  inline void pushPolygonMode(GLenum v)
  { polygonMode_.push(v); }
  inline void popPolygonMode()
  { polygonMode_.pop(); }
  /**
   * Set the scale and units used to calculate depth values.
   * v.x-'factor' specifies a scale factor that is used to create a variable
   * depth offset for each polygon. The initial value is 0.
   * v.y-'units' is multiplied by an implementation-specific value to
   * create a constant depth offset. The initial value is 0.
   */
  inline void pushPolygonOffset(const Vec2f &v)
  { polygonOffset_.push(v); }
  inline void popPolygonOffset()
  { polygonOffset_.pop(); }

  /**
   * Specify the diameter of rasterized points.
   * The initial value is 1.
   */
  inline void pushPointSize(GLfloat v)
  { pointSize_.push(v); }
  inline void popPointSize()
  { pointSize_.pop(); }

  /**
   * Specifies the threshold value to which point sizes are clamped
   * if they exceed the specified value. The default value is 1.0.
   */
  inline void pushPointFadeThreshold(GLfloat v)
  { pointFadeThreshold_.push(v); }
  inline void popPointFadeThreshold()
  { pointFadeThreshold_.pop(); }
  /**
   * Specify the point sprite texture coordinate origin,
   * either GL_LOWER_LEFT or GL_UPPER_LEFT.
   * The default value is GL_UPPER_LEFT.
   */
  inline void pushPointSpriteOrigin(GLenum v)
  { pointSpriteOrigin_.push(v); }
  inline void popPointSpriteOrigin()
  { pointSpriteOrigin_.pop(); }

  /**
   * Specifies the number of vertices that
   * will be used to make up a single patch primitive.
   */
  inline void pushPatchVertices(GLint v)
  { patchVertices_.push(v); }
  inline void popPatchVertices()
  { patchVertices_.pop(); }
  /**
   * Specifies the default outer or inner tessellation levels
   * to be used when no tessellation control shader is present.
   */
  inline void pushPatchLevel(const Vec4f &inner, const Vec4f &outer)
  { patchLevel_.push(PatchLevels(inner,outer)); }
  inline void popPatchLevel()
  { patchLevel_.pop(); }

  /**
   * Enable and disable writing of frame buffer color components.
   */
  inline void pushColorMask(const ColorMask &v)
  { colorMask_.push(v); }
  inline void popColorMask()
  { colorMask_.pop(); }
  /**
   * Enable and disable writing of frame buffer color components.
   */
  inline void pushColorMask(GLuint index, const ColorMask &v)
  { colorMask_.push(index,v); }
  inline void popColorMask(GLuint index)
  { colorMask_.pop(index); }

  /**
   * Specify the width of rasterized lines.
   * The initial value is 1.
   */
  inline void pushLineWidth(GLfloat v)
  { lineWidth_.push(v); }
  inline void popLineWidth()
  { lineWidth_.pop(); }

  /**
   * Specifies minimum rate at which sample shaing takes place.
   */
  inline void pushMinSampleShading(GLfloat v)
  { minSampleShading_.push(v); }
  inline void popMinSampleShading()
  { minSampleShading_.pop(); }

  /**
   * Specify a logical pixel operation for rendering.
   * 'opcode' must be one of GL_CLEAR,GL_SET,GL_COPY,GL_COPY_INVERTED,
   * GL_NOOP,GL_INVERT,GL_AND,GL_NAND,GL_OR,GL_NOR,GL_XOR,GL_EQUIV,GL_AND_REVERSE,
   * GL_AND_INVERTED,GL_OR_REVERSE,GL_OR_INVERTED. The initial value is GL_COPY.
   */
  inline void pushLogicOp(GLenum opcode)
  { logicOp_.push(opcode); }
  inline void popLogicOp()
  { logicOp_.pop(); }

protected:
  GLint maxDrawBuffers_;
  GLint maxTextureUnits_;
  GLint maxViewports_;

  GLboolean isFeedbackAcive_;

  ValueStackAtomic<FrameBufferObject*> fbo_;

  ValueStackAtomic<Shader*> shader_;
  map< string, Stack<ShaderInput*> > inputs_;

  Stack<TextureState*> *textureArray_;
  GLint textureCounter_;

  Stack<GLboolean> *toggleStacks_;

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

  ValueStackAtomic<GLfloat> lineWidth_;
  ValueStackAtomic<GLfloat> minSampleShading_;
  ValueStackAtomic<GLenum> logicOp_;
  ValueStackAtomic<GLenum> frontFace_;
};

#endif /* RENDER_STATE_H_ */
