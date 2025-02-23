
-- defines
#ifndef RENDER_LAYER
#define RENDER_LAYER 1
#endif
#ifndef RENDER_TARGET
#define RENDER_TARGET 2D
#endif
#if RENDER_LAYER == 1
#ifndef HAS_TESSELATION
    #ifndef HAS_GS_TRANSFORM
#define VS_CAMERA_TRANSFORM
    #endif
#endif
#if SHADER_STAGE == tes
#define TES_CAMERA_TRANSFORM
#endif
#define in_layer 0
#endif // RENDER_LAYER == 1
// Macros for Layered Camera access
#if RENDER_TARGET == CUBE || RENDER_TARGET == DUAL_PARABOLOID
#define REGEN_VIEW_(layer)          in_viewMatrix[layer]
#define REGEN_VIEW_INV_(layer)      in_inverseViewMatrix[layer]
#define REGEN_VIEW_PROJ_(layer)     in_viewProjectionMatrix[layer]
#define REGEN_VIEW_PROJ_INV_(layer) in_inverseViewProjectionMatrix[layer]
#define REGEN_CAM_DIR_(layer)       in_cameraDirection[layer]
#elif RENDER_TARGET == 2D_ARRAY
#define REGEN_VIEW_(layer)          in_viewMatrix[layer]
#define REGEN_VIEW_INV_(layer)      in_inverseViewMatrix[layer]
#define REGEN_PROJ_(layer)          in_projectionMatrix[layer]
#define REGEN_PROJ_INV_(layer)      in_inverseProjectionMatrix[layer]
#define REGEN_VIEW_PROJ_(layer)     in_viewProjectionMatrix[layer]
#define REGEN_VIEW_PROJ_INV_(layer) in_inverseViewProjectionMatrix[layer]
#define REGEN_CAM_NEAR_(layer)      in_near[layer]
#define REGEN_CAM_FAR_(layer)       in_far[layer]
#define REGEN_CAM_POS_(layer)       in_cameraPosition[layer]
#endif // RENDER_TARGET == 2D_ARRAY
#ifndef REGEN_VIEW_(layer)
#define REGEN_VIEW_(layer)          in_viewMatrix
#endif
#ifndef REGEN_VIEW_INV_(layer)
#define REGEN_VIEW_INV_(layer)      in_inverseViewMatrix
#endif
#ifndef REGEN_PROJ_(layer)
#define REGEN_PROJ_(layer)          in_projectionMatrix
#endif
#ifndef REGEN_PROJ_INV_(layer)
#define REGEN_PROJ_INV_(layer)      in_inverseProjectionMatrix
#endif
#ifndef REGEN_VIEW_PROJ_(layer)
#define REGEN_VIEW_PROJ_(layer)     in_viewProjectionMatrix
#endif
#ifndef REGEN_VIEW_PROJ_INV_(layer)
#define REGEN_VIEW_PROJ_INV_(layer) in_inverseViewProjectionMatrix
#endif
#ifndef REGEN_CAM_DIR_(layer)
#define REGEN_CAM_DIR_(layer)       in_cameraDirection
#endif
#ifndef REGEN_CAM_POS_(layer)
#define REGEN_CAM_POS_(layer)       in_cameraPosition
#endif
#ifndef REGEN_CAM_NEAR_(layer)
#define REGEN_CAM_NEAR_(layer)      in_near
#endif
#ifndef REGEN_CAM_FAR_(layer)
#define REGEN_CAM_FAR_(layer)       in_far
#endif
#ifdef USE_PARABOLOID_PROJECTION || IGNORE_VIEW_ROTATION || IGNORE_VIEW_TRANSLATION
#define SEPERATE_VIEW_PROJ
#endif

