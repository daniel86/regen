--------------------------------
--------------------------------
----- A simple sky box mesh. Depth test should be done against scene depth
----- buffer. The cube faces are translated to the far plane. The depth
----- function should be less or equal.
--------------------------------
--------------------------------
-- vs
#extension GL_EXT_gpu_shader4 : enable
#include regen.meshes.mesh.defines

in vec3 in_pos;

uniform float in_far;

#define HANDLE_IO(i)

#if RENDER_LAYER > 1
void main() {
    vec4 posWorld = vec4(in_pos.xyz*in_far*0.99,1.0);
    gl_Position = vec4(in_pos.xyz*in_far*0.99,1.0);
    HANDLE_IO(gl_VertexID);
}
#else
#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

void main() {
    vec4 posWorld = vec4(in_pos.xyz*in_far*0.99,1.0);
    vec4 posScreen = transformEyeToScreen(transformWorldToEye(posWorld,0),0);
    // push to far plane. needs less or equal check
    posScreen.z = posScreen.w;
    gl_Position = posScreen;
    HANDLE_IO(gl_VertexID);
}
#endif

-- gs
#if RENDER_LAYER > 1
#extension GL_EXT_geometry_shader4 : enable
#include regen.meshes.mesh.defines

layout(triangles) in;
// TODO: use ${RENDER_LAYER}*3
layout(triangle_strip, max_vertices=18) out;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

out vec3 out_posWorld;
out vec3 out_posEye;

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
  
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  // select framebuffer layer
  gl_Layer = ${LAYER};
#if RENDER_TARGET == CUBE
  view = in_viewMatrix[${LAYER}];
  proj = in_projectionMatrix;
#elif RENDER_TARGET == 2D_ARRAY
  view = in_viewMatrix;
  proj = in_projectionMatrix[${LAYER}];
#endif
  emitVertex(gl_PositionIn[0], view, proj, 0);
  emitVertex(gl_PositionIn[1], view, proj, 1);
  emitVertex(gl_PositionIn[2], view, proj, 2);
  EndPrimitive();
#endif
#endfor
}
#endif

-- fs
#include regen.meshes.mesh.defines
#include regen.utility.textures.defines

out vec4 out_color;

#include regen.utility.textures.input
#include regen.utility.textures.mapToFragmentUnshaded

void main() {
    textureMappingFragmentUnshaded(gl_FragCoord.xyz, out_color);
}
