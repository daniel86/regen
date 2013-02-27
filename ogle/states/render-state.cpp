/*
 * render-state.cpp
 *
 *  Created on: 05.08.2012
 *      Author: daniel
 */

#include "render-state.h"

#include <ogle/utility/gl-error.h>
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

// TODO: -> utility
GLint getGLInteger(GLenum e) {
  GLint i=0;
  glGetIntegerv(e,&i);
  return i;
}

// TODO: deriving classes
RenderState::RenderState()
: maxDrawBuffers_( getGLInteger(GL_MAX_DRAW_BUFFERS) ),
  maxTextureUnits_( getGLInteger(GL_MAX_TEXTURE_IMAGE_UNITS_ARB) ),
  maxViewports_( getGLInteger(GL_MAX_VIEWPORTS) ),
  fbo_(ogleFBO),
  shader_(ogleShader),
  scissor_(maxViewports_, ogleScissor, ogleScissori),
  cullFace_(glCullFace),
  depthMask_(glDepthMask),
  depthFunc_(glDepthFunc),
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
  textureArray_ = new Stack< TextureState* >[maxTextureUnits_];
  textureCounter_ = 0;
  // init toggle states
  for(GLint i=0; i<TOGGLE_STATE_LAST; ++i) {
    pushToggle((Toggle)i,toggleToDefault_[i]);
  }
  // init value states
  pushCullFace(GL_BACK);
  pushDepthMask(GL_TRUE);
  pushDepthFunc(GL_LEQUAL);
  pushDepthRange(DepthRange(0.0,1.0));
  pushBlendEquation(BlendEquation(GL_FUNC_ADD));
  pushBlendFunction(BlendFunction(GL_ONE,GL_ONE,GL_ZERO,GL_ZERO));
  pushPolygonMode(GL_FILL);
  pushPolygonOffset(Vec2f(0.0f));
  pushPointSize(1.0);
  pushColorMask(ColorMask(GL_TRUE));
  pushLogicOp(GL_COPY);
  pushFrontFace(GL_CCW);
  pushPointFadeThreshold(1.0);
  pushPointSpriteOrigin(GL_UPPER_LEFT);
  pushLineWidth(1.0);
  // TODO: stack state ?
  glClearDepth(1.0f);
}
RenderState::~RenderState()
{
  delete []toggleStacks_;
  delete []textureArray_;
}

const GLenum RenderState::toggleToID_[TOGGLE_STATE_LAST] = {
    GL_BLEND,
    GL_COLOR_LOGIC_OP,
    GL_CULL_FACE,
    GL_DEBUG_OUTPUT,
    GL_DEBUG_OUTPUT_SYNCHRONOUS,
    GL_DEPTH_CLAMP,
    GL_DEPTH_TEST,
    GL_DITHER,
    GL_FRAMEBUFFER_SRGB,
    GL_LINE_SMOOTH,
    GL_MULTISAMPLE,
    GL_POLYGON_OFFSET_FILL,
    GL_POLYGON_OFFSET_LINE,
    GL_POLYGON_OFFSET_POINT,
    GL_POLYGON_SMOOTH,
    GL_PRIMITIVE_RESTART,
    GL_PRIMITIVE_RESTART_FIXED_INDEX,
    GL_SAMPLE_ALPHA_TO_COVERAGE,
    GL_SAMPLE_ALPHA_TO_ONE,
    GL_SAMPLE_COVERAGE,
    GL_SAMPLE_SHADING,
    GL_SAMPLE_MASK,
    GL_SCISSOR_TEST,
    GL_STENCIL_TEST,
    GL_TEXTURE_CUBE_MAP_SEAMLESS,
    GL_PROGRAM_POINT_SIZE,
    GL_CLIP_DISTANCE0,
    GL_CLIP_DISTANCE1,
    GL_CLIP_DISTANCE2,
    GL_CLIP_DISTANCE3
};

