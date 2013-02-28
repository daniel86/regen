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

// #define USE_VALUE_COMPARISON

static inline void ogleBlendEquation(const BlendEquation &v)
{ glBlendEquationSeparate(v.x,v.y); }
static inline void ogleBlendEquationi(GLuint i, const BlendEquation &v)
{ glBlendEquationSeparatei(i,v.x,v.y); }
static inline void ogleBlendFunc(const BlendFunction &v)
{ glBlendFuncSeparate(v.x,v.y,v.z,v.w); }
static inline void ogleBlendFunci(GLuint i, const BlendFunction &v)
{ glBlendFuncSeparatei(i,v.x,v.y,v.z,v.w); }
static inline void ogleColorMask(const ColorMask &v)
{ glColorMask(v.x,v.y,v.z,v.w); }
static inline void ogleColorMaski(GLuint i, const ColorMask &v)
{ glColorMaski(i,v.x,v.y,v.z,v.w); }
static inline void ogleDepthRange(const DepthRange &v)
{ glDepthRange(v.x,v.y); }
static inline void ogleDepthRangei(GLuint i, const DepthRange &v)
{ glDepthRangeIndexed(i,v.x,v.y); }
static inline void ogleStencilOp(const StencilOp &v)
{ glStencilOp(v.x,v.y,v.z); }
static inline void ogleStencilFunc(const StencilFunc &v)
{ glStencilFunc(v.func_,v.ref_,v.mask_); }
static inline void oglePolygonOffset(const Vec2f &v)
{ glPolygonOffset(v.x,v.y); }
static inline void ogleBlendColor(const Vec4f &v)
{ glBlendColor(v.x,v.y,v.z,v.w); }
static inline void ogleScissor(const Scissor &v)
{ glScissor(v.x,v.y,v.z,v.w); }
static inline void ogleScissori(GLuint i, const Scissor &v)
{ glScissorIndexed(i, v.x,v.y,v.z,v.w); }
static inline void oglePatchLevel(const PatchLevels &l) {
  glPatchParameterfv(GL_PATCH_DEFAULT_OUTER_LEVEL, &l.inner_.x);
  glPatchParameterfv(GL_PATCH_DEFAULT_INNER_LEVEL, &l.outer_.x);
}
static inline void ogleShader(Shader *v) {
  glUseProgram(v->id());
  v->uploadInputs();
}
static inline void ogleFBO(FrameBufferObject *v) {
  v->bind();
  v->set_viewport();
}
static inline void ogleTexture(GLuint channel, Texture* const &t) {
  t->activateBind(channel);
}

RenderState::RenderState()
: maxDrawBuffers_( getGLInteger(GL_MAX_DRAW_BUFFERS) ),
  maxTextureUnits_( getGLInteger(GL_MAX_TEXTURE_IMAGE_UNITS_ARB) ),
  maxViewports_( getGLInteger(GL_MAX_VIEWPORTS) ),
  fbo_(ogleFBO),
  shader_(ogleShader),
  texture_( maxTextureUnits_, __lockedValue, ogleTexture ),
  scissor_(maxViewports_, ogleScissor, ogleScissori),
  cullFace_(glCullFace),
  depthMask_(glDepthMask),
  depthFunc_(glDepthFunc),
  depthClear_(glClearDepth),
  depthRange_(maxViewports_, ogleDepthRange, ogleDepthRangei),
  blendColor_(ogleBlendColor),
  blendEquation_(maxDrawBuffers_, ogleBlendEquation, ogleBlendEquationi),
  blendFunc_(maxDrawBuffers_, ogleBlendFunc, ogleBlendFunci),
  stencilMask_(glStencilMask),
  stencilFunc_(ogleStencilFunc),
  stencilOp_(ogleStencilOp),
  polygonMode_(GL_FRONT_AND_BACK,glPolygonMode),
  polygonOffset_(oglePolygonOffset),
  pointSize_(glPointSize),
  pointFadeThreshold_(GL_POINT_FADE_THRESHOLD_SIZE, glPointParameterf),
  pointSpriteOrigin_(GL_POINT_SPRITE_COORD_ORIGIN, glPointParameteri),
  patchVertices_(GL_PATCH_VERTICES, glPatchParameteri),
  patchLevel_(oglePatchLevel),
  colorMask_(maxDrawBuffers_, ogleColorMask, ogleColorMaski),
  lineWidth_(glLineWidth),
  minSampleShading_(glMinSampleShading),
  logicOp_(glLogicOp),
  frontFace_(glFrontFace)
{
  toggleStacks_ = new Stack<GLboolean>[TOGGLE_STATE_LAST];
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
    pushToggle((Toggle)i,enabled);
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
RenderState::~RenderState()
{
  delete []toggleStacks_;
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

void RenderState::pushShaderInput(ShaderInput *in)
{
  /*
  Stack<Shader*> &stack = shader_.stack_;
  // shader inputs should be pushed before shader
  if(stack.isEmpty()) { return; }

  // TODO: avoid uploadUniform / uploadAttribute call
  // TODO: avoid inputs_ map

  inputs_[in->name()].push(in);

  Shader *activeShader = stack.top();
  if(in->isVertexAttribute()) {
    activeShader->uploadAttribute(in);
  } else if(!in->isConstant()) {
    activeShader->uploadUniform(in);
  }
  */
}
void RenderState::popShaderInput(const string &name)
{
  /*
  Stack<Shader*> &stack = shader_.stack_;
  // shader inputs should be pushed before shader
  if(stack.isEmpty()) { return; }
  Shader *activeShader = stack.top();

  Stack<ShaderInput*> &inputStack = inputs_[name];
  if(inputStack.isEmpty()) { return; }
  inputStack.pop();

  // reactivate new top stack member
  if(!inputStack.isEmpty()) {
    ShaderInput *reactivated = inputStack.topPtr();
    // re-apply input
    if(reactivated->isVertexAttribute()) {
      activeShader->uploadAttribute(reactivated);
    } else if(!reactivated->isConstant()) {
      activeShader->uploadUniform(reactivated);
    }
  }
  */
}
