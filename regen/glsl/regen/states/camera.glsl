
-- defines
#ifndef RENDER_LAYER
#define RENDER_LAYER 1
#endif
#endif
#ifndef RENDER_TARGET
#define RENDER_TARGET 2D
#endif
#if RENDER_LAYER == 1
#ifndef HAS_TESSELATION
#define VS_CAMERA_TRANSFORM
#endif
#if SHADER_STAGE == tes
#define TES_CAMERA_TRANSFORM
#endif
#define in_layer 0
#endif // RENDER_LAYER == 1
// Macros for Layered Camera access
#if RENDER_TARGET == CUBE
#define __VIEW__(layer)          in_viewMatrix[layer]
#define __VIEW_INV_(layer)       in_inverseViewMatrix[layer]
#define __VIEW_PROJ__(layer)     in_viewProjectionMatrix[layer]
#define __VIEW_PROJ_INV__(layer) in_inverseViewProjectionMatrix[layer]
#define __CAM_DIR__(layer)       in_cameraDirection[layer]
#elif RENDER_TARGET == 2D_ARRAY
#define __PROJ__(layer)          in_projectionMatrix[layer]
#define __PROJ_INV__(layer)      in_inverseProjectionMatrix[layer]
#define __VIEW_PROJ__(layer)     in_viewProjectionMatrix[layer]
#define __VIEW_PROJ_INV__(layer) in_inverseViewProjectionMatrix[layer]
#define __CAM_NEAR__(layer)      in_near[layer]
#define __CAM_FAR__(layer)       in_far[layer]
#endif // RENDER_TARGET == 2D_ARRAY
#ifndef __VIEW__(layer)
#define __VIEW__(layer)          in_viewMatrix
#endif
#ifndef __VIEW_INV__(layer)
#define __VIEW_INV__(layer)      in_inverseViewMatrix
#endif
#ifndef __PROJ__(layer)
#define __PROJ__(layer)          in_projectionMatrix
#endif
#ifndef __PROJ_INV__(layer)
#define __PROJ_INV__(layer)      in_inverseProjectionMatrix
#endif
#ifndef __VIEW_PROJ__(layer)
#define __VIEW_PROJ__(layer)     in_viewProjectionMatrix
#endif
#ifndef __VIEW_PROJ_INV__(layer)
#define __VIEW_PROJ_INV__(layer) in_inverseViewProjectionMatrix
#endif
#ifndef __CAM_DIR__(layer)
#define __CAM_DIR__(layer)       in_cameraDirection
#endif
#ifndef __CAM_POS__(layer)
#define __CAM_POS__(layer)       in_cameraPosition
#endif
#ifndef __CAM_NEAR__(layer)
#define __CAM_NEAR__(layer)      in_near
#endif
#ifndef __CAM_FAR__(layer)
#define __CAM_FAR__(layer)       in_far
#endif

-- input
#ifndef __camera_input_INCLUDED
#define2 __camera_input_INCLUDED
#include regen.states.camera.defines

////////////////
//// Camera input start
////////////////

uniform float in_fov;
uniform float in_aspect;

#if RENDER_TARGET == CUBE
uniform vec3 in_cameraPosition;
uniform vec3 in_cameraDirection[6];

uniform float in_near;
uniform float in_far;

uniform mat4 in_viewMatrix[6];
uniform mat4 in_inverseViewMatrix[6];

uniform mat4 in_projectionMatrix;
uniform mat4 in_inverseProjectionMatrix;

uniform mat4 in_viewProjectionMatrix[6];
uniform mat4 in_inverseViewProjectionMatrix[6];

#elif RENDER_TARGET == 2D_ARRAY
uniform vec3 in_cameraPosition;
uniform vec3 in_cameraDirection;

uniform float in_near[${RENDER_LAYER}];
uniform float in_far[${RENDER_LAYER}];

uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewMatrix;

uniform mat4 in_projectionMatrix[${RENDER_LAYER}];
uniform mat4 in_inverseProjectionMatrix[${RENDER_LAYER}];

uniform mat4 in_viewProjectionMatrix[${RENDER_LAYER}];
uniform mat4 in_inverseViewProjectionMatrix[${RENDER_LAYER}];

#else // RENDER_TARGET == 2D
uniform vec3 in_cameraPosition;
uniform vec3 in_cameraDirection;

uniform float in_near;
uniform float in_far;

uniform mat4 in_viewMatrix;
uniform mat4 in_inverseViewMatrix;

uniform mat4 in_projectionMatrix;
uniform mat4 in_inverseProjectionMatrix;

uniform mat4 in_viewProjectionMatrix;
uniform mat4 in_inverseViewProjectionMatrix;
#endif

////////////////
//// Camera input stop
////////////////
#endif