const GLenum RenderState::toggleToDefault_[TOGGLE_STATE_LAST] = {
    GL_FALSE, //GL_BLEND,
    GL_FALSE, //GL_COLOR_LOGIC_OP,
    GL_TRUE, //GL_CULL_FACE,
    GL_FALSE, //GL_DEBUG_OUTPUT,
    GL_FALSE, //GL_DEBUG_OUTPUT_SYNCHRONOUS,
    GL_FALSE, //GL_DEPTH_CLAMP,
    GL_TRUE, //GL_DEPTH_TEST,
    GL_FALSE, //GL_DITHER,
    GL_FALSE, //GL_FRAMEBUFFER_SRGB,
    GL_FALSE, //GL_LINE_SMOOTH,
    GL_FALSE, //GL_MULTISAMPLE,
    GL_FALSE, //GL_POLYGON_OFFSET_FILL,
    GL_FALSE, //GL_POLYGON_OFFSET_LINE,
    GL_FALSE, //GL_POLYGON_OFFSET_POINT,
    GL_FALSE, //GL_POLYGON_SMOOTH,
    GL_FALSE, //GL_PRIMITIVE_RESTART,
    GL_FALSE, //GL_PRIMITIVE_RESTART_FIXED_INDEX,
    GL_FALSE, //GL_SAMPLE_ALPHA_TO_COVERAGE,
    GL_FALSE, //GL_SAMPLE_ALPHA_TO_ONE,
    GL_FALSE, //GL_SAMPLE_COVERAGE,
    GL_FALSE, //GL_SAMPLE_SHADING,
    GL_FALSE, //GL_SAMPLE_MASK,
    GL_FALSE, //GL_SCISSOR_TEST,
    GL_FALSE, //GL_STENCIL_TEST,
    GL_TRUE, //GL_TEXTURE_CUBE_MAP_SEAMLESS,
    GL_FALSE, //GL_PROGRAM_POINT_SIZE,
    GL_FALSE, //GL_CLIP_DISTANCE0,
    GL_FALSE, //GL_CLIP_DISTANCE1,
    GL_FALSE, //GL_CLIP_DISTANCE2,
    GL_FALSE //GL_CLIP_DISTANCE3
};

// TODO: think about how hiding is handled
GLboolean RenderState::isNodeHidden(StateNode *node)
{
  return node->isHidden();
}
GLboolean RenderState::isStateHidden(State *state)
{
  return state->isHidden();
}

void RenderState::pushTexture(TextureState *tex)
{
  GLuint channel = tex->channel();
  textureArray_[channel].push(tex);

  // TODO: avoid TextureState arg
  // TODO: avoid uploadTexture call

  glActiveTexture(GL_TEXTURE0 + channel);
  tex->texture()->bind();

  Stack<Shader*> &shaderStack = shader_.stack_;
  if(!shaderStack.isEmpty()) {
    shaderStack.top()->uploadTexture(channel, tex->name());
  }
}
void RenderState::popTexture(GLuint channel)
{
  Stack<TextureState*> &queue = textureArray_[channel];
  queue.pop();
  if(!queue.isEmpty()) {
    glActiveTexture(GL_TEXTURE0 + channel);
    queue.top()->texture()->bind();

    Stack<Shader*> &shaderStack = shader_.stack_;
    if(!shaderStack.isEmpty()) {
      shaderStack.top()->uploadTexture(channel, queue.top()->name());
    }
  }
}

void RenderState::pushShaderInput(ShaderInput *in)
{
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
}
void RenderState::popShaderInput(const string &name)
{
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
}

#if 0

GLint RenderState::maxTextureUnits_ = -1;

RenderState::RenderState()
: isDepthTestEnabled_(GL_TRUE),
  isDepthWriteEnabled_(GL_TRUE),
  textureCounter_(-1),
  useTransformFeedback_(GL_FALSE)
{
  glEnable(GL_DEPTH_TEST);
  glDepthMask(GL_TRUE);
  glDepthFunc(GL_LEQUAL);
  glClearDepth(1.0f);

  if(maxTextureUnits_==-1) {
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB, &maxTextureUnits_);
  }
  textureArray = new Stack< TextureState* >[maxTextureUnits_];
}

