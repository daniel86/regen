
-- gs-geometry
#include regen.states.camera.defines
#include regen.defines.all
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

out vec3 out_posWorld;
out vec3 out_posEye;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye

#define HANDLE_IO(i)

void emitVertex(vec4 posWorld, mat4 view, mat4 proj, int index) {
  vec4 posEye = transformWorldToEye(posWorld, view);
  out_posWorld = posWorld.xyz;
  out_posEye = posEye.xyz;
  gl_Position = proj * posEye;
  HANDLE_IO(index);
  
  EmitVertex();
}

void main() {
  mat4 view, proj;
#if RENDER_TARGET == CUBE
  proj = in_projectionMatrix;
#elif RENDER_TARGET == 2D_ARRAY
  view = in_viewMatrix;
#endif
  
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  // select framebuffer layer
  gl_Layer = ${LAYER};
#if RENDER_TARGET == CUBE
  view = in_viewMatrix[${LAYER}];
#elif RENDER_TARGET == 2D_ARRAY
  proj = in_projectionMatrix[${LAYER}];
#endif
  emitVertex(gl_PositionIn[0], view, proj, 0);
  emitVertex(gl_PositionIn[1], view, proj, 1);
  emitVertex(gl_PositionIn[2], view, proj, 2);
  EndPrimitive();
#endif // SKIP_LAYER
#endfor
}
#endif