--------------------------------
--------------------------------
----- A simple sky box mesh. Depth test should be done against scene depth
----- buffer. The cube faces are translated to the far plane. The depth
----- function should be less or equal.
--------------------------------
--------------------------------
-- emitVertex
#define HANDLE_IO(i)
void emitVertex(vec3 pos, int index, int layer) {
    vec4 posWorld = vec4(normalize(pos),1.0);
    out_posWorld = posWorld;
    out_posEye = transformWorldToEye(posWorld,layer);
    gl_Position = transformEyeToScreen(out_posEye,layer);
    gl_Position.z = gl_Position.w;
    HANDLE_IO(index);
}

-- vs_include
#extension GL_EXT_gpu_shader4 : enable
#include regen.models.mesh.defines

in vec3 in_pos;

#include regen.states.camera.input

#if RENDER_LAYER == 1
out vec4 out_posWorld;
out vec4 out_posEye;
#endif

#if RENDER_LAYER == 1
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen
#include regen.models.sky-box.emitVertex
#endif
-- vs
#include regen.models.sky-box.vs_include
#if RENDER_LAYER == 1
void main() {
    emitVertex(in_pos.xyz, gl_VertexID, 0);
}
#else
#define HANDLE_IO(i)
void main() {
    gl_Position = vec4(in_pos,0.0);
    HANDLE_IO(gl_VertexID);
}
#endif

-- tcs
#include regen.models.mesh.tcs
-- tes
#include regen.models.mesh.tes

-- gs_include
#include regen.states.camera.defines
#include regen.defines.all
#extension GL_EXT_geometry_shader4 : enable
#define2 __MAX_VERTICES__ ${${RENDER_LAYER}*3}

layout(triangles) in;
layout(triangle_strip, max_vertices=${__MAX_VERTICES__}) out;

out vec4 out_posWorld;
out vec4 out_posEye;
flat out int out_layer;

#include regen.states.camera.input
#include regen.states.camera.transformWorldToEye
#include regen.states.camera.transformEyeToScreen

#include regen.models.sky-box.emitVertex
-- gs
#if RENDER_LAYER > 1
#include regen.models.sky-box.gs_include
void main() {
#for LAYER to ${RENDER_LAYER}
#ifndef SKIP_LAYER${LAYER}
  gl_Layer = ${LAYER};
  out_layer = ${LAYER};
  emitVertex(gl_PositionIn[0].xyz, 0, ${LAYER}); EmitVertex();
  emitVertex(gl_PositionIn[1].xyz, 1, ${LAYER}); EmitVertex();
  emitVertex(gl_PositionIn[2].xyz, 2, ${LAYER}); EmitVertex();
  EndPrimitive();
#endif // SKIP_LAYER
#endfor
}
#endif

-- fs
#include regen.models.mesh.defines
#include regen.states.textures.defines

out vec4 out_color;

#include regen.states.textures.input
#include regen.states.textures.mapToFragmentUnshaded

void main() {
    textureMappingFragmentUnshaded(gl_FragCoord.xyz, out_color);
}