-- transformWorldToEye
#ifndef __transformWorldToEye_INCLUDED
#define2 __transformWorldToEye_INCLUDED
#include regen.states.camera.input
vec4 transformWorldToEye(vec4 posWorld, mat4 view) {
#ifdef IGNORE_VIEW_ROTATION
    return vec4(view[3].xyz,0.0) + posWorld;
#elif IGNORE_VIEW_TRANSLATION
    return mat4(view[0], view[1], view[2], vec3(0.0), 1.0) * posWorld;
#else
    return view * posWorld;
#endif
}
vec4 transformWorldToEye(vec4 posWorld, int layer) {
  return transformWorldToEye(posWorld, __VIEW__(layer));
}
vec4 transformWorldToEye(vec3 posWorld, int layer) {
  return transformWorldToEye(vec4(posWorld,1.0),__VIEW__(layer));
}
#endif
-- transformWorldToScreen
#ifndef __transformEyeToScreen_INCLUDED
#define2 __transformEyeToScreen_INCLUDED
#include regen.states.camera.input
vec4 transformWorldToScreen(vec4 posWorld, int layer) {
  return __VIEW_PROJ__(layer) * posWorld;
}
vec4 transformWorldToScreen(vec3 posWorld, int layer) {
  return __VIEW_PROJ__(layer) * vec4(posWorld,1.0);
}
#endif
-- transformWorldToTexco
#ifndef __transformWorldToTexco_included__
#define __transformWorldToTexco_included__
#include regen.states.camera.input
vec3 transformWorldToTexco(vec4 posWorld, int layer)
{
  vec4 posScreen = __VIEW_PROJ__(layer)*posWorld;
  return (posScreen.xyz/posScreen.w + vec3(1.0))*0.5;
}
#endif

-- transformEyeToScreen
#ifndef __transformEyeToScreen_INCLUDED
#define2 __transformEyeToScreen_INCLUDED
#include regen.states.camera.input
vec4 transformEyeToScreen(vec4 posEye, mat4 proj) {
  return proj * posEye;
}
vec4 transformEyeToScreen(vec4 posEye, int layer) {
  return __PROJ__(layer) * posEye;
}
vec4 transformEyeToScreen(vec3 posEye, int layer) {
  return __PROJ__(layer) * vec4(posEye,1.0);
}
#endif
-- transformEyeToTexco
#ifndef __transformEyeToTexco_included__
#define __transformEyeToTexco_included__
#include regen.states.camera.input
vec3 transformEyeToTexco(vec4 posEye, int layer)
{
  vec4 posScreen = __PROJ__(layer)*posEye;
  return (posScreen.xyz/posScreen.w + vec3(1.0))*0.5;
}
#endif
-- transformEyeToWorld
#ifndef __transformEyeToWorld_INCLUDED
#define2 __transformEyeToWorld_INCLUDED
#include regen.states.camera.input
vec4 transformEyeToWorld(vec4 posEye, mat4 viewInv) {
  return viewInv * posEye;
}
vec4 transformEyeToWorld(vec4 posEye, int layer) {
  return transformEyeToWorld(posEye, __VIEW_INV__(layer));
}
vec4 transformEyeToWorld(vec3 posEye, int layer) {
  return transformEyeToWorld(vec4(posEye,1.0),__VIEW_INV__(layer));
}
#endif

-- transformScreenToTexco
#ifndef __transformScreenToTexco_included__
#define __transformScreenToTexco_included__
#include regen.states.camera.input
vec2 transformScreenToTexco(vec4 posScreen)
{
  return (posScreen.xy/posScreen.w + vec2(1.0))*0.5;
}
#endif

-- transformTexcoToWorld
#ifndef __transformTexcoToWorld_included__
#define __transformTexcoToWorld_included__
#include regen.states.camera.input
vec3 transformTexcoToWorld(vec2 texco, float depth, int layer) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = __VIEW_PROJ_INV__(layer)*vec4(pos0,1.0);
    return D.xyz/D.w;
}
#endif
-- transformTexcoToView
#ifndef __transformTexcoToView_included__
#define __transformTexcoToView_included__
#include regen.states.camera.input
vec3 transformTexcoToView(vec2 texco, float depth, int layer) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = __PROJ_INV__(layer)*vec4(pos0,1.0);
    return D.xyz/D.w;
}
#endif

-- transformParabolid
#ifndef __transformParabolid_INCLUDED
#define2 __transformParabolid_INCLUDED
vec4 transformParabolid(vec4 posScreen, int layer) {
  float l = length(posScreen.xyz);
  vec4 posParabolid;
  posParabolid.xyz = posScreen.xyz / L;
  posParabolid.xy /= (posParabolid.z+1.0);
  posParabolid.zw  = vec2((l - __CAM_NEAR__(layer))/(__CAM_FAR__(layer) - __CAM_NEAR__(layer)), 1.0);
  return posParabolid;
}
#endif

-- linearizeDepth
#ifndef __linearizeDepth_included__
#define __linearizeDepth_included__
float linearizeDepth(float expDepth, float n, float f)
{
    return (2.0*n)/(f+n - expDepth*(f-n));
}
#endif

-- exponentialDepth
#ifndef __exponentialDepth_included__
#define __exponentialDepth_included__
float exponentialDepth(float linearDepth, float n, float f)
{
  return ((f+n)*linearDepth - (2.0*n)) / ((f-n)*linearDepth);
}
#endif

-- depthCorrection
#ifndef __depthCorrection_Include__
#define __depthCorrection_Include__
void depthCorrection(float depth, int layer)
{
  vec3 pe = in_posEye + depth*normalize(in_posEye);
  vec4 ps = __PROJ__(layer) * vec4(pe,1.0);
  gl_FragDepth = (ps.z/ps.w)*0.5 + 0.5;
}
#endif
