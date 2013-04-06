/*
 * render-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include <regen/config.h>
#include <regen/utility/gl-util.h>

#include "render-state.h"
using namespace regen;

#ifndef GL_DEBUG_OUTPUT
#define GL_DEBUG_OUTPUT GL_NONE
#endif
#ifndef GL_DEBUG_OUTPUT_SYNCHRONOUS
#define GL_DEBUG_OUTPUT_SYNCHRONOUS GL_NONE
#endif
#ifndef GL_PRIMITIVE_RESTART_FIXED_INDEX
#define GL_PRIMITIVE_RESTART_FIXED_INDEX GL_NONE
#endif
#ifndef GL_TEXTURE_CUBE_MAP_SEAMLESS
#define GL_TEXTURE_CUBE_MAP_SEAMLESS GL_NONE
#endif
#ifndef GL_DISPATCH_INDIRECT_BUFFER
#define GL_DISPATCH_INDIRECT_BUFFER 0x90EE
#endif
#ifndef GL_DRAW_INDIRECT_BUFFER
#define GL_DRAW_INDIRECT_BUFFER 0x8F3F
#endif
#ifndef GL_ATOMIC_COUNTER_BUFFER
#define GL_ATOMIC_COUNTER_BUFFER 0x92C0
#endif
#ifndef GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS
#define GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS 0
#endif
#ifndef GL_SHADER_STORAGE_BUFFER
#define GL_SHADER_STORAGE_BUFFER 0x90D2
#endif
#ifndef GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS
#define GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS 0
#endif
#ifndef GL_UNIFORM_BUFFER
#define GL_UNIFORM_BUFFER 0x8A11
#endif
#ifndef GL_MAX_UNIFORM_BUFFER_BINDINGS
#define GL_MAX_UNIFORM_BUFFER_BINDINGS 0
#endif

static inline void __BlendEquation(const BlendEquation &v)
{ glBlendEquationSeparate(v.x,v.y); }
static inline void __BlendEquationi(GLuint i, const BlendEquation &v)
{ glBlendEquationSeparatei(i,v.x,v.y); }
static inline void __BlendFunc(const BlendFunction &v)
{ glBlendFuncSeparate(v.x,v.y,v.z,v.w); }
static inline void __BlendFunci(GLuint i, const BlendFunction &v)
{ glBlendFuncSeparatei(i,v.x,v.y,v.z,v.w); }
static inline void __ClearColor(const ClearColor &v)
{ glClearColor(v.x,v.y,v.z,v.w); }
static inline void __ColorMask(const ColorMask &v)
{ glColorMask(v.x,v.y,v.z,v.w); }
static inline void __ColorMaski(GLuint i, const ColorMask &v)
{ glColorMaski(i,v.x,v.y,v.z,v.w); }
static inline void __DepthRange(const DepthRange &v)
{ glDepthRange(v.x,v.y); }
static inline void __DepthRangei(GLuint i, const DepthRange &v)
{ glDepthRangeIndexed(i,v.x,v.y); }
static inline void __StencilOp(const StencilOp &v)
{ glStencilOp(v.x,v.y,v.z); }
static inline void __StencilFunc(const StencilFunc &v)
{ glStencilFunc(v.func_,v.ref_,v.mask_); }
static inline void __PolygonOffset(const Vec2f &v)
{ glPolygonOffset(v.x,v.y); }
static inline void __BlendColor(const Vec4f &v)
{ glBlendColor(v.x,v.y,v.z,v.w); }
static inline void __Scissor(const Scissor &v)
{ glScissor(v.x,v.y,v.z,v.w); }
static inline void __Scissori(GLuint i, const Scissor &v)
{ glScissorIndexed(i, v.x,v.y,v.z,v.w); }
static inline void __Viewport(const Viewport &v)
{ glViewport(v.x,v.y,v.z,v.w); }
static inline void __AttribDivisor(GLuint i, const GLuint &v)
{ glVertexAttribDivisor(i,v); }
static inline void __Texture(GLuint i, const TextureBind &v)
{ glBindTexture(v.target_, v.id_); }
static inline void __UniformBufferRange(GLuint i, const BufferRange &v)
{ glBindBufferRange(GL_UNIFORM_BUFFER, i, v.buffer_, v.offset_, v.size_); }
static inline void __FeedbackBufferRange(GLuint i, const BufferRange &v)
{ glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, i, v.buffer_, v.offset_, v.size_); }
static inline void __AtomicCounterBufferRange(GLuint i, const BufferRange &v)
{ glBindBufferRange(GL_ATOMIC_COUNTER_BUFFER, i, v.buffer_, v.offset_, v.size_); }
static inline void __ShaderStorageBufferRange(GLuint i, const BufferRange &v)
{ glBindBufferRange(GL_SHADER_STORAGE_BUFFER, i, v.buffer_, v.offset_, v.size_); }

static inline void __PatchLevel(const PatchLevels &l) {
  glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, &l.inner_.x);
  glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, &l.outer_.x);
}

#ifndef WIN32
typedef void (*ToggleFunc)(GLenum);
#endif
inline void __Toggle(GLuint index, const GLboolean &v) {
#ifdef WIN32
  static const void (__stdcall *)(GLenum) toggleFuncs_[2] = {glDisable, glEnable};
#else
  static const ToggleFunc toggleFuncs_[2] = {glDisable, glEnable};
#endif
  GLenum toggleID = RenderState::toggleToID((RenderState::Toggle)index);
  toggleFuncs_[v](toggleID);
}

RenderState* RenderState::get()
{
  static RenderState rs;
  return &rs;
}

#ifdef WIN32
// TODO: GL function pointer errors in visual studio.
//   wrapper functions as a quick fix....
template<typename T> void __BindBuffer(GLenum key,T v)
{ glBindBuffer(key,v); }
template<typename T> void __BindFramebuffer(GLenum key,T v)
{ glBindFramebuffer(key,v); }
template<typename T> void __UseProgram(T v)
{ glUseProgram(v); }
template<typename T> void __ActiveTexture(T v)
{ glActiveTexture(v); }
template<typename T> void __CullFace(T v)
{ glCullFace(v); }
template<typename T> void __DepthMask(T v)
{ glDepthMask(v); }
template<typename T> void __DepthFunc(T v)
{ glDepthFunc(v); }
template<typename T> void __ClearDepth(T v)
{ glClearDepth(v); }
template<typename T> void __StencilMask(T v)
{ glStencilMask(v); }
template<typename T> void __PolygonMode(GLenum key,T v)
{ glPolygonMode(key,v); }
template<typename T> void __PointSize(T v)
{ glPointSize(v); }
template<typename T> void __LineWidth(T v)
{ glLineWidth(v); }
template<typename T> void __LogicOp(T v)
{ glLogicOp(v); }
template<typename T> void __FrontFace(T v)
{ glFrontFace(v); }
template<typename T> void __MinSampleShading(T v)
{ glMinSampleShading(v); }
template<typename T> void __PointParameterf(GLenum key,T v)
{ glPointParameterf(key,v); }
template<typename T> void __PointParameteri(GLenum key,T v)
{ glPointParameteri(key,v); }
#else
#define __BindBuffer glBindBuffer
#define __BindFramebuffer glBindFramebuffer
#define __UseProgram glUseProgram
#define __ActiveTexture glActiveTexture
#define __CullFace glCullFace
#define __DepthMask glDepthMask
#define __DepthFunc glDepthFunc
#define __ClearDepth glClearDepth
#define __StencilMask glStencilMask
#define __PolygonMode glPolygonMode
#define __PointSize glPointSize
#define __LineWidth glLineWidth
#define __LogicOp glLogicOp
#define __FrontFace glFrontFace
#define __PointParameterf glPointParameterf
#define __PointParameteri glPointParameteri
#define __MinSampleShading glMinSampleShading
#endif

RenderState::RenderState()
: maxDrawBuffers_(getGLInteger(GL_MAX_DRAW_BUFFERS)),
  maxTextureUnits_(getGLInteger(GL_MAX_TEXTURE_IMAGE_UNITS)),
  maxViewports_(getGLInteger(GL_MAX_VIEWPORTS)),
  maxAttributes_(getGLInteger(GL_MAX_VERTEX_ATTRIBS)),
  maxFeedbackBuffers_(getGLInteger(GL_MAX_TRANSFORM_FEEDBACK_BUFFERS)),
  maxUniformBuffers_(getGLInteger(GL_MAX_UNIFORM_BUFFER_BINDINGS)),
  maxAtomicCounterBuffers_(getGLInteger(GL_MAX_ATOMIC_COUNTER_BUFFER_BINDINGS)),
  maxShaderStorageBuffers_(getGLInteger(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS)),
  feedbackCount_(0),
  toggles_(TOGGLE_STATE_LAST, __lockedValue, __Toggle ),
  arrayBuffer_(GL_ARRAY_BUFFER,__BindBuffer),
  elementArrayBuffer_(GL_ELEMENT_ARRAY_BUFFER,__BindBuffer),
  pixelPackBuffer_(GL_PIXEL_PACK_BUFFER,__BindBuffer),
  pixelUnpackBuffer_(GL_PIXEL_UNPACK_BUFFER,__BindBuffer),
  dispatchIndirectBuffer_(GL_DISPATCH_INDIRECT_BUFFER,__BindBuffer),
  drawIndirectBuffer_(GL_DRAW_INDIRECT_BUFFER,__BindBuffer),
  textureBuffer_(GL_TEXTURE_BUFFER,__BindBuffer),
  copyReadBuffer_(GL_COPY_READ_BUFFER,__BindBuffer),
  copyWriteBuffer_(GL_COPY_WRITE_BUFFER,__BindBuffer),
  uniformBufferRange_(maxUniformBuffers_,__lockedValue,__UniformBufferRange),
  feedbackBufferRange_(maxFeedbackBuffers_,__lockedValue,__FeedbackBufferRange),
  atomicCounterBufferRange_(maxAtomicCounterBuffers_,__lockedValue,__AtomicCounterBufferRange),
  shaderStorageBufferRange_(maxShaderStorageBuffers_,__lockedValue,__ShaderStorageBufferRange),
  readFrameBuffer_(GL_READ_FRAMEBUFFER, __BindFramebuffer),
  drawFrameBuffer_(GL_DRAW_FRAMEBUFFER, __BindFramebuffer),
  viewport_(__Viewport),
  shader_(__UseProgram),
  activeTexture_(__ActiveTexture),
  textures_(maxTextureUnits_, __lockedValue, __Texture),
  attributeDivisor_(maxAttributes_, __lockedValue, __AttribDivisor),
  scissor_(maxViewports_, __Scissor, __Scissori),
  cullFace_(__CullFace),
  depthMask_(__DepthMask),
  depthFunc_(__DepthFunc),
  depthClear_(__ClearDepth),
  depthRange_(maxViewports_, __DepthRange, __DepthRangei),
  blendColor_(__BlendColor),
  blendEquation_(maxDrawBuffers_, __BlendEquation, __BlendEquationi),
  blendFunc_(maxDrawBuffers_, __BlendFunc, __BlendFunci),
  stencilMask_(__StencilMask),
  stencilFunc_(__StencilFunc),
  stencilOp_(__StencilOp),
  polygonMode_(GL_FRONT_AND_BACK,__PolygonMode),
  polygonOffset_(__PolygonOffset),
  pointSize_(__PointSize),
  pointFadeThreshold_(GL_POINT_FADE_THRESHOLD_SIZE, __PointParameterf),
  pointSpriteOrigin_(GL_POINT_SPRITE_COORD_ORIGIN, __PointParameteri),
  patchVertices_(GL_PATCH_VERTICES, __PointParameteri),
  patchLevel_(__PatchLevel),
  colorMask_(maxDrawBuffers_, __ColorMask, __ColorMaski),
  clearColor_(__ClearColor),
  lineWidth_(__LineWidth),
  minSampleShading_(__MinSampleShading),
  logicOp_(__LogicOp),
  frontFace_(__FrontFace)
{
  textureCounter_ = 0;
  // init toggle states
  GLenum enabledToggles[] = {
      GL_CULL_FACE, GL_DEPTH_TEST,
      GL_TEXTURE_CUBE_MAP_SEAMLESS
  };
  for(GLint i=0; i<TOGGLE_STATE_LAST; ++i) {
    GLenum e = toggleToID((Toggle)i);
    // avoid initial state set for unsupported states
    if(e==GL_NONE) continue;

    GLboolean enabled = GL_FALSE;
    for(GLuint j=0; j<sizeof(enabledToggles)/sizeof(GLenum); ++j) {
      if(enabledToggles[j]==e) {
        enabled = GL_TRUE;
        break;
      }
    }
    toggles_.push(i,enabled);
  }
  // init value states
  cullFace_.push(GL_BACK);
  depthMask_.push(GL_TRUE);
  depthFunc_.push(GL_LEQUAL);
  depthClear_.push(1.0);
  depthRange_.push(DepthRange(0.0,1.0));
  blendEquation_.push(BlendEquation(GL_FUNC_ADD));
  blendFunc_.push(BlendFunction(GL_ONE,GL_ONE,GL_ZERO,GL_ZERO));
  polygonMode_.push(GL_FILL);
  polygonOffset_.push(Vec2f(0.0f));
  pointSize_.push(1.0);
  lineWidth_.push(1.0);
  colorMask_.push(ColorMask(GL_TRUE));
  clearColor_.push(ClearColor(0.0f));
  logicOp_.push(GL_COPY);
  frontFace_.push(GL_CCW);
  pointFadeThreshold_.push(1.0);
  pointSpriteOrigin_.push(GL_UPPER_LEFT);
  activeTexture_.push(GL_TEXTURE0);
}

GLenum RenderState::toggleToID(Toggle t)
{
  switch(t) {
  case BLEND:
    return GL_BLEND;
  case COLOR_LOGIC_OP:
    return GL_COLOR_LOGIC_OP;
  case CULL_FACE:
    return GL_CULL_FACE;
  case DEBUG_OUTPUT:
    return GL_DEBUG_OUTPUT;
  case DEBUG_OUTPUT_SYNCHRONOUS:
    return GL_DEBUG_OUTPUT_SYNCHRONOUS;
  case DEPTH_CLAMP:
    return GL_DEPTH_CLAMP;
  case DEPTH_TEST:
    return GL_DEPTH_TEST;
  case DITHER:
    return GL_DITHER;
  case FRAMEBUFFER_SRGB:
    return GL_FRAMEBUFFER_SRGB;
  case LINE_SMOOTH:
    return GL_LINE_SMOOTH;
  case MULTISAMPLE:
    return GL_MULTISAMPLE;
  case POLYGON_OFFSET_FILL:
    return GL_POLYGON_OFFSET_FILL;
  case POLYGON_OFFSET_LINE:
    return GL_POLYGON_OFFSET_LINE;
  case POLYGON_OFFSET_POINT:
    return GL_POLYGON_OFFSET_POINT;
  case POLYGON_SMOOTH:
    return GL_POLYGON_SMOOTH;
  case PRIMITIVE_RESTART:
    return GL_PRIMITIVE_RESTART;
  case PRIMITIVE_RESTART_FIXED_INDEX:
    return GL_PRIMITIVE_RESTART_FIXED_INDEX;
  case RASTARIZER_DISCARD:
    return GL_RASTERIZER_DISCARD;
  case SAMPLE_ALPHA_TO_COVERAGE:
    return GL_SAMPLE_ALPHA_TO_COVERAGE;
  case SAMPLE_ALPHA_TO_ONE:
    return GL_SAMPLE_ALPHA_TO_ONE;
  case SAMPLE_COVERAGE:
    return GL_SAMPLE_COVERAGE;
  case SAMPLE_SHADING:
    return GL_SAMPLE_SHADING;
  case SAMPLE_MASK:
    return GL_SAMPLE_MASK;
  case SCISSOR_TEST:
    return GL_SCISSOR_TEST;
  case STENCIL_TEST:
    return GL_STENCIL_TEST;
  case TEXTURE_CUBE_MAP_SEAMLESS:
    return GL_TEXTURE_CUBE_MAP_SEAMLESS;
  case PROGRAM_POINT_SIZE:
    return GL_PROGRAM_POINT_SIZE;
  case CLIP_DISTANCE0:
    return GL_CLIP_DISTANCE0;
  case CLIP_DISTANCE1:
    return GL_CLIP_DISTANCE1;
  case CLIP_DISTANCE2:
    return GL_CLIP_DISTANCE2;
  case CLIP_DISTANCE3:
    return GL_CLIP_DISTANCE3;
  case TOGGLE_STATE_LAST:
    return GL_NONE;
  }
  return GL_NONE;
};