void RenderState::set_isDepthTestEnabled(GLboolean v)
{
  isDepthTestEnabled_ = v;
  if(v) {
    glEnable(GL_DEPTH_TEST);
  } else {
    glDisable(GL_DEPTH_TEST);
  }
}
GLboolean RenderState::isDepthTestEnabled()
{
  return isDepthTestEnabled_;
}

void RenderState::set_isDepthWriteEnabled(GLboolean v)
{
  isDepthWriteEnabled_ = v;
  glDepthMask(v);
}
GLboolean RenderState::isDepthWriteEnabled()
{
  return isDepthWriteEnabled_;
}


GLboolean RenderState::isNodeHidden(StateNode *node)
{
  return node->isHidden();
}
GLboolean RenderState::isStateHidden(State *state)
{
  return state->isHidden();
}

RenderState::~RenderState()
{
  delete[] textureArray;
}

GLboolean RenderState::useTransformFeedback() const
{
  return useTransformFeedback_;
}
void RenderState::set_useTransformFeedback(GLboolean toggle)
{
  useTransformFeedback_ = toggle;
}

void RenderState::pushMesh(MeshState *mesh)
{
  if(shaders.isEmpty()) {
    mesh->draw( 1 );
  } else {
    mesh->draw( shaders.top()->numInstances() );
  }
}
void RenderState::popMesh()
{
}

void RenderState::pushFBO(FrameBufferObject *fbo)
{
  fbos.push(fbo);
  fbo->bind();
  fbo->set_viewport();
}
void RenderState::popFBO()
{
  fbos.pop();
  if(!fbos.isEmpty()) {
    // re-enable FBO from parent node
    FrameBufferObject *parent = fbos.top();
    parent->bind();
    parent->set_viewport();
  }
}

void RenderState::pushShader(Shader *shader)
{
  shaders.push(shader);
  glUseProgram(shader->id());
  shader->uploadInputs();
}
void RenderState::popShader()
{
  shaders.pop();
  if(!shaders.isEmpty()) {
    // re-enable Shader from parent node
    Shader *parent = shaders.top();
    glUseProgram(parent->id());
    parent->uploadInputs();
  }
}

//////////////////////////

GLuint RenderState::nextTexChannel()
{
  if(textureCounter_ < maxTextureUnits_) {
    textureCounter_ += 1;
  }
  return textureCounter_;
}
void RenderState::releaseTexChannel()
{
  Stack< TextureState* > &queue = textureArray[textureCounter_];
  if(queue.isEmpty()) {
    textureCounter_ -= 1;
  }
}

void RenderState::pushTexture(TextureState *tex)
{
  GLuint channel = tex->channel();
  Stack< TextureState* > &queue = textureArray[channel];
  queue.push(tex);

  glActiveTexture(GL_TEXTURE0 + channel);
  tex->texture()->bind();
  if(!shaders.isEmpty()) {
    shaders.top()->uploadTexture(channel, tex->name());
  }
}
void RenderState::popTexture(GLuint channel)
{
  Stack< TextureState* > &queue = textureArray[channel];
  queue.pop();
  if(queue.isEmpty()) { return; }

  glActiveTexture(GL_TEXTURE0 + channel);
  queue.top()->texture()->bind();
  if(!shaders.isEmpty()) {
    TextureState *texState = queue.top();
    shaders.top()->uploadTexture(channel, texState->name());
  }
}

//////////////////////////

void RenderState::pushShaderInput(ShaderInput *in)
{
  if(shaders.isEmpty()) { return; }
  Shader *activeShader = shaders.top();

  inputs_[in->name()].push(in);

  if(in->isVertexAttribute()) {
    activeShader->uploadAttribute(in);
  } else if(!in->isConstant()) {
    activeShader->uploadUniform(in);
  }
}
void RenderState::popShaderInput(const string &name)
{
  if(shaders.isEmpty()) { return; }
  Shader *activeShader = shaders.top();

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
}

#endif
