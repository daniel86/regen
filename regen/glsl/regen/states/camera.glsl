
-- input
#if RENDER_TARGET == CUBE
uniform mat4 in_viewMatrix[6];
uniform mat4 in_inverseViewMatrix[6];
uniform mat4 in_projectionMatrix;
uniform mat4 in_inverseProjectionMatrix;
uniform mat4 in_viewProjectionMatrix[6];
uniform mat4 in_inverseViewProjectionMatrix[6];
#if SHADER_STAGE == fs
#define __VIEW__          in_viewMatrix[in_layer]
#define __VIEW_INV__      in_inverseViewMatrix[in_layer]
#define __PROJ__          in_projectionMatrix
#define __PROJ_INV__      in_inverseProjectionMatrix
#define __VIEW_PROJ__     in_viewProjectionMatrix[in_layer]
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix[in_layer]
#endif

#elif RENDER_TARGET == 2D_ARRAY
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix[${RENDER_LAYER}];
uniform mat4 in_viewProjectionMatrix[${RENDER_LAYER}];
#if SHADER_STAGE == fs
#define __VIEW__          in_viewMatrix
#define __VIEW_INV__      in_inverseViewMatrix
#define __PROJ__          in_projectionMatrix[in_layer]
#define __PROJ_INV__      in_inverseProjectionMatrix[in_layer]
#define __VIEW_PROJ__     in_viewProjectionMatrix[in_layer]
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix[in_layer]
#endif

#else // RENDER_TARGET == 2D
uniform mat4 in_viewMatrix;
uniform mat4 in_projectionMatrix;
uniform mat4 in_viewProjectionMatrix;
#if SHADER_STAGE == fs
#define __VIEW__          in_viewMatrix
#define __VIEW_INV__      in_inverseViewMatrix
#define __PROJ__          in_projectionMatrix
#define __PROJ_INV__      in_inverseProjectionMatrix
#define __VIEW_PROJ__     in_viewProjectionMatrix
#define __VIEW_PROJ_INV__ in_inverseViewProjectionMatrix
#endif
#endif
// TODO: layered, direction
uniform vec3 in_cameraPosition;
uniform vec3 in_cameraDirection;

-- transformWorldToEye
#ifndef __transformWorldToEye_INCLUDED
#define2 __transformWorldToEye_INCLUDED

vec4 transformWorldToEye(vec4 ws, mat4 view) {
#ifdef IGNORE_VIEW_ROTATION
    return vec4(view[3].xyz,0.0) + ws;
#elif IGNORE_VIEW_TRANSLATION
    return mat4(view[0], view[1], view[2], vec3(0.0), 1.0) * ws;
#else
    return view * ws;
#endif
}
vec4 transformWorldToEye(vec4 ws, int layer) {
#if RENDER_TARGET == CUBE
  return transformWorldToEye(ws, in_viewMatrix[layer]);
#else
  return transformWorldToEye(ws, in_viewMatrix);
#endif
}

#endif

-- transformEyeToScreen
#ifndef __transformEyeToScreen_INCLUDED
#define2 __transformEyeToScreen_INCLUDED

vec4 transformEyeToScreen(vec4 es, int layer) {
#if RENDER_TARGET == 2D_ARRAY
  return in_projectionMatrix[layer] * es;
#else
  return in_projectionMatrix * es;
#endif
}

#endif

-- transformWorldToScreen
#ifndef __transformEyeToScreen_INCLUDED
#define2 __transformEyeToScreen_INCLUDED

vec4 transformWorldToScreen(vec4 es, int layer) {
#if RENDER_TARGET == 2D_ARRAY
  return in_viewProjectionMatrix[layer] * es;
#else
  return in_viewProjectionMatrix * es;
#endif
}

#endif
