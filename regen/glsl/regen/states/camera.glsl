
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
#endif // RENDER_LAYER == 1

#if SHADER_STAGE == fs
#if RENDER_TARGET == CUBE
#define __VIEW__          in_viewMatrix[in_layer]
#define __VIEW_INV__      in_inverseViewMatrix[in_layer]
#define __VIEW_PROJ__     in_viewProjectionMatrix[in_layer]
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix[in_layer]
#define __CAM_DIR__       in_cameraDirection[in_layer]
#elif RENDER_TARGET == 2D_ARRAY
#define __PROJ__          in_projectionMatrix[in_layer]
#define __PROJ_INV__      in_inverseProjectionMatrix[in_layer]
#define __VIEW_PROJ__     in_viewProjectionMatrix[in_layer]
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix[in_layer]
#define __CAM_NEAR__      in_near[in_layer]
#define __CAM_FAR__       in_far[in_layer]
#endif // RENDER_TARGET == 2D_ARRAY
#endif // SHADER_STAGE == fs
#ifndef __VIEW__
#define __VIEW__          in_viewMatrix
#endif
#ifndef __VIEW_INV__
#define __VIEW_INV__      in_inverseViewMatrix
#endif
#ifndef __PROJ__
#define __PROJ__          in_projectionMatrix
#endif
#ifndef __PROJ_INV__
#define __PROJ_INV__      in_inverseProjectionMatrix
#endif
#ifndef __VIEW_PROJ__
#define __VIEW_PROJ__     in_viewProjectionMatrix
#endif
#ifndef __VIEW_PROJ_INV__
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix
#endif
#ifndef __CAM_DIR__
#define __CAM_DIR__       in_cameraDirection
#endif
#ifndef __CAM_POS__
#define __CAM_POS__       in_cameraPosition
#endif
#ifndef __CAM_NEAR__
#define __CAM_NEAR__      in_near
#endif
#ifndef __CAM_FAR__
#define __CAM_FAR__       in_far
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

-- transformParabolid
#ifndef __transformParabolid_INCLUDED
#define2 __transformParabolid_INCLUDED
vec4 transformParabolid(vec4 posScreen) {
  float l = length(posScreen.xyz);
  vec4 posParabolid;
  posParabolid.xy  = posScreen.xy / L;
  posParabolid.xy /= (posParabolid.z+1.0);
  posParabolid.zw  = vec2((l - in_near)/(in_far - in_near), 1.0);
  return posParabolid;
}
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
// TODO: more generic
#if RENDER_TARGET == CUBE
  return transformWorldToEye(posWorld, in_viewMatrix[layer]);
#else
  return transformWorldToEye(posWorld, in_viewMatrix);
#endif
}
vec4 transformWorldToEye(vec3 posWorld, int layer) {
  return transformWorldToEye(vec4(posWorld,1.0),layer);
}
#endif

-- transformEyeToScreen
#ifndef __transformEyeToScreen_INCLUDED
#define2 __transformEyeToScreen_INCLUDED
#include regen.states.camera.input
vec4 transformEyeToScreen(vec4 posEye, int layer) {
// TODO: more generic
#if RENDER_TARGET == 2D_ARRAY
  return in_projectionMatrix[layer] * posEye;
#else
  return in_projectionMatrix * posEye;
#endif
}
vec4 transformEyeToScreen(vec3 posEye, int layer) {
  return transformEyeToScreen(vec4(posEye,1.0),layer);
}
#endif

-- transformWorldToScreen
#ifndef __transformEyeToScreen_INCLUDED
#define2 __transformEyeToScreen_INCLUDED
#include regen.states.camera.input
vec4 transformWorldToScreen(vec4 posWorld, int layer) {
#if RENDER_LAYER > 1
  return in_viewProjectionMatrix[layer] * posWorld;
#else
  return in_viewProjectionMatrix * posWorld;
#endif
}
vec4 transformWorldToScreen(vec3 posWorld, int layer) {
  return transformWorldToScreen(vec4(posWorld,1.0),layer);
}
#endif

-- transformWorldToTexco
#ifndef __transformWorldToTexco_included__
#define __transformWorldToTexco_included__
#include regen.states.camera.input
vec3 transformWorldToTexco(vec4 posWorld)
{
    vec4 posScreen = __VIEW_PROJ__*posWorld;
    return (posScreen.xyz/posScreen.w + vec3(1.0))*0.5;
}
#endif

-- transformEyeToTexco
#ifndef __transformEyeToTexco_included__
#define __transformEyeToTexco_included__
#include regen.states.camera.input
vec3 transformEyeToTexco(vec4 posEye)
{
    vec4 posScreen = __PROJ__*posEye;
    return (posScreen.xyz/posScreen.w + vec3(1.0))*0.5;
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
vec3 transformTexcoToWorld(vec2 texco, float depth) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = __VIEW_PROJ_INV__*vec4(pos0,1.0);
    return D.xyz/D.w;
}
#endif

-- transformTexcoToView
#ifndef __transformTexcoToView_included__
#define __transformTexcoToView_included__
#include regen.states.camera.input
vec3 transformTexcoToView(vec2 texco, float depth) {
    vec3 pos0 = vec3(texco.xy, depth)*2.0 - vec3(1.0);
    vec4 D = __PROJ_INV__*vec4(pos0,1.0);
    return D.xyz/D.w;
}
#endif

-- linearizeDepth
#ifndef __linearizeDepth_included__
#define __linearizeDepth_included__
//float linearizeDepth(float d, float n, float f)
//{
//    float z_n = 2.0*d - 1.0;
//    return 2.0*n*f/(f+n-z_n*(f-n));
//}
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