-- input
#ifndef REGEN_camera_input_INCLUDED
#define2 REGEN_camera_input_INCLUDED
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
#ifndef REGEN_transformWorldToEye_INCLUDED
#define2 REGEN_transformWorldToEye_INCLUDED
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
  return transformWorldToEye(posWorld, REGEN_VIEW_(layer));
}
vec4 transformWorldToEye(vec3 posWorld, int layer) {
  return transformWorldToEye(vec4(posWorld,1.0),REGEN_VIEW_(layer));
}
#endif
-- transformWorldToScreen
#ifndef REGEN_transformWorldToScreen_INCLUDED
#define2 REGEN_transformWorldToScreen_INCLUDED
#include regen.states.camera.input
#ifdef SEPERATE_VIEW_PROJ
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#endif
vec4 transformWorldToScreen(vec4 posWorld, int layer) {
#ifdef SEPERATE_VIEW_PROJ
  return transformEyeToScreen(transformWorldToEye(posWorld,layer),layer);
#else
  return REGEN_VIEW_PROJ_(layer) * posWorld;
#endif
}
#endif
-- transformWorldToTexco
#ifndef REGEN_transformWorldToTexco_included_
#define REGEN_transformWorldToTexco_included_
#include regen.states.camera.input
#include regen.states.camera.transformWorldToScreen
#include regen.states.camera.transformScreenToTexco
vec3 transformWorldToTexco(vec4 posWorld, int layer)
{
  return transformScreenToTexco(transformWorldToScreen(posWorld,layer));
}
#endif

-- transformEyeToScreen
#ifndef REGEN_transformEyeToScreen_INCLUDED
#define2 REGEN_transformEyeToScreen_INCLUDED
#include regen.states.camera.input
#ifdef USE_PARABOLOID_PROJECTION
#include regen.states.camera.transformParaboloid
#endif
vec4 transformEyeToScreen(vec4 posEye, int layer) {
#ifdef USE_PARABOLOID_PROJECTION
  return transformParaboloid(posEye,layer);
#else
  return REGEN_PROJ_(layer) * posEye;
#endif
}
#endif
-- transformEyeToTexco
#ifndef REGEN_transformEyeToTexco_included_
#define REGEN_transformEyeToTexco_included_
#include regen.states.camera.input
#include regen.states.camera.transformEyeToScreen
#include regen.states.camera.transformScreenToTexco
vec3 transformEyeToTexco(vec4 posEye, int layer)
{
  return transformScreenToTexco(transformEyeToScreen(posEye,layer));
}
#endif
-- transformEyeToWorld
#ifndef REGEN_transformEyeToWorld_INCLUDED
#define2 REGEN_transformEyeToWorld_INCLUDED
#include regen.states.camera.input
vec4 transformEyeToWorld(vec4 posEye, mat4 viewInv) {
#ifdef IGNORE_VIEW_ROTATION
  return posWorld - vec4(view[3].xyz,0.0);
#elif IGNORE_VIEW_TRANSLATION
  return mat4(viewInv[0], viewInv[1], viewInv[2], vec3(0.0), 1.0) * posEye;
#else
  return viewInv * posEye;
#endif
}
vec4 transformEyeToWorld(vec4 posEye, int layer) {
  return transformEyeToWorld(posEye, REGEN_VIEW_INV_(layer));
}
vec4 transformEyeToWorld(vec3 posEye, int layer) {
  return transformEyeToWorld(vec4(posEye,1.0),REGEN_VIEW_INV_(layer));
}
#endif

-- transformScreenToTexco
#ifndef REGEN_transformScreenToTexco_included_
#define REGEN_transformScreenToTexco_included_
#include regen.states.camera.input
vec3 transformScreenToTexco(vec4 posScreen)
{
  return 0.5*posScreen.xyz/posScreen.w + vec3(0.5);
}
#endif
-- transformScreenToEye
#ifndef REGEN_transformScreenToEye_included_
#define REGEN_transformScreenToEye_included_
#include regen.states.camera.input
#ifdef USE_PARABOLOID_PROJECTION
#include regen.states.camera.transformParaboloidInv
#endif
vec3 transformScreenToEye(vec4 posScreen, int layer)
{
#ifdef USE_PARABOLOID_PROJECTION
  return transformParaboloidInv(posScreen,layer);
#else
  vec4 posEye = REGEN_PROJ_INV_(layer) * posScreen;
  return posEye.xyz/posEye.w;
#endif
}
#endif
-- transformScreenToWorld
#ifndef REGEN_transformScreenToWorld_included_
#define REGEN_transformScreenToWorld_included_
#include regen.states.camera.input
#ifdef SEPERATE_VIEW_PROJ
#include regen.states.camera.transformScreenToEye
#include regen.states.camera.transformEyeToWorld
#endif
vec3 transformScreenToWorld(vec4 posScreen, int layer)
{
#ifdef SEPERATE_VIEW_PROJ
  return transformEyeToWorld(vec4(transformScreenToEye(posScreen,layer),1.0),layer).xyz;
#else
  vec4 posWorld = REGEN_VIEW_PROJ_INV_(layer) * posScreen;
  return posWorld.xyz/posWorld.w;
#endif
}
#endif

