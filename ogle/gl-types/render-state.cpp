/*
 * render-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "render-state.h"

#include <ogle/utility/gl-util.h>
#include <ogle/states/state.h>
#include <ogle/meshes/mesh-state.h>
#include <ogle/states/texture-state.h>
#include <ogle/states/state-node.h>

typedef void (*ToggleFunc)(GLenum);

static inline void __BlendEquation(const BlendEquation &v)
{ glBlendEquationSeparate(v.x,v.y); }
static inline void __BlendEquationi(GLuint i, const BlendEquation &v)
{ glBlendEquationSeparatei(i,v.x,v.y); }
static inline void __BlendFunc(const BlendFunction &v)
{ glBlendFuncSeparate(v.x,v.y,v.z,v.w); }
static inline void __BlendFunci(GLuint i, const BlendFunction &v)
{ glBlendFuncSeparatei(i,v.x,v.y,v.z,v.w); }
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
static inline void __FBO(FrameBufferObject *v)
{ v->activate(); }
static inline void __Shader(Shader *v)
{ v->activate(); }
static inline void __Texture(GLuint channel, Texture* const &t)
{ t->activate(channel); }
inline void __Toggle(GLuint index, const GLboolean &v) {
  static const ToggleFunc toggleFuncs_[2] = {glDisable, glEnable};
  toggleFuncs_[v]( RenderState::toggleToID((RenderState::Toggle)index) );
}
static inline void __PatchLevel(const PatchLevels &l) {
  glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, &l.inner_.x);
  glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, &l.outer_.x);
}

RenderState::RenderState()
: maxDrawBuffers_( getGLInteger(GL_MAX_DRAW_BUFFERS) ),
  maxTextureUnits_( getGLInteger(GL_MAX_TEXTURE_IMAGE_UNITS) ),
  maxViewports_( getGLInteger(GL_MAX_VIEWPORTS) ),
  toggles_( TOGGLE_STATE_LAST, __lockedValue, __Toggle ),
  fbo_(__FBO),
  shader_(__Shader),
  texture_( maxTextureUnits_, __lockedValue, __Texture ),
  scissor_(maxViewports_, __Scissor, __Scissori),
  cullFace_(glCullFace),
  depthMask_(glDepthMask),
  depthFunc_(glDepthFunc),
  depthClear_(glClearDepth),
  depthRange_(maxViewports_, __DepthRange, __DepthRangei),
  blendColor_(__BlendColor),
  blendEquation_(maxDrawBuffers_, __BlendEquation, __BlendEquationi),
  blendFunc_(maxDrawBuffers_, __BlendFunc, __BlendFunci),
  stencilMask_(glStencilMask),
  stencilFunc_(__StencilFunc),
  stencilOp_(__StencilOp),
  polygonMode_(GL_FRONT_AND_BACK,glPolygonMode),
  polygonOffset_(__PolygonOffset),
  pointSize_(glPointSize),
  pointFadeThreshold_(GL_POINT_FADE_THRESHOLD_SIZE, glPointParameterf),
  pointSpriteOrigin_(GL_POINT_SPRITE_COORD_ORIGIN, glPointParameteri),
  patchVertices_(GL_PATCH_VERTICES, glPatchParameteri),
  patchLevel_(__PatchLevel),
  colorMask_(maxDrawBuffers_, __ColorMask, __ColorMaski),
  lineWidth_(glLineWidth),
  minSampleShading_(glMinSampleShading),
  logicOp_(glLogicOp),
  frontFace_(glFrontFace)
{
  textureCounter_ = 0;
  // init toggle states
  GLenum enabledToggles[] = {
      GL_CULL_FACE, GL_DEPTH_TEST,
      GL_TEXTURE_CUBE_MAP_SEAMLESS
  };
  for(GLint i=0; i<TOGGLE_STATE_LAST; ++i) {
    GLenum e = toggleToID((Toggle)i);
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
  colorMask_.push(ColorMask(GL_TRUE));
  logicOp_.push(GL_COPY);
  frontFace_.push(GL_CCW);
  pointFadeThreshold_.push(1.0);
  pointSpriteOrigin_.push(GL_UPPER_LEFT);
  lineWidth_.push(1.0);
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
