
--------------------------------
--------------------------------
----- Visualization of star map.
----- Code based on: https://code.google.com/p/osghimmel/
--------------------------------
--------------------------------
-- vs
#include regen.models.mesh.defines

in vec3 in_pos;
#if RENDER_LAYER <= 1
out vec4 out_eye;
out vec4 out_ray;
#endif

uniform mat4 in_equToHorMatrix;

#include regen.states.camera.input

void main() {
    vec4 p = vec4(in_pos.xy, 0.0, 1.0);
    gl_Position = p;
#if RENDER_LAYER <= 1
    out_eye = in_inverseProjectionMatrix * p * in_viewMatrix;
    out_ray = in_equToHorMatrix * out_eye;
#endif
}

-- gs
#include regen.states.camera.defines
#include regen.defines.all
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

out vec4 out_eye;
out vec4 out_ray;
flat out int out_layer;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, int index, int layer) {
  out_eye = __PROJ_INV__(layer) * posWorld * __VIEW__(layer);
  out_ray = in_equToHorMatrix * out_eye;
  gl_Position = posWorld;
  HANDLE_IO(index);
  EmitVertex();
}

void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  // select framebuffer layer
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
  emitVertex(gl_PositionIn[0], 0, ${LAYER});
  emitVertex(gl_PositionIn[1], 1, ${LAYER});
  emitVertex(gl_PositionIn[2], 2, ${LAYER});
  EndPrimitive();
#endif // SKIP_LAYER
#endfor
}
#endif

-- fs
out vec4 out_color;
in vec4 in_eye;
in vec4 in_ray;

uniform vec3 in_sunPosition;
uniform float in_q;
uniform vec4 in_cmn;
uniform vec2 in_inverseViewport;

const float in_deltaM = 1.0;
const float in_scattering = 1.0;
const float surfaceHeight = 0.99;

uniform samplerCube in_starmapCube;

#include regen.sky.utility.scatter
#include regen.sky.utility.computeEyeExtinction

void main(void) {
  vec3 eye = normalize(in_eye.xyz);
  float ext = computeEyeExtinction(eye);
  if(ext <= 0.0) discard;
  
  vec3 stu = normalize(in_ray.xyz);
  vec4 fc = texture(in_starmapCube, stu);
  fc *= 3e-2 / sqrt(in_q) * in_deltaM;
  
  float omega = acos(eye.y * 0.9998);
  // Day-Twilight-Night-Intensity Mapping (Butterworth-Filter)
  float b = 1.0 / sqrt(1 + pow(in_sunPosition.z + 1.14, 32));
  
  out_color = smoothstep(0.0, 0.05, ext) * vec4(b * (
	fc.rgb - in_scattering*scatter(omega)), 1.0);
}
