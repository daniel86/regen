
-- defines
#ifndef RENDER_LAYER
#define RENDER_LAYER 1
#endif
#endif
#ifndef RENDER_TARGET
#define RENDER_TARGET 2D
#endif

#if RENDER_TARGET == CUBE
#if SHADER_STAGE == fs
#define __VIEW__          in_viewMatrix[in_layer]
#define __VIEW_INV__      in_inverseViewMatrix[in_layer]
#define __PROJ__          in_projectionMatrix
#define __PROJ_INV__      in_inverseProjectionMatrix
#define __VIEW_PROJ__     in_viewProjectionMatrix[in_layer]
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix[in_layer]
#define __CAM_DIR__       in_cameraDirection[in_layer]
#define __CAM_POS__       in_cameraPosition
#define __CAM_NEAR__      in_near
#define __CAM_FAR__       in_far
#endif

#elif RENDER_TARGET == 2D_ARRAY
#if SHADER_STAGE == fs
#define __VIEW__          in_viewMatrix
#define __VIEW_INV__      in_inverseViewMatrix
#define __PROJ__          in_projectionMatrix[in_layer]
#define __PROJ_INV__      in_inverseProjectionMatrix[in_layer]
#define __VIEW_PROJ__     in_viewProjectionMatrix[in_layer]
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix[in_layer]
#define __CAM_DIR__       in_cameraDirection
#define __CAM_POS__       in_cameraPosition
#define __CAM_NEAR__      in_near[in_layer]
#define __CAM_FAR__       in_far[in_layer]
#endif

#else // RENDER_TARGET == 2D
#if SHADER_STAGE == fs
#define __VIEW__          in_viewMatrix
#define __VIEW_INV__      in_inverseViewMatrix
#define __PROJ__          in_projectionMatrix
#define __PROJ_INV__      in_inverseProjectionMatrix
#define __VIEW_PROJ__     in_viewProjectionMatrix
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix
#define __CAM_DIR__       in_cameraDirection
#define __CAM_POS__       in_cameraPosition
#define __CAM_NEAR__      in_near
#define __CAM_FAR__       in_far
#endif
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

vec4 transformEyeToScreen(vec4 posEye, int layer) {
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

vec4 transformWorldToScreen(vec4 posWorld, int layer) {
#if RENDER_TARGET == 2D_ARRAY
  return in_viewProjectionMatrix[layer] * posWorld;
#else
  return in_viewProjectionMatrix * posWorld;
#endif
}

vec4 transformWorldToScreen(vec3 posWorld, int layer) {
  return transformWorldToScreen(vec4(posWorld,1.0),layer);
}

#endif