-- transformTexcoToScreen
#ifndef REGEN_transformTexcoToScreen_included_
#define REGEN_transformTexcoToScreen_included_
#include regen.states.camera.input
vec3 transformTexcoToScreen(vec2 texco, float depth) {
    return vec3(texco.xy, depth)*2.0 - vec3(1.0);
}
#endif
-- transformTexcoToView
#ifndef REGEN_transformTexcoToView_included_
#define REGEN_transformTexcoToView_included_
#include regen.states.camera.input
#include regen.states.camera.transformTexcoToScreen
#include regen.states.camera.transformScreenToEye
vec3 transformTexcoToView(vec2 texco, float depth, int layer) {
    return transformScreenToEye(
      vec4(transformTexcoToScreen(texco,depth),1.0), layer);
}
#endif
-- transformTexcoToWorld
#ifndef REGEN_transformTexcoToWorld_included_
#define REGEN_transformTexcoToWorld_included_
#include regen.states.camera.input
#include regen.states.camera.transformTexcoToScreen
#include regen.states.camera.transformScreenToWorld
vec3 transformTexcoToWorld(vec2 texco, float depth, int layer) {
    return transformScreenToWorld(
      vec4(transformTexcoToScreen(texco,depth),1.0), layer);
}
#endif

-- transformParaboloid
#ifndef REGEN_transformParaboloid_INCLUDED
#define2 REGEN_transformParaboloid_INCLUDED
#include regen.states.camera.input
vec4 transformParaboloid(vec4 posEye, int layer) {
    vec3 pos = posEye.xyz;
    // normalize incoming vector by its w component
    pos /= posEye.w;
    float l = length(pos.xyz);
    pos /= l;
    // x/y coordinates of the paraboloid
	pos.xy /= pos.z + 1.0f;
	pos.x = -pos.x;
	// distance from pos to the origin (0,0,0)
    pos.z = (l - REGEN_CAM_NEAR_(layer)) / (REGEN_CAM_FAR_(layer) - REGEN_CAM_NEAR_(layer));
    // NOTE: could add bias here if self-shadowing occurs
    //const float zBias = 0.01;
    //pos.z += zBias;
    return vec4(pos,1.0);
}
#endif
-- transformParaboloidInv
#ifndef REGEN_transformParaboloidInv_INCLUDED
#define2 REGEN_transformParaboloidInv_INCLUDED
#include regen.states.camera.input
vec3 transformParaboloidInv(vec4 posScreen, int layer) {
    float l = posScreen.z*(REGEN_CAM_FAR_(layer) - REGEN_CAM_NEAR_(layer)) + REGEN_CAM_NEAR_(layer);
    float k = dot(posScreen.xy,posScreen.xy);
    float z = -l*(k-1)/(k+1);
    posScreen.x *= -1.0;
    return vec3(posScreen.xy*(z+l), z);
}
#endif

-- linearizeDepth
#ifndef REGEN_linearizeDepth_included_
#define REGEN_linearizeDepth_included_
float linearizeDepth(float expDepth, float n, float f)
{
    return (2.0*n)/(f+n - expDepth*(f-n));
}
#endif
-- exponentialDepth
#ifndef REGEN_exponentialDepth_included_
#define REGEN_exponentialDepth_included_
float exponentialDepth(float linearDepth, float n, float f)
{
  return ((f+n)*linearDepth - (2.0*n)) / ((f-n)*linearDepth);
}
#endif

-- depthCorrection
#ifndef REGEN_depthCorrection_Include_
#define REGEN_depthCorrection_Include_
#include regen.states.camera.transformEyeToScreen
void depthCorrection(float depth, int layer)
{
  vec3 pe = in_posEye + depth*normalize(in_posEye);
  vec4 ps = transformEyeToScreen(vec4(pe,1.0),layer);
  gl_FragDepth = (ps.z/ps.w)*0.5 + 0.5;
}
#endif
